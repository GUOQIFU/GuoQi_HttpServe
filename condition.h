#pragma once
#ifndef CONDITION_H
#define CONDITION_H
#include<pthread.h>




//条件变量是利用线程间共享的全局变量进行同步的一种机制，
//主要包括两个动作：一个线程等待"条件变量的条件成立"而挂起；另一个线程使"条件成立"（给出条件成立信号）。
//封装一个互斥量和条件变量作为状态
typedef struct condition
{
	pthread_mutex_t pmutex;//线程互斥量
	pthread_cond_t pcond;//条件变量
}condition_t;

//对状态的操作函数
int condition_init(condition_t *cond);//初始化
int condition_lock(condition_t *cond);//pthread_mutex_lock 加锁
int condition_unlock(condition_t *cond);//pthread_mutex_unlock 解锁
int condition_wait(condition_t *cond);// pthread_cond_wait 等待
int condition_timedwait(condition_t *cond, const struct timespec *abstime);//  pthread_cond_timedwait 设定等待时间
int condition_signal(condition_t* cond);//pthread_cond_signal 唤醒一个睡眠线程
int condition_broadcast(condition_t *cond);// pthread_cond_broadcast 唤醒所有睡眠线程
int condition_destroy(condition_t *cond);//pthread_mutex_destroy 销毁线程

#endif


