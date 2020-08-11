#include"guo_http.h"
#include"threads_pool.h"

void *threadpool_threads(void *arg)//�߳�ִ��
{
	struct timespec abstime;
	int timeout;
	printf("thread %d is starting\n", (int)pthread_self());
	threadpool_t *guo_pool = (threadpool_t *)arg;
	threadpool_task_t *guo_task;

        while(1)
	{
		timeout = 0;
		condition_lock(&guo_pool->ready);//����֮ǰ�ȼ���
		guo_pool->notwork_threads++;

		//�ж��Ƿ��������Լ��̳߳����ٱ�־
		while (guo_pool->first_task == NULL && !guo_pool->quit)
		{
			printf("thread %d is waiting \n", (int)pthread_self());

			clock_gettime(CLOCK_REALTIME, &abstime);//��ȡ��ǰʱ�䣬�����ϵȴ�ʱ�䣬���ý��̵ĳ�ʱ˯��ʱ��
			abstime.tv_sec += 2;
			int status;
			//�ú�������������������̷߳��ʣ���������ʱ������
			status = condition_timedwait(&guo_pool->ready, &abstime);
			if (status == ETIMEDOUT)
			{
				timeout = 1;
				break;
			}
		}
		guo_pool->notwork_threads --;

		if (guo_pool->first_task != NULL)
		{
			//ȡ���ȴ�������ǰ��������Ƴ����񣬲�ִ������
			guo_task = guo_pool->first_task;
			guo_pool->first_task = guo_task->next;

			condition_unlock(&guo_pool->ready);

			//ִ������
			guo_task->function(guo_task->arg);

			free(guo_task);
			condition_lock(&guo_pool->ready);
		}


		//�˳��̳߳�
		if (guo_pool->quit && guo_pool->first_task == NULL)
		{
			guo_pool->work_threads--;//��ǰ�Ĺ����߳�Ϊ -1
			if (guo_pool->work_threads == 0)
			{
				condition_signal(&guo_pool->ready);
			}
			condition_unlock(&guo_pool->ready);
			break;
		}
		//��ʱ�˳�
		if (timeout == 1)
		{
			guo_pool->work_threads--;
			condition_unlock(&guo_pool->ready);
			break;
		}
		condition_unlock(&guo_pool->ready);
	}
	printf("thread %d is exiting \n", (int)pthread_self());
	return NULL;
}


void threadpool_init(threadpool_t *threadpool, int threads)//��ʼ��
{
	condition_init(&threadpool->ready);//��ʼ���������� ready
	threadpool->first_task = NULL;
	threadpool->last_task = NULL;
	threadpool->work_threads = 0;
	threadpool->notwork_threads = 0;
	threadpool->max_threads = threads;
	threadpool->quit = 0;
}
int threadpool_add_task(threadpool_t *threadpool, void *(*function)(void *args), void *argument)
{
	condition_lock(&threadpool->ready);//�̳߳ص�״̬�����������Ļ���Ҫ��������ֹ�̰߳�ȫ����
	threadpool_task_t *newtask = (threadpool_task_t *)malloc(sizeof(threadpool_task_t));
	newtask->function = function;//������
	newtask->arg = argument;//����������
	newtask->next = NULL;//�������

//	condition_lock(&threadpool->ready);//�̳߳ص�״̬�����������Ļ���Ҫ��������ֹ�̰߳�ȫ����

	if (threadpool->first_task == NULL)
	{
		threadpool->first_task = newtask;
	}
	else
	{
		threadpool->last_task->next = newtask;//��������������������е�����
	}
	threadpool->last_task = newtask;//�������β�ڵ����

	if (threadpool->notwork_threads > 0)//�Ƿ��ж���Ŀ����̣߳������ֱ�ӻ���ȥִ�����񣬷���ʹ������߳�ȥִ������
	{
		condition_signal(&threadpool->ready);
	}
	else if (threadpool->work_threads < threadpool->max_threads)
	{
		pthread_t newpthread;
		pthread_create(&newpthread,NULL, threadpool_threads, (void *)threadpool);
		pthread_detach(newpthread);
		threadpool->work_threads++;
	}

	condition_unlock(&threadpool->ready);//����
	return 0;
}

int threadpool_destroy(threadpool_t *threadpool)
{
	if (threadpool->quit)//�������ֱ�ӷ���
	{
		return -1;
	}
	condition_lock(&threadpool->ready);//����,�ճ�����

	threadpool->quit = 1;//���ٱ�־

	if (threadpool->work_threads > 0)
	{
		if (threadpool->notwork_threads > 0)
		{
			condition_broadcast(&threadpool->ready);//����п����̣߳�������
		}
		while (threadpool->work_threads)//�ȴ������߳̽�������
		{
			condition_wait(&threadpool->ready);
		}
	}
	condition_unlock(&threadpool->ready);
	condition_destroy(&threadpool->ready);
}

