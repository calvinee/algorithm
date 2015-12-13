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

#ifndef MAX
#define MAX(a,b) (a > b ? a : b)
#endif

////////////////////////////////////////////////////////////////////////////////
// declaration, forward
////////////////////////////////////////////////////////////////////////////////
/*��¼����ͼ�εĳ������״*/
struct GraphNode
{
	char shape[5][5];      //ͼ�ε���״����ΪboolҲռ����1B������ʹ��char����bool������
	int x;                //ͼ�εĸ߶�
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


///////////////////////////////////////////////////////////////////////////////
//! Simple test kernel for device functionality
//! @param g_graph  memory to process (in and out)
///////////////////////////////////////////////////////////////////////////////
__global__ void gpuTestSiukwan(GraphAll *const g_graph)
{
	// write data to global memory
	const unsigned int tid = threadIdx.x;
	for (int k = 0; k < g_graph->graph[tid].formatCount; k++)
	{
		for (int i = 0; i < g_graph->graph[tid].format[k].x; i++)
		{
			for (int j = 0; j < g_graph->graph[tid].format[k].y; j++)
			{
				//g_graph->graph[tid].format[k].shape[i][j] += '0';
			}
		}
	}
}

extern "C" bool
SiukwanTest(GraphAll *const g_graph)
{
	gpuTestSiukwan << < 1, 12 >> >(g_graph);
	return true;
}




/*
������  ��getFirstUnfill
�������ܣ�������mat�е�һ���հ׵ĸ��ӣ����꣩
���������
mat:����mat
*/
__device__ void getFirstUnfill(MatrixNode &g_mat, int &x, int&y)
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
������  ��getFirstFill
�������ܣ����graph�е�һ����ֵ��λ�ã����꣩
���������
graph:����graph
*/
__device__ void getFirstFill(GraphNode &graph, int &x, int&y)
{
	for (int j = 0; j < graph.x; j++)
	{
		for (int h = 0; h < graph.y; h++)
		{//�ҳ���һ��δ���λ�ã���¼������ֱ������
			if (graph.shape[j][h] != 0)
			{//��Ϊ0������ֵ
				x = j;
				y = h;
				break;
			}
		}
		if (x != -1) break;
	}
}

/*
������  ��canFillMatrix
�������ܣ�ͼ���ܷ���䵽������
���������
g_mat    :�����ľ���
graph    :��������ͼ��
toFill_x :��������ĵ�һ���ո�λ��x
toFill_y :��������ĵ�һ���ո�λ��y
fill_x   :����ͼ�εĵ�һ���ǿո�λ��x
fill_y   :����ͼ�εĵ�һ���ǿո�λ��y
*/
__device__ bool canFillMatrix(MatrixNode &g_mat, GraphNode &graph, int&toFill_x, int&toFill_y,  int&fill_x, int&fill_y)
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



__global__ void dfsCUDA_LastVersion(MatrixNode *const g_mat, GraphAll *const g_graph, int*solutionSum)
{
	// write data to global memory
	const unsigned int tid = (blockIdx.x*blockDim.x) + threadIdx.x;
	if (tid >= solutionSum[0]) return;

	//��ȡδ���ĵ�һ���ո�
	int unfill_x = -1, unfill_y = -1;
	getFirstUnfill(g_mat[tid], unfill_x, unfill_y);

	//һ��ʼֻ��12���̣߳��߳�id��ͼ��id
	if (g_mat[tid].depth == 0)
	{
		for (int j = 0; j < g_graph->graph[tid].formatCount; j++)
		{
			if (canFillMatrix(g_mat[tid], g_graph->graph[tid].format[j], unfill_x, unfill_y, g_graph->graph[tid].format[j].fill_x, g_graph->graph[tid].format[j].fill_y))
			{//��¼i��j����¼��һ����Щ�𰸿���
				//�ò��������
				g_mat[tid].thisLevelCount++;
				g_mat[tid].solution[tid][j] = true;//��¼����i��j����
			}
		}
	}
	else
	{
		//�����������12��ͼ��
		for (int i = 0; i < 12; i++)
		{
			if (g_mat[tid].graphUsed[i])
				continue;
			for (int j = 0; j < g_graph->graph[i].formatCount; j++)
			{
				if (canFillMatrix(g_mat[tid], g_graph->graph[i].format[j], unfill_x, unfill_y, g_graph->graph[i].format[j].fill_x, g_graph->graph[i].format[j].fill_y))
				{//��¼i��j����¼��һ����Щ�𰸿���
					//�ò��������
					g_mat[tid].thisLevelCount++;
					g_mat[tid].solution[i][j] = true;//��¼����i��j����
				}
			}
		}
	}
	//����++
	g_mat[tid].depth++;
}

//�ӿں���
extern "C" void runCUDA(int blocks, int threads, MatrixNode *const g_mat, GraphAll *const g_graph, int *solutionSum)
{
	dfsCUDA_LastVersion << < blocks, threads >> >(g_mat, g_graph, solutionSum);
}

