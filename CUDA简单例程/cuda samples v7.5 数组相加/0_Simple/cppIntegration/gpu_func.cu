////////////////////////////////////////////////////////////////////////////
//
// Copyright 1993-2015 NVIDIA Corporation.  All rights reserved.
//
// Please refer to the NVIDIA end user license agreement (EULA) associated
// with this source code for terms and conditions that govern your use of
// this software. Any use, reproduction, disclosure, or distribution of
// this software and related documentation outside the terms of the EULA
// is strictly prohibited.
//
////////////////////////////////////////////////////////////////////////////

/*
* ��������ƴͼ�����GPU�ⷨ
*/

// System includes
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>

// CUDA runtime
#include <cuda_runtime.h>

// helper functions and utilities to work with CUDA
#include <helper_cuda.h>
#include <helper_functions.h>

/*
������  ��addOne
�������ܣ�����+1����
���������
num      :��Ҫ��1����
*/
__device__ void addOne(int &num)
{
	num++;
}
/*
������  ��gpuTestSiukwanAddOne
�������ܣ�����a_gpu[i] * b_gpu[i]�����ѽ����ŵ�c_gpu[i]�У�ʵ���豸�˵ĺ�������
���������
a_gpu    :�����ľ���
b_gpu    :��������ͼ��
c_gpu    :��������ĵ�һ���ո�λ��x
*/
__global__ void gpuTestSiukwanAddOne(int *const a_gpu, int *const b_gpu, int *const c_gpu)
{
	// write data to global memory
	const unsigned int tid = (blockIdx.x*blockDim.x) + threadIdx.x;
	c_gpu[tid] = a_gpu[tid] * b_gpu[tid];
	for (int i = 0; i < 10240; i++)
		addOne(c_gpu[tid]);
}

/*
������  ��gpuTestSiukwan
�������ܣ�����a_gpu[i] * b_gpu[i]�����ѽ����ŵ�c_gpu[i]��,
���������
a_gpu    :�����ľ���
b_gpu    :��������ͼ��
c_gpu    :��������ĵ�һ���ո�λ��x
*/
__global__ void gpuTestSiukwan(int *const a_gpu, int *const b_gpu, int *const c_gpu)
{
	// write data to global memory
	const unsigned int tid = (blockIdx.x*blockDim.x) + threadIdx.x;
	c_gpu[tid] = a_gpu[tid] * b_gpu[tid];
	for (int i = 0; i < 10240; i++)
		c_gpu[tid]++;
}

//�ӿں���
extern "C" void runCUDA(int blocks, int threads, int *const a_gpu, int *const b_gpu, int *const c_gpu)
{
	gpuTestSiukwan << < blocks, threads >> >(a_gpu, b_gpu, c_gpu);
}

//�ӿں���
extern "C" void runCUDA_AddOne(int blocks, int threads, int *const a_gpu, int *const b_gpu, int *const c_gpu)
{
	gpuTestSiukwanAddOne << < blocks, threads >> >(a_gpu, b_gpu, c_gpu);
}