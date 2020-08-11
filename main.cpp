#include"guo_http.h"
#include"threads_pool.h"

#define LISTENQ 10240 //定义最大监听数
#define OPEN_MAX 5000 //最大事件


//对一个对端已经关闭的socket调用两次write, 第二次将会生成SIGPIPE信号, 该信号默认结束进程。所以一般选择将其忽略
void handle_for_sigpipe()//安全屏蔽掉SIGPIPE
{
	struct sigaction sa;
	memset(&sa, '\0', sizeof(sa));
	sa.sa_handler = SIG_IGN;//忽略该信号
	sa.sa_flags = 0;
	if (sigaction(SIGPIPE, &sa, NULL))
		return;
}

int main(void)
{
	//在linux下写socket的程序的时候，如果尝试send到一个disconnected socket上，就会让底层抛出一个SIGPIPE信号,而该信号的默认处理程序是终止程序
	//参照TCP四次分手，当其中一段的socket已经关闭了之后，另一方任然在发送数据，也就是向缓存区 write 数据，但是对方已经关闭read缓存区，
	//所以发送端会收到RST报文，当发送端再次调用write时，就会触发SIGPIPE信号，促使进程关闭。
	//这个信号的缺省处理方法是退出进程，大多数时候这都不是我们期望的。因此我们需要重载这个信号的处理方法。
	handle_for_sigpipe();//安全屏蔽掉SIGPIPE

	u_short port = 12345 ;	// 使用随机端口 unsigned short int  取值范围 0 - 65535
	int server_sock_fd = -1;
	int client_sock_fd = -1;
	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);
	struct epoll_event tep;
	struct epoll_event events[OPEN_MAX];

	threadpool_t *Guo_pool = new threadpool;
	threadpool_init(Guo_pool,30);

	 //pthread_t newthread;
	// 启动服务,监听,等待连接5
	server_sock_fd = startup(&port);
	printf("httpd running on port %d\n", port);

	int flag = fcntl(server_sock_fd, F_GETFL, 0); //设置套接字为非阻塞模式
	if (flag == -1) return -1;
	flag |= O_NONBLOCK;
	if (fcntl(server_sock_fd, F_SETFL, flag) == -1)
		perror("Set non block failed!");


	ssize_t epoll_fd = epoll_create(LISTENQ);
	if (epoll_fd < 0)
	{
		perror("epoll_init failed");
	}

	tep.events = EPOLLIN | EPOLLET; //定义事件 ET边沿触发模式
	tep.data.fd = server_sock_fd;
	ssize_t  res = epoll_ctl(epoll_fd,EPOLL_CTL_ADD, server_sock_fd,&tep);
	if (res == -1)
	{
		perror("epoll_ctl failed");
	}


	while (1)
	{
		ssize_t nready = epoll_wait(epoll_fd,events,OPEN_MAX,10);
	//	/*参数1：epoll_create()的返回值
	//		  参数2：ep 是 epoll_event 类型数组
	//		  参数3：类型数组的容量
	//		  参数4；time参数 -1 阻塞监听
	//		  返回整型值。	
		if (nready < 0)
		{
			perror("epoll_wait failed");
		}
		if (-1 == nready)
		{
			if (errno != EINTR)
			{
				return -1;
			}
		}

		for (size_t i = 0; i < nready; i++)
		{
			if (events[i].data.fd == server_sock_fd)//监听是否有新连接
			{
				// 接受请求
				client_sock_fd = accept(server_sock_fd, (struct sockaddr *)&client_addr, &client_addr_len);
				if (client_sock_fd == -1)
				{
					error_die("accept");
				}

				tep.events = EPOLLIN;
				tep.data.fd = client_sock_fd;
				res = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_sock_fd, &tep);//将新连接添加到红黑树上
				if(res == -1)
					perror("epoll_ctl error");
			}else{
				threadpool_add_task(Guo_pool,accept_request,(void *)events[i].data.fd);
			}
		}
	}
	// 关闭
	close(server_sock_fd);
	threadpool_destroy(Guo_pool);
	return(0);
}
