#include"guo_http.h"
#include"threads_pool.h"

#define LISTENQ 10240 //������������
#define OPEN_MAX 5000 //����¼�


//��һ���Զ��Ѿ��رյ�socket��������write, �ڶ��ν�������SIGPIPE�ź�, ���ź�Ĭ�Ͻ������̡�����һ��ѡ�������
void handle_for_sigpipe()//��ȫ���ε�SIGPIPE
{
	struct sigaction sa;
	memset(&sa, '\0', sizeof(sa));
	sa.sa_handler = SIG_IGN;//���Ը��ź�
	sa.sa_flags = 0;
	if (sigaction(SIGPIPE, &sa, NULL))
		return;
}

int main(void)
{
	//��linux��дsocket�ĳ����ʱ���������send��һ��disconnected socket�ϣ��ͻ��õײ��׳�һ��SIGPIPE�ź�,�����źŵ�Ĭ�ϴ����������ֹ����
	//����TCP�Ĵη��֣�������һ�ε�socket�Ѿ��ر���֮����һ����Ȼ�ڷ������ݣ�Ҳ�����򻺴��� write ���ݣ����ǶԷ��Ѿ��ر�read��������
	//���Է��Ͷ˻��յ�RST���ģ������Ͷ��ٴε���writeʱ���ͻᴥ��SIGPIPE�źţ���ʹ���̹رա�
	//����źŵ�ȱʡ���������˳����̣������ʱ���ⶼ�������������ġ����������Ҫ��������źŵĴ�������
	handle_for_sigpipe();//��ȫ���ε�SIGPIPE

	u_short port = 12345 ;	// ʹ������˿� unsigned short int  ȡֵ��Χ 0 - 65535
	int server_sock_fd = -1;
	int client_sock_fd = -1;
	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);
	struct epoll_event tep;
	struct epoll_event events[OPEN_MAX];

	threadpool_t *Guo_pool = new threadpool;
	threadpool_init(Guo_pool,30);

	 //pthread_t newthread;
	// ��������,����,�ȴ�����5
	server_sock_fd = startup(&port);
	printf("httpd running on port %d\n", port);

	int flag = fcntl(server_sock_fd, F_GETFL, 0); //�����׽���Ϊ������ģʽ
	if (flag == -1) return -1;
	flag |= O_NONBLOCK;
	if (fcntl(server_sock_fd, F_SETFL, flag) == -1)
		perror("Set non block failed!");


	ssize_t epoll_fd = epoll_create(LISTENQ);
	if (epoll_fd < 0)
	{
		perror("epoll_init failed");
	}

	tep.events = EPOLLIN | EPOLLET; //�����¼� ET���ش���ģʽ
	tep.data.fd = server_sock_fd;
	ssize_t  res = epoll_ctl(epoll_fd,EPOLL_CTL_ADD, server_sock_fd,&tep);
	if (res == -1)
	{
		perror("epoll_ctl failed");
	}


	while (1)
	{
		ssize_t nready = epoll_wait(epoll_fd,events,OPEN_MAX,10);
	//	/*����1��epoll_create()�ķ���ֵ
	//		  ����2��ep �� epoll_event ��������
	//		  ����3���������������
	//		  ����4��time���� -1 ��������
	//		  ��������ֵ��	
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
			if (events[i].data.fd == server_sock_fd)//�����Ƿ���������
			{
				// ��������
				client_sock_fd = accept(server_sock_fd, (struct sockaddr *)&client_addr, &client_addr_len);
				if (client_sock_fd == -1)
				{
					error_die("accept");
				}

				tep.events = EPOLLIN;
				tep.data.fd = client_sock_fd;
				res = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_sock_fd, &tep);//����������ӵ��������
				if(res == -1)
					perror("epoll_ctl error");
			}else{
				threadpool_add_task(Guo_pool,accept_request,(void *)events[i].data.fd);
			}
		}
	}
	// �ر�
	close(server_sock_fd);
	threadpool_destroy(Guo_pool);
	return(0);
}
