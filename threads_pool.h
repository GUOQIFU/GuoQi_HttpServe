#pragma once
#ifndef THREADS_POOL_H
#define THREADS_POOL_H
#include<pthread.h>
#include"guo_http.h"
#include"condition.h"

typedef struct task
{
	void *(*function)(void *args);//函数指针，需要执行的任务
	void *arg;
	struct task *next;//任务队列
}threadpool_task_t;

typedef struct threadpool
{
	condition_t ready;//状态量
	//threadpool_task_t *queue;//任务队列
	threadpool_task_t *first_task;//任务队列中第一个任务
	threadpool_task_t *last_task;//最后一个任务
	int max_threads;//最大线程数量
	int work_threads;//当前工作线程
	int notwork_threads;//空闲线程
	int quit;//是否退出标志
}threadpool_t;

void threadpool_init(threadpool_t *threadpool,int threads);//初始化
int threadpool_add_task(threadpool_t *threadpool, void *(*function)(void *args), void *args);//添加任务
int threadpool_destroy(threadpool_t *threadpool);//销毁线程
void *threadpool_threads(void *arg);//线程执行

#endif // !THREADS_POOL_H
