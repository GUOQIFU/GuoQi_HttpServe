#pragma once
#ifndef THREADS_POOL_H
#define THREADS_POOL_H
#include<pthread.h>
#include"guo_http.h"
#include"condition.h"

typedef struct task
{
	void *(*function)(void *args);//����ָ�룬��Ҫִ�е�����
	void *arg;
	struct task *next;//�������
}threadpool_task_t;

typedef struct threadpool
{
	condition_t ready;//״̬��
	//threadpool_task_t *queue;//�������
	threadpool_task_t *first_task;//��������е�һ������
	threadpool_task_t *last_task;//���һ������
	int max_threads;//����߳�����
	int work_threads;//��ǰ�����߳�
	int notwork_threads;//�����߳�
	int quit;//�Ƿ��˳���־
}threadpool_t;

void threadpool_init(threadpool_t *threadpool,int threads);//��ʼ��
int threadpool_add_task(threadpool_t *threadpool, void *(*function)(void *args), void *args);//�������
int threadpool_destroy(threadpool_t *threadpool);//�����߳�
void *threadpool_threads(void *arg);//�߳�ִ��

#endif // !THREADS_POOL_H
