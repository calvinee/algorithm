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

/* 
 * ��������ƴͼ�����GPU�ⷨ
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
////////////////////////////////////////////////////////////////////////////////
// С����
////////////////////////////////////////////////////////////////////////////////
vector<vector<char>> ReverseGraph(vector<vector<char>>& g);
vector<vector<char>> RotateGraph90(vector<vector<char>>& g);
vector<vector<char>> RotateGraph(vector<vector<char>>& g, int times);
void initGraphs(vector<vector<vector<char>>>&g);

/*��¼����ͼ�εĳ������״*/
struct GraphNode
{
	char shape[5][5];      //ͼ�ε���״����ΪboolҲռ����1B������ʹ��char����bool������
	int x ;                //ͼ�εĸ߶�
	int y;                 //ͼ�εĿ��
	int fill_x;            //ͼ�εĵ�һ����ֵ�������x
	int fill_y;            //ͼ�εĵ�һ����ֵ�������y
};

/*
 *��¼ÿ��ͼ�εı��Σ�
 *����ԭʼ����ת����ת��
 *�����8��
 */
struct GraphFormat
{
	GraphNode format[8];   //ͼ�εı��Σ������8��
	int formatCount;       //ͼ�εı�������
	char c;                //ͼ�εı�ţ�123456789abc
};

/*
 *�洢����ͼ��
 *һ��12��ͼ��
 */
struct GraphAll
{
	GraphFormat graph[12]; //����ͼ��
	int graphCount;        //ͼ������
};

/*
 *��Ҫ���ľ������ݽṹ
 *ÿ��GPU�̷߳���һ��
 */
struct MatrixNode{
	char shape[20][20];    //ȷ����12����ʼͼ�Σ�ͼ�δ���3*3��С�ģ�����С��3�ģ������ڴ𰸡����ٴ�3*20��ʼ��������״��Сֱ�Ӷ���Ϊ20*20
	int x, y;              //����ĳ���
	bool solution[12][8];  //�������һ�������Ŀ�����
	bool graphUsed[12];    //��¼12��ͼ�ε�ʹ�����
	int thisLevelCount;    //�Ե�ǰ����Ϊ�������ܹ������ͼ�ε�����
	int depth;             //�����ȣ�Ҳ�൱���Ѿ�����˶��ٸ�ͼ��
};


//���к�������
void  PrintTitle();
void  printAllGraphStruct();
int   initCUDAandShowMessage();
bool  CinParameters(int&m, int&n);
void  initGraphStruct(GraphAll&cpu_graph);
void  initTmpMatAndCinMN(MatrixNode &cpu_mat_tmp);
void  getFirstUnfill_cpu(MatrixNode &g_mat, int &x, int&y);
bool  canFillMatrix_cpu(MatrixNode &g_mat, GraphNode &graph, int&toFill_x, int&toFill_y, int&fill_x, int&fill_y);
void  fillMatrix_cpu(MatrixNode &g_mat, GraphNode &graph, int&toFill_x, int&toFill_y, int&fill_x, int&fill_y, char&c);
void  PrintMatrix_cpu(MatrixNode &g_mat);
void  refreshSolution(MatrixNode *cpu_mat, int & solutionSum, vector<MatrixNode>& result);

extern "C" bool SiukwanTest(GraphAll *const g_graph);
extern "C" void runCUDA(int blocks, int threads, MatrixNode *const g_mat, GraphAll *const g_graph, int*solutionSum);


GraphAll cpu_graph;//cpuͼ�ζ���Ϊȫ�ֱ���


////////////////////////////////////////////////////////////////////////////////
// Program main
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
	//��ʾ�Կ��������Ϣ
	if (initCUDAandShowMessage() == 0)
	{
		cout << "û�м�⵽Nvida�Կ���CUDA��" << endl;
		cout << "���������" << endl;
		cout << "1.��ʹ�ð�װ��Nvida�Կ���CUDA toolkit 7.5�ĵ��Խ������иó���" << endl;
		cout << "2.��ֱ������CPU�汾���򣺵��̼߳����̳߳���.exe" << endl;
		getchar();
		return 0;
	}

	//��ʼ��ͼ�εĽṹ�壬Ԥ�ȼ����12��ͼ�εĸ��ֱ��Σ����洢��cpu_graph
	initGraphStruct(cpu_graph);

	//����gpu�ı�������������ں�����ʹ���У�ֻ�ᱻ��ȡ������Ҫд��
	GraphAll *gpu_graph;

	//��gpu�Ϸ����ڴ�
	cudaMalloc((void **)&gpu_graph, sizeof(cpu_graph));

	//�����ݴ��ڴ渴�Ƶ��Դ�
	cudaMemcpy(gpu_graph, &cpu_graph, sizeof(cpu_graph), cudaMemcpyHostToDevice);

	//����gpu�д�����ϵ����ݴ��Դ渴�Ƶ��ڴ�
	cudaMemcpy(&cpu_graph, gpu_graph, sizeof(cpu_graph), cudaMemcpyDeviceToHost);
	

	//��ӡ��Ŀ�������Ϣ
	PrintTitle();

	//ѭ���ȴ�����
	while (1)
	{
		//�������
		MatrixNode cpu_mat_tmp;

		//��ʼ������
		initTmpMatAndCinMN(cpu_mat_tmp);

		//����������������������ָ��
		int *solutionSum = new int[1];

		//��ʼ������12���������Ϊһ����12����ʼ��ͼ��
		solutionSum[0] = 12;

		//��������׶���Ҫ������block��thread���������߳̿��ÿ���߳̿����������
		vector<int>blocks = { 16, 256, 256, 256, 256, 512, 512, 1024, 1024, 1024, 1024, 1024, 0 };
		vector<int>threads = { 1, 256, 256, 256, 256, 512, 1024, 1024, 1024, 1024, 1024, 1024, 0 };

		//����ռ��cpu_mat��ÿ���߳�ռ��һ��cpu_mat
		MatrixNode *cpu_mat = new MatrixNode[solutionSum[0]];
		//��ʼ��ÿ��cpu_mat
		for (int i = 0; i < solutionSum[0]; i++)
			cpu_mat[i] = cpu_mat_tmp;

		//����gpu_mat
		MatrixNode *gpu_mat;
		//�洢���մ�
		vector<MatrixNode> result(0);


		//windows���н���ʱ��ͳ��
		time_t  StartTime;
		time_t  EndTime;
		double  TimeUse = 0;
		time(&StartTime);

		//���в��������һ��12��ͼ�Σ���12�㣬ʵ�����ǿ����������
		for (int gpuTimes = 0; gpuTimes < 12; gpuTimes++)
		{
			//��ʾ��ǰ�Ĳ��ţ�cpu_matռ�õ��ڴ�ռ䣬��Ҫ�����������������������߳���
			printf("\n%2d��ʹ���ڴ棺%.4fMB\n    ������%d������߳���Ϊ��%d\n", gpuTimes, (float)(sizeof(MatrixNode)*solutionSum[0] / 1000000.0), solutionSum[0], blocks[gpuTimes] * threads[gpuTimes]);

			//������
			result.clear();
			
			//�ڲ�ѭ������
			int times = 0;

			//ʣ����Ҫ�������������
			int leftSolutionSum = solutionSum[0];

			//�������߳�����
			int threadSum = blocks[gpuTimes] * threads[gpuTimes];

			//��Ҫ������������ᳬ��GPUһ���Կ������߳���������ʹ�ô��еķ������л���
			while (leftSolutionSum != 0)
			{
				//�����Ҫ�������������
				int thisSolutionSum;

				//ʣ����Ҫ����������������ڵ�����ο������߳���������������һ�α���
				if (leftSolutionSum <= threadSum)
				{
					thisSolutionSum = leftSolutionSum;
					leftSolutionSum = 0;
				}
				//ʣ����Ҫ�������������������ο������߳����������������߳���
				else
				{
					thisSolutionSum = threadSum;
					leftSolutionSum -= threadSum;
				}

				//��ӡ��ǰִ�еĲ���������ʹ���Դ������ͱ����������
				printf("    -->����%d���Դ棺%.2fMB��������%d\n", times, (float)(sizeof(MatrixNode)*thisSolutionSum / 1000000.0), thisSolutionSum);

				//gpu_solutionSum��¼����Ҫ�����������
				int *gpu_solutionSum;

				//��gpu�Ϸ����ڴ�
				cudaMalloc((void **)&gpu_solutionSum, sizeof(int) * 1);

				//�����ݴ��ڴ渴�Ƶ��Դ�
				cudaMemcpy(gpu_solutionSum, &thisSolutionSum, sizeof(int) * 1, cudaMemcpyHostToDevice);

				//��gpu�Ϸ����ڴ�
				cudaMalloc((void **)&gpu_mat, sizeof(MatrixNode)*thisSolutionSum);

				//cpu_mat�����ݴ��ڴ渴�Ƶ��Դ棬��Ҫ��Ҫ����ƫ���� times*threadSum
				cudaMemcpy(gpu_mat, cpu_mat + times*threadSum, sizeof(MatrixNode)*thisSolutionSum, cudaMemcpyHostToDevice);


				////////////////////////////////////////////////////////////////////////////////
				// ���Ĵ��룺���п�������������������
				////////////////////////////////////////////////////////////////////////////////
				runCUDA(blocks[gpuTimes], threads[gpuTimes], gpu_mat, gpu_graph, gpu_solutionSum);


				//�����ݴ��Դ渴�Ƶ��ڴ�
				cudaMemcpy(cpu_mat + times*threadSum, gpu_mat, sizeof(MatrixNode)*thisSolutionSum, cudaMemcpyDeviceToHost);

				//�ͷ���ر������Դ�
				cudaFree(gpu_mat);
				cudaFree(gpu_solutionSum);


				//��ʱ�𰸴洢����
				vector<MatrixNode> tmpResult(0);

				//���ú������´�
				refreshSolution(cpu_mat + times*threadSum, thisSolutionSum, tmpResult);

				//�㼯�𰸵�result����
				for (int i = 0; i < (int)tmpResult.size(); i++)
				{
					result.push_back(tmpResult[i]);
				}

				//�ڲ�ѭ������+1
				times++;
			}

			
			//�´���Ҫ�������������Ϊ�˴δ𰸵�����
			solutionSum[0] = (int)result.size();


			//�������һ�Σ�����Ҫ����
			if (gpuTimes != blocks.size() - 1)
			{
				//�ͷ�cpu_mat���ڴ�
				free(cpu_mat);
				//�����µ��ڴ����򣬱���Ҫ����
				cpu_mat = new MatrixNode[solutionSum[0]];
				//��result�ı������ƹ�ȥ
				for (int i = 0; i < solutionSum[0]; i++)
					cpu_mat[i] = result[i];
			}
		}

		//������մ𰸸���
		printf("\n���մ𰸸�����%d\n\n", (int)result.size());

		//windows���н���ʱ��ͳ��
		time(&EndTime);
		TimeUse = difftime(EndTime, StartTime);
		cout << "�ܻ���ʱ��Ϊ��" << (int)TimeUse / 60 << "��" << ((int)TimeUse) % 60 << "��" << endl;

		//�ͷ�cpu_mat���ڴ�
		free(cpu_mat);

		//�������Ҫ���������������result�����

	}

    // cudaDeviceReset causes the driver to clean up all state. While
    // not mandatory in normal operation, it is good practice.  It is also
    // needed to ensure correct operation when the application is being
    // profiled. Calling cudaDeviceReset causes all profile data to be
    // flushed before the application exits
    cudaDeviceReset();
    exit(1);
}



/*
������  ��initTmpMatAndCinMN
�������ܣ���ʼ��cpu_mat_tmpģ�棬����ȡ�û�����ľ��󳤿���Ϣ����m��n��¼��ģ����
���������
cpu_mat_tmp:��Ҫ��ʼ���ľ���ģ��
*/
void initTmpMatAndCinMN(MatrixNode &cpu_mat_tmp)
{
	for (int i = 0; i < 20; i++)
	{
		for (int j = 0; j < 20; j++)
			cpu_mat_tmp.shape[i][j] = 0;
	}
	for (int i = 0; i < 12; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			cpu_mat_tmp.solution[i][j] = 0;
		}
		cpu_mat_tmp.graphUsed[i] = false;
	}
	int m, n;
	while (!CinParameters(m, n))
	{
	}
	//���ó���
	if (m > n)
	{
		cpu_mat_tmp.x = m;
		cpu_mat_tmp.y = n;
	}
	else
	{
		cpu_mat_tmp.x = n;
		cpu_mat_tmp.y = m;
	}
	cpu_mat_tmp.depth = 0;
	cpu_mat_tmp.thisLevelCount = 0;
}


/*
������  ��refreshSolution
�������ܣ���cpu_mat�м�¼����ɢ����һ���ܱ�����������洢��result�У����Ҹ���cpu_mat�м�¼����Ϣ�Ծ���������
���������
cpu_mat     :��Ҫ��ȡ�ľ���ָ�루���飩����Щ���������������һ�������ͼ�ε��������Ϣ
solutionSum :cpu_mat����Ĵ�С
result      :��cpu_mat����������µľ������
*/
void refreshSolution(MatrixNode *cpu_mat, int & solutionSum, vector<MatrixNode>& result)
{
	for (int i = 0; i < solutionSum; i++)
	{
		if (cpu_mat[i].thisLevelCount != 0)
		{//���������������ѹ���µ�������

			//�ҳ����ͼ��û�����Ĳ���
			int unfill_x = -1;
			int unfill_y = -1;
			getFirstUnfill_cpu(cpu_mat[i], unfill_x, unfill_y);

			//����12��ͼ��
			for (int type = 0; type < 12; type++)
			{
				//����8�ֱ���
				for (int transform = 0; transform < 8; transform++)
				{
					//������ڣ���ѹ������
					if (cpu_mat[i].solution[type][transform] != 0)
					{
						//��¼��ʱͼ��
						MatrixNode tmpMat = cpu_mat[i];
						//����Ѿ�ʹ�õ�ͼ��
						tmpMat.graphUsed[type] = true;

						//�������
						tmpMat.thisLevelCount = 0;

						//�����еĽ����������
						for (int tmpType = 0; tmpType < 12; tmpType++)
							for (int tmpTransform = 0; tmpTransform < 8; tmpTransform++)
								tmpMat.solution[tmpType][tmpTransform] = false;
						//��ͼ����䵽������
						fillMatrix_cpu(tmpMat, cpu_graph.graph[type].format[transform], unfill_x, unfill_y, cpu_graph.graph[type].format[transform].fill_x, cpu_graph.graph[type].format[transform].fill_y, cpu_graph.graph[type].c);
						result.push_back(tmpMat);

					}
				}
			}
		}
	}
}


/*
������  ��PrintTitle
�������ܣ���ӡ��Ŀ�������Ϣ
*/
void PrintTitle()
{
	cout << "|---------------------------------------------------------------|\n";
	cout << "|                  �㷨ʵ����7:����ƴͼ����                     |\n";
	cout << "|                                                               |\n";
	cout << "| ����12��ƽ��ͼ�Σ�1,2,3,4,5,6,7,8,9��a,b,c������ͼ��ʾ��      |\n";
	cout << "| ÿ��ͼ�ε���״������ͬ�������Ƕ�����5����С��ͬ����������ɡ� |\n";
	cout << "| ����ͼ����12��ͼ��ƴ�ӳ�һ��6*10�ľ���                      |\n";
	cout << "| ���������12��ͼ��ƴ�ӳɸ������ε�ƴ�ӷ���.                   |\n";
	cout << "|                     _________________                         |\n";
	cout << "|                    |__  6 _____|  |  |                        |\n";
	cout << "|                    |  |__|c ______|1 |                        |\n";
	cout << "|                    |  |  |__|________|                        |\n";
	cout << "|                    |a |__ 5 |   __ 2 |                        |\n";
	cout << "|                    |  |  |  |__|  |__|                        |\n";
	cout << "|                    |__|7 |__|__ 8  __|                        |\n";
	cout << "|                    |________|  |__|  |                        |\n";
	cout << "|                    |  |__ 9  __| b __|                        |\n";
	cout << "|                    |  3  |__|_____|4 |                        |\n";
	cout << "|                    |_____|___________|                        |\n";
	cout << "|                                                               |\n";
	cout << "|                          ������                               |\n";
	cout << "|               ��С�15213068 ë�Ƿ�15213097                   |\n";
	cout << "|               ��˳��15213042 ����־15213065                   |\n";
	cout << "|---------------------------------------------------------------|\n";
	cout << "|                                                               |\n";
	cout << "|                       CUDA����汾                            |\n";
	cout << "|                                                               |\n";
	cout << "|---------------------------------------------------------------|\n";
}


/*
������  ��CinParameters
�������ܣ���ȡ����ĳ����������ͣ����󳤿����������true�����򷵻�false
���������
runtype:������������
m      :������
n      :����߶�
*/
bool CinParameters( int&m, int&n)
{
	cout << "���������Ŀ�ȣ�" << endl;
	cin >> m;
	if (m <= 0)
	{
		cout << "����Ϊ����������������!" << endl;
		return false;
	}
	cout << "���������ĳ��ȣ�" << endl;
	cin >> n;
	if (n <= 0)
	{
		cout << "����Ϊ����������������!" << endl;
		return false;
	}
	if (m*n != 60)
	{
		cout << "m*n������60������������!" << endl;
		return false;
	}
	return true;
}

/*
������  ��initCUDAandShowMessage
�������ܣ���ȡ�Կ���Ϣ����������������Կ�������Ϣ
*/
int initCUDAandShowMessage()
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
	return count;
}


/*
������  ��RotateGraph90
�������ܣ���graph������ת90��
���������
g:��Ҫ��ת��graph
*/
vector<vector<char>> RotateGraph90(vector<vector<char>>& g)
{//��ͼ����ת90��
	vector<vector<char>> newG(g[0].size(), vector<char>(g.size()));
	for (int i = 0; i < newG.size(); i++)
	{
		for (int j = 0; j < newG[i].size(); j++)
			newG[i][j] = g[newG[i].size() - 1 - j][i];
	}
	return newG;
}

/*
������  ��PrintMatrix_cpu
�������ܣ���ӡcpu�˵ľ���
���������
g_mat:��Ҫ��ӡ�ľ���
*/
void PrintMatrix_cpu(MatrixNode &g_mat)
{
	for (int i = 0; i < g_mat.x; i++)
	{
		for (int j = 0; j < g_mat.y; j++)
		{
			printf("%c ", g_mat.shape[i][j]);
		}
		printf("\n");
	}
}

/*
������  ��RotateGraph
�������ܣ���graph������ת0�ȣ�90�ȣ�180�Ȼ�270��
���������
g    :��Ҫ��ת��graph
times:��ת������0�α�ʾ��ת0�ȣ�1�α�ʾ��ת90��
*/
vector<vector<char>> RotateGraph(vector<vector<char>>& g, int times)
{//��ת90�ȶ�Σ�ʵ����ת180�ȣ�270��
	vector<vector<char>> newG = g;
	for (int i = 0; i < times; i++)
		newG = RotateGraph90(newG);
	return newG;
}


/*
������  ��ReverseGraph
�������ܣ���graph�������ҷ�ת
���������
g:��Ҫ��ת��graph
*/
vector<vector<char>> ReverseGraph(vector<vector<char>>& g)
{
	vector<vector<char>> newG = g;
	for (int i = 0; i < newG.size(); i++)
	{
		for (int j = 0; j < newG[i].size(); j++)
			newG[i][j] = g[i][newG[i].size() - 1 - j];
	}
	return newG;
}

/*
������  ��initGraphs
�������ܣ���ʼ��12��ͼ��
���������
g:��Ҫ��ʼ����ͼ�α���
*/
void initGraphs(vector<vector<vector<char>>>&g)
{
	//�洢12��ͼ��
	g =
	{
		{//1
			{ 0, 0, 1 },   //      ||
			{ 0, 0, 1 },   //      ||
			{ 1, 1, 1 }    //  ||||||
		},
		{//2
			{ 1, 1, 1 },   //  ||||||  
			{ 1, 0, 1 }    //  ||  ||
		},
		{//3
			{ 1, 0 },      //  ||
			{ 1, 1 },      //  ||||
			{ 1, 1 }       //  ||||
		},
		{//4
			{ 0, 0, 0, 1 },//        ||
			{ 1, 1, 1, 1 } //  ||||||||
		},
		{//5
			{ 1, 0 },      //  ||
			{ 1, 1 },      //  ||||  
			{ 0, 1 },      //    ||
			{ 0, 1 }       //    ||
		},
		{//6
			{ 1, 1, 1, 1 },//  ||||||||
			{ 0, 1, 0, 0 } //    ||
		},
		{//7
			{ 0, 1, 0 },   //      ||
			{ 0, 1, 0 },   //      ||
			{ 1, 1, 1 }    //  ||||||
		},
		{//8
			{ 0, 1, 0 },   //    ||
			{ 1, 1, 1 },   //  ||||||
			{ 0, 1, 0 }    //    ||
		},
		{//9
			{ 0, 0, 1 },   //      ||
			{ 1, 1, 1 },   //  ||||||
			{ 0, 1, 0 }    //    ||
		},
		{//a
			{ 1 },         //  ||
			{ 1 },         //  ||
			{ 1 },         //  ||
			{ 1 },         //  ||
			{ 1 },         //  ||
		},
		{//b
			{ 0, 0, 1 },   //      ||
			{ 0, 1, 1 },   //    ||||
			{ 1, 1, 0 }    //  ||||
		},
		{//c
			{ 0, 0, 1 },   //      ||
			{ 1, 1, 1 },   //  ||||||
			{ 1, 0, 0 }    //  ||
		}
	};
}
/*
������  ��getFirstFill
�������ܣ����graph�е�һ����ֵ��λ�ã����꣩
���������
graph:����graph
*/
pair<int, int> getFirstFill(vector<vector<char>>& graph)
{
	pair<int, int> fill = { -1, -1 };
	for (int j = 0; j < graph.size(); j++)
	{
		for (int h = 0; h < graph[j].size(); h++)
		{//�ҳ���һ��δ���λ�ã���¼������ֱ������
			if (graph[j][h] != 0)
			{//��Ϊ0������ֵ
				fill = { j, h };
				break;
			}
		}
		if (fill.first != -1) break;
	}
	return fill;
}

/*
������  ��int2char
�������ܣ�������ת��Ϊ��Ŀ��Ӧ��char,0��ʾΪ1��9��ʾΪa
���������
a:��Ҫת��������
*/
char int2char(int a)
{
	if (a < 9) return a + '1';
	else return a - 9 + 'a';
}

/*
������  ��getFirstUnfill
�������ܣ�������mat�е�һ���հ׵ĸ��ӣ����꣩
���������
mat:����mat
*/
void getFirstUnfill_cpu(MatrixNode &g_mat, int &x, int&y)
{
	for (int i = 0; i <g_mat.x; i++)
	{
		for (int j = 0; j < g_mat.y; j++)
		{//�ҳ���һ��δ���λ�ã���¼������ֱ������
			if (g_mat.shape[i][j] == 0)
			{//Ϊ0����û�����
				x = i;
				y = j;
				break;//�ҵ���һ������������
			}
		}
		if (x != -1) break;
	}
}



/*
������  ��initGraphStruct
�������ܣ���ʼ��12��ͼ�εĸ��ֱ��Σ����洢���ṹ��cpu_graph��
���������
cpu_graph:��Ҫ��ʼ����ͼ�α����ṹ��
*/
void initGraphStruct(GraphAll&cpu_graph)
{
	//�洢12��ͼ��
	vector<vector<vector<char>>> graph_vector;
	//�ȳ�ʼ����12��ͼ�εĻ���״̬
	initGraphs(graph_vector);
	//cpu_graph.graph = new GraphFormat[12];
	cpu_graph.graphCount = 12;
	for (int type = 0; type < 12; type++)
	{
		vector<vector<vector<char>>> allGraph(0);//��¼���е�����
		map<vector<vector<char>>, bool> checkDuplicate;//����ظ�
		vector<vector<char>> transGraph; //�仯���ͼƬ
		for (int i = 0; i < 4; i++)
		{//����4���Ƕȵı任
			transGraph = RotateGraph(graph_vector[type], i);
			for (int j = 0; j < 2; j++)
			{
				if (j == 1)//0����ת��1Ϊ��ת
					transGraph = ReverseGraph(transGraph);
				if (checkDuplicate[transGraph] == 0)
				{//û���ظ��Ĳ�ѹ����
					checkDuplicate[transGraph] = 1;
					allGraph.push_back(transGraph);
				}
			}
		}
		cpu_graph.graph[type].formatCount = (int)allGraph.size();
		cpu_graph.graph[type].c = int2char(type);
		for (int k = 0; k < allGraph.size(); k++)
		{
			cpu_graph.graph[type].format[k].x = (int)allGraph[k].size();
			cpu_graph.graph[type].format[k].y = (int)allGraph[k][0].size();
			pair<int, int> fill = getFirstFill(allGraph[k]);
			cpu_graph.graph[type].format[k].fill_x = fill.first;
			cpu_graph.graph[type].format[k].fill_y = fill.second;
			for (int i = 0; i < (int)allGraph[k].size(); i++)
			{
				for (int j = 0; j < (int)allGraph[k][i].size(); j++)
				{
					cpu_graph.graph[type].format[k].shape[i][j] = allGraph[k][i][j];
				}
			}
		}

	}

}

/*
������  ��printAllGraphStruct
�������ܣ���ӡcpu_graph�е�����ͼ��
*/
void printAllGraphStruct()
{
	for (int i = 0; i < cpu_graph.graphCount; i++)
	{
		printf("\nͼ��%d��%d�ֱ��Σ�\n\n", i, cpu_graph.graph[i].formatCount);

		//�������еı���
		for (int j = 0; j < cpu_graph.graph[i].formatCount; j++)
		{
			for (int k = 0; k < cpu_graph.graph[i].format[j].x; k++)
			{
				for (int h = 0; h < cpu_graph.graph[i].format[j].y; h++)
				{//���ÿ�����ص�
					cout << cpu_graph.graph[i].format[j].shape[k][h] << " ";
				}
				cout << endl;
			}
			cout << endl;
		}
	}
}


/*
������  ��canFillMatrix_cpu
�������ܣ�ͼ���ܷ���䵽������
���������
g_mat    :�����ľ���
graph    :��������ͼ��
toFill_x :��������ĵ�һ���ո�λ��x
toFill_y :��������ĵ�һ���ո�λ��y
fill_x   :����ͼ�εĵ�һ���ǿո�λ��x
fill_y   :����ͼ�εĵ�һ���ǿո�λ��y
*/
bool canFillMatrix_cpu(MatrixNode &g_mat, GraphNode &graph, int&toFill_x, int&toFill_y, int&fill_x, int&fill_y)
{//�ж�ͼ���ܷ���䵽�����У����о���ĵ�һ���հ׵�����ΪtoFill


	for (int i = 0; i < graph.x; i++)
	{
		for (int j = 0; j < graph.y; j++)
		{
			if (i + toFill_x - fill_x >= g_mat.x ||
				j + toFill_y - fill_y >= g_mat.y ||
				i + toFill_x - fill_x < 0 ||
				j + toFill_y - fill_y < 0
				)
			{//ͼ������Խ�磬����false
				return false;
			}
			else if (graph.shape[i][j] != 0 && g_mat.shape[i + toFill_x - fill_x][j + toFill_y - fill_y] != 0)
			{//ͼ�����Ĳ��ֲ�Ϊ�գ�����false
				return false;
			}
		}
	}
	return true;
}

/*
������  ��fillMatrix_cpu
�������ܣ���ͼ����䵽������
���������
g_mat    :�����ľ���
graph    :��������ͼ��
toFill_x :��������ĵ�һ���ո�λ��x
toFill_y :��������ĵ�һ���ո�λ��y
fill_x   :����ͼ�εĵ�һ���ǿո�λ��x
fill_y   :����ͼ�εĵ�һ���ǿո�λ��y
c        :����ͼ�ε��ַ�
*/
void fillMatrix_cpu(MatrixNode &g_mat, GraphNode &graph, int&toFill_x, int&toFill_y, int&fill_x, int&fill_y, char&c)
{//�Ծ���������
	for (int i = 0; i < graph.x; i++)
	{
		for (int j = 0; j < graph.y; j++)
		{
			if (graph.shape[i][j] != 0)
			{//��ʱ����Ҫ�󣬽������
				g_mat.shape[i + toFill_x - fill_x][j + toFill_y - fill_y] = c;
			}
		}
	}
}
