#include"guo_http.h"
#include"threads_pool.h"

void *threadpool_threads(void *arg)//线程执行
{
	struct timespec abstime;
	int timeout;
	printf("thread %d is starting\n", (int)pthread_self());
	threadpool_t *guo_pool = (threadpool_t *)arg;
	threadpool_task_t *guo_task;

        while(1)
	{
		timeout = 0;
		condition_lock(&guo_pool->ready);//访问之前先加锁
		guo_pool->notwork_threads++;

		//判断是否有任务以及线程池销毁标志
		while (guo_pool->first_task == NULL && !guo_pool->quit)
		{
			printf("thread %d is waiting \n", (int)pthread_self());

			clock_gettime(CLOCK_REALTIME, &abstime);//获取当前时间，并加上等待时间，设置进程的超时睡眠时间
			abstime.tv_sec += 2;
			int status;
			//该函数会解锁，允许其他线程访问，当被唤醒时，加锁
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
			//取出等待队列最前面的任务，移除任务，并执行任务
			guo_task = guo_pool->first_task;
			guo_pool->first_task = guo_task->next;

			condition_unlock(&guo_pool->ready);

			//执行任务
			guo_task->function(guo_task->arg);

			free(guo_task);
			condition_lock(&guo_pool->ready);
		}


		//退出线程池
		if (guo_pool->quit && guo_pool->first_task == NULL)
		{
			guo_pool->work_threads--;//当前的工作线程为 -1
			if (guo_pool->work_threads == 0)
			{
				condition_signal(&guo_pool->ready);
			}
			condition_unlock(&guo_pool->ready);
			break;
		}
		//超时退出
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


void threadpool_init(threadpool_t *threadpool, int threads)//初始化
{
	condition_init(&threadpool->ready);//初始化条件变量 ready
	threadpool->first_task = NULL;
	threadpool->last_task = NULL;
	threadpool->work_threads = 0;
	threadpool->notwork_threads = 0;
	threadpool->max_threads = threads;
	threadpool->quit = 0;
}
int threadpool_add_task(threadpool_t *threadpool, void *(*function)(void *args), void *argument)
{
	condition_lock(&threadpool->ready);//线程池的状态被共享，操作的话需要加锁，防止线程安全问题
	threadpool_task_t *newtask = (threadpool_task_t *)malloc(sizeof(threadpool_task_t));
	newtask->function = function;//任务函数
	newtask->arg = argument;//任务函数参数
	newtask->next = NULL;//任务队列

//	condition_lock(&threadpool->ready);//线程池的状态被共享，操作的话需要加锁，防止线程安全问题

	if (threadpool->first_task == NULL)
	{
		threadpool->first_task = newtask;
	}
	else
	{
		threadpool->last_task->next = newtask;//否则将新任务置于任务队列的最后边
	}
	threadpool->last_task = newtask;//任务队列尾节点更新

	if (threadpool->notwork_threads > 0)//是否有多余的空闲线程，如果有直接唤醒去执行任务，否则就创建新线程去执行任务
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

	condition_unlock(&threadpool->ready);//解锁
	return 0;
}

int threadpool_destroy(threadpool_t *threadpool)
{
	if (threadpool->quit)//如果销毁直接返回
	{
		return -1;
	}
	condition_lock(&threadpool->ready);//加锁,日常操作

	threadpool->quit = 1;//销毁标志

	if (threadpool->work_threads > 0)
	{
		if (threadpool->notwork_threads > 0)
		{
			condition_broadcast(&threadpool->ready);//如果有空闲线程，唤醒它
		}
		while (threadpool->work_threads)//等待工作线程结束任务
		{
			condition_wait(&threadpool->ready);
		}
	}
	condition_unlock(&threadpool->ready);
	condition_destroy(&threadpool->ready);
}

