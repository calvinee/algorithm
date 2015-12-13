/*
 * Copyright 1993-2015 NVIDIA Corporation.  All rights reserved.
 *
 * Please refer to the NVIDIA end user license agreement (EULA) associated
 * with this source code for terms and conditions that govern your use of
 * this software. Any use, reproduction, disclosure, or distribution of
 * this software and related documentation outside the terms of the EULA
 * is strictly prohibited.
 *
 */

// includes, system
#include <iostream>
#include <stdlib.h>
#include <vector>
#include <map>
// Required to include CUDA vector types
#include <cuda_runtime.h>
#include <vector_types.h>
#include <time.h>  
#include <helper_functions.h>
#include <helper_cuda.h>

using namespace std;


void initCUDAandShowMessage();
extern "C" void runCUDA(int blocks, int threads, int *const a_gpu, int *const b_gpu, int *const c_gpu);
extern "C" void runCUDA_AddOne(int blocks, int threads, int *const a_gpu, int *const b_gpu, int *const c_gpu);

void multipleCPU(int *const a_gpu, int *const b_gpu, int *const c_gpu)
{
	for (int i = 0; i < 1024*1024; i++)
	{
		c_gpu[i] = a_gpu[i] * b_gpu[i];
		for (int j = 0; j < 10240; j++)
			c_gpu[i]++;
	}
}

////////////////////////////////////////////////////////////////////////////////
// Program main
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
	//��ʾ�Կ��������Ϣ
	initCUDAandShowMessage();



	//�������߳̿�����
	int blocks = 1;

	//ÿ���߳̿鿪�����߳�����
	int threads = 1024;

	//��������a��b��c������ʼ��
	int *a_cpu = new int[1024 * 1024];
	int *b_cpu = new int[1024 * 1024];
	int *c_cpu = new int[1024 * 1024];
	for (int i = 0; i < 1024 * 1024; i++)
	{
		a_cpu[i] = 100;
		b_cpu[i] = 300;
		c_cpu[i] = 0;
	}

	//����gpu�ϵı���
	int *a_gpu, *b_gpu, *c_gpu;

	//��gpu�Ϸ����ڴ�
	cudaMalloc((void **)&a_gpu, sizeof(int) * 1024);
	cudaMalloc((void **)&b_gpu, sizeof(int) * 1024);
	cudaMalloc((void **)&c_gpu, sizeof(int) * 1024);

	//�����ݴ��ڴ渴�Ƶ��Դ�
	cudaMemcpy(a_gpu, a_cpu, sizeof(int) * 1024, cudaMemcpyHostToDevice);
	cudaMemcpy(b_gpu, b_cpu, sizeof(int) * 1024, cudaMemcpyHostToDevice);



	//windows���н���ʱ��ͳ��
	time_t  StartTime;
	time_t  EndTime;
	double  TimeUse = 0;
	time(&StartTime);


	//////////////////////////////////////////////////////
	//CUDA����
	//////////////////////////////////////////////////////
	runCUDA(blocks, threads,a_gpu, b_gpu, c_gpu);


	//windows���н���ʱ��ͳ��
	time(&EndTime);
	TimeUse = difftime(EndTime, StartTime);

	//����gpu�д�����ϵ����ݴ��Դ渴�Ƶ��ڴ�
	cudaMemcpy(c_cpu, c_gpu, sizeof(int) * 1024, cudaMemcpyDeviceToHost);
	

	//�ͷ���ر������Դ�
	cudaFree(a_gpu);
	cudaFree(b_gpu);
	cudaFree(c_gpu);

	cout << "GPU�ܻ���ʱ��Ϊ��" << (int)TimeUse / 60 << "��" << ((int)TimeUse) % 60 << "��" << endl;
	for (int i = 0; i < 1024; i+=200)
	{
		printf("c_cpu[%d]=%d\n",i, c_cpu[i]);
	}



	//////////////////////////////////////////////////////
	//CPU����
	//////////////////////////////////////////////////////

	//windows���н���ʱ��ͳ��
	time(&StartTime);
	multipleCPU(a_cpu, b_cpu, c_cpu);
	//windows���н���ʱ��ͳ��
	time(&EndTime);
	TimeUse = difftime(EndTime, StartTime);
	cout << "CPU�ܻ���ʱ��Ϊ��" << (int)TimeUse / 60 << "��" << ((int)TimeUse) % 60 << "��" << endl;
	for (int i = 0; i < 1024; i += 200)
	{
		printf("c_cpu[%d]=%d\n", i, c_cpu[i]);
	}

	//�ͷ��ڴ�
	free(a_cpu);
	free(b_cpu);
	free(c_cpu);



    // cudaDeviceReset causes the driver to clean up all state. While
    // not mandatory in normal operation, it is good practice.  It is also
    // needed to ensure correct operation when the application is being
    // profiled. Calling cudaDeviceReset causes all profile data to be
    // flushed before the application exits
    cudaDeviceReset();
    exit(1);
}

/*
������  ��initCUDAandShowMessage
�������ܣ���ȡ�Կ���Ϣ���������
*/
void initCUDAandShowMessage()
{/*
 name
 ���ڱ�ʶ�豸��ASCII�ַ�����
 totalGlobalMem
 �豸�Ͽ��õ�ȫ�ִ洢�������������ֽ�Ϊ��λ��
 sharedMemPerBlock
 �߳̿����ʹ�õĹ���洢�������ֵ�����ֽ�Ϊ��λ���ദ�����ϵ������߳̿����ͬʱ������Щ�洢����
 regsPerBlock
 �߳̿����ʹ�õ�32λ�Ĵ��������ֵ���ദ�����ϵ������߳̿����ͬʱ������Щ�Ĵ�����
 warpSize
 ���̼߳����warp���С��
 memPitch
 ����ͨ��cudaMallocPitch()Ϊ�����洢������Ĵ洢�����ƺ������������ࣨpitch�������ֽ�Ϊ��λ��
 maxThreadsPerBlock
 ÿ�����е�����߳�����
 maxThreadsDim[3]
 �����ά�ȵ����ֵ��
 maxGridSize[3]
 �������ά�ȵ����ֵ��
 totalConstMem
 �豸�Ͽ��õĲ���洢�����������ֽ�Ϊ��λ��
 major��minor
 �����豸������������Ҫ�޶��źʹ�Ҫ�޶��ţ�
 clockRate
 ��ǧ��Ϊ��λ��ʱ��Ƶ�ʣ�
 textureAlignment
 ����Ҫ����textureAlignment�ֽڶ���������ַ���������ȡ��Ӧ��ƫ�ƣ�
 deviceOverlap
 ����豸�����������豸֮�䲢�����ƴ洢����ͬʱ����ִ���ںˣ����ֵΪ 1�������ֵΪ 0��
 multiProcessorCount
 �豸�϶ദ������������
 */
	cudaDeviceProp prop;
	int count;
	cudaGetDeviceCount(&count);
	for (int i = 0; i < count; i++)
	{
		cudaGetDeviceProperties(&prop, i);
		cudaDeviceProp sDevProp = prop;
		printf("Device name         : %s\n", sDevProp.name); //���ڱ�ʶ�豸��ASCII�ַ�����
		printf("Device memory       : %d\n", sDevProp.totalGlobalMem);//�豸�Ͽ��õ�ȫ�ִ洢�������������ֽ�Ϊ��λ��
		printf("Memory per-block    : %d\n", sDevProp.sharedMemPerBlock);//�߳̿����ʹ�õĹ���洢�������ֵ�����ֽ�Ϊ��λ���ദ�����ϵ������߳̿����ͬʱ������Щ�洢����
		printf("Register per-block  : %d\n", sDevProp.regsPerBlock);//�߳̿����ʹ�õ�32λ�Ĵ��������ֵ���ദ�����ϵ������߳̿����ͬʱ������Щ�Ĵ�����
		printf("Warp size           : %d\n", sDevProp.warpSize);//���̼߳����warp���С��
		printf("Memory pitch        : %d\n", sDevProp.memPitch);//����ͨ��cudaMallocPitch()Ϊ�����洢������Ĵ洢�����ƺ������������ࣨpitch�������ֽ�Ϊ��λ��
		printf("Constant Memory     : %d\n", sDevProp.totalConstMem);//�豸�Ͽ��õĲ���洢�����������ֽ�Ϊ��λ�� 
		printf("Max thread per-block: %d\n", sDevProp.maxThreadsPerBlock);//ÿ�����е�����߳���;
		printf("Max thread dim      : ( %d, %d, %d )\n", sDevProp.maxThreadsDim[0],
			sDevProp.maxThreadsDim[1], sDevProp.maxThreadsDim[2]);//
		printf("Max grid size       : ( %d, %d, %d )\n", sDevProp.maxGridSize[0],
			sDevProp.maxGridSize[1], sDevProp.maxGridSize[2]);//
		printf("Ver                 : %d.%d\n", sDevProp.major, sDevProp.minor);//
		printf("Clock               : %d\n", sDevProp.clockRate);//��ǧ��Ϊ��λ��ʱ��Ƶ�ʣ�
		printf("textureAlignment    : %d\n", sDevProp.textureAlignment);//����Ҫ����textureAlignment�ֽڶ���������ַ���������ȡ��Ӧ��ƫ�ƣ�
		printf("�ദ����MP������    : %d\n", sDevProp.multiProcessorCount);//�豸�϶ദ����������
		cudaSetDevice(i);
		printf("\n CUDA initialized.\n");
	}
}
