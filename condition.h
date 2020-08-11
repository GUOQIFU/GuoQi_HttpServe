#pragma once
#ifndef CONDITION_H
#define CONDITION_H
#include<pthread.h>




//���������������̼߳乲���ȫ�ֱ�������ͬ����һ�ֻ��ƣ�
//��Ҫ��������������һ���̵߳ȴ�"������������������"��������һ���߳�ʹ"��������"���������������źţ���
//��װһ��������������������Ϊ״̬
typedef struct condition
{
	pthread_mutex_t pmutex;//�̻߳�����
	pthread_cond_t pcond;//��������
}condition_t;

//��״̬�Ĳ�������
int condition_init(condition_t *cond);//��ʼ��
int condition_lock(condition_t *cond);//pthread_mutex_lock ����
int condition_unlock(condition_t *cond);//pthread_mutex_unlock ����
int condition_wait(condition_t *cond);// pthread_cond_wait �ȴ�
int condition_timedwait(condition_t *cond, const struct timespec *abstime);//  pthread_cond_timedwait �趨�ȴ�ʱ��
int condition_signal(condition_t* cond);//pthread_cond_signal ����һ��˯���߳�
int condition_broadcast(condition_t *cond);// pthread_cond_broadcast ��������˯���߳�
int condition_destroy(condition_t *cond);//pthread_mutex_destroy �����߳�

#endif


