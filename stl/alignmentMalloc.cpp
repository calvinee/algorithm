
//#include<string>
//#include <iomanip>
#include<vector>
#include <algorithm>
//#include<stack>
#include<set>
#include<queue>
#include<map>
//#include<unordered_set>
#include<unordered_map>
//#include <sstream>
//#include "func.h"
//#include <list>
#include<stdio.h>
#include<iostream>
#include<string>
#include<memory.h>
#include<limits.h>
#include<bitset>
//#include "siukwanAlloctor.h"
using namespace std;
//��������ַ��ָ��
void * aligned_malloc(int size, int alignment)
{
	void* ptr = malloc(size + alignment);

	if (ptr)
	{
		//ע���Ǽ���alignment����ȡ�룬��stl�еļ�����ȡ����һ����stl�е�����ȡ���Ǽ��ϣ�alignment-1��
		//�������alignment����ȷ��alignedǰ��ĵ�ַ�ռ���ڿհײ���
		void* aligned = (void*)(((long)ptr + alignment) & ~(alignment - 1));
		((void**)aligned)[-1] = ptr;//�൱��*(aligned-1)����˼�������﷨������Ҫǿ��ת�����ͣ���˼�ǰ�ptr�����aligned-1ָ��ĵ�ַ�Ŀռ���

		return aligned;
	}
	else
		return NULL;
}
//�ͷź���aligned_free
void aligned_free(void *paligned)
{
	//ɾ����Ӧ�ĵ�ַ�ռ�
	free(((void**)paligned)[-1]);
}
int main()
{

	int *p = (int*)aligned_malloc(sizeof(int), 8);
	cout << p << endl;
	aligned_free(p);
	return 0;
}