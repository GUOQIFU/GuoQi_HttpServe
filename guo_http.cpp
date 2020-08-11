#include"guo_http.h"

#define ISspace(x) isspace((int)(x))
#define SERVER_STRING "Server: jdbhttpd/0.1.0\r\n"


void *accept_request(void *client)
{
	char	buf[1024];
	int		numchars;
	char	method[255];	// ����(��������)
	char	url[255];		// �������ԴURL
	char	path[512];		// ������Դ�ı���·��
	size_t i, j;
	struct stat st;//״̬��Ϣ
	int cgi = 0;/* Ϊ��ʱ,��ʾ�����������һ��CGI���� */
	char *query_string = NULL; 

	// 1. �ӿͻ��������������ݰ��ж�ȡһ�С����� buf[] �е��ֽ���
	numchars = get_line((long)client, buf, sizeof(buf)); 
	// 2. �Ӷ�ȡ����������ȡ����������
	i = 0; j = 0;
	while (!ISspace(buf[j]) && (i < sizeof(method) - 1))
		//ISspace(int c); �ж��ַ�c�Ƿ�Ϊ�հ׷����Ƿ��ط�0ֵ���񷵻�0 
	{
		method[i] = buf[j];// ������ȡ����Ϣ
		i++; j++;
	}
	method[i] = '\0';
	// 3. �ж���������
	if (strcasecmp(method, "GET") && strcasecmp(method, "POST"))//��֧�������ַ�����
    //strcasecmp��const char *s1, const char *s2);�ж��ַ����Ƿ���ȣ������Դ�Сд���������ͬ�򷵻�0��
    //s1 ���ȴ���s2 �����򷵻ش���0 ��ֵ��s1 ������С��s2 �����򷵻�С��0 ��ֵ��
	{
		unimplemented((long)client);	// ��֧����������,��ͻ��˷���501
		return	NULL;
	}

	if (strcasecmp(method, "POST") == 0) 
	{
		cgi = 1;	// �����POST����, ֱ�ӽ�  cgi��־λ λ�� -> ����cgi����
	}

	// 4. ��ȡ�������ԴURL��GET������URL������Ϣ��������Ҫ��ȡ�����Դ
	//http���������и�ʽ��GET /mix/76.html?name=kelvin&password=123456 HTTP/1.1
	//����ķ�����GET��
	//�����URL(/mix/76.html)
	//��̬��Ϣ ?name=kelvin&password=123456
	//Э�����ͼ��汾(HTTP/1.1)
	i = 0;
	while (ISspace(buf[j]) && (j < sizeof(buf))) {
		j++;	// ��λ��һ���ǿո�λ��
	}
	while (!ISspace(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf)))//url = mix/76.html?name=kelvin&password=123456
	{
		url[i] = buf[j];
		i++; j++;
	}
	url[i] = '\0';

	if (strcasecmp(method, "GET") == 0)
	{
		query_string = url;	// ΪGET����,��ѯ���Ϊurl

		// �����ѯ����к���'?',��ѯ���Ϊ'?'�ַ����沿��
		while ((*query_string != '?') && (*query_string != '\0')) {
			query_string++;
		}
		if (*query_string == '?')//url�����ʺţ�˵���Ƕ�̬��������cgi = 1, �����ֱ��ִ�о�̬ҳ��ĳ���
		{
			cgi = 1;
			*query_string = '\0';	// ��'?'���ض�,ǰ���Ϊurl
			query_string++;
		}
	}

	// 5. ���ɱ���·��
	sprintf(path, "htdocs%s", url);//sprintf()�����ǽ��������뵽�ַ����У������Ľ��Ϊ  path =��htdocs + url��;
	if (path[strlen(path) - 1] == '/') {
		strcat(path, "index.html");	//�������url����'/'��β,��ָ��Ϊ��Ŀ¼�µ�index.html�ļ�
	}
	// 6. �ж�������Դ��״̬
	if (stat(path, &st) == -1) //��ȡ�ļ���Ϣ���ɹ�Ϊ 0��ʧ��Ϊ -1��
	{
		// ��client�ж�ȡ,ֱ��������������(��ʼ��startline���ײ�header֮����)
		while ((numchars > 0) && strcmp("\n", buf)) //strcmp��const char *s1,const char *s2��; �ַ����Ƚϣ���ȷ���0��s1 > s2 ;������ֵ�����򷵻ظ�ֵ
		{
			numchars = get_line((long)client, buf, sizeof(buf)); //???????    ��ʲô����
		}
		not_found((long)client);	// 501
	}
	else
	{
		// ������Ǹ�Ŀ¼,ָ��Ϊ��Ŀ¼�µ�index.html�ļ�
		if ((st.st_mode & S_IFMT) == S_IFDIR) {
			strcat(path, "/index.html");
		}
		// �������һ����ִ���ļ�,��ΪCGI����
		if ((st.st_mode & S_IXUSR) ||
			(st.st_mode & S_IXGRP) ||
			(st.st_mode & S_IXOTH)) {
			cgi = 1;
		}
		// �ж���ִ��һ��CGI�����Ƿ���һ���ļ����ݸ��ͻ���
		if (!cgi) {
			serve_file((long)client, path);
		}
		else {
			execute_cgi((long)client, path, method, query_string);
		}
	}

	close((long)client);
	return NULL;
}

void bad_request(int client)
{
	char buf[1024];
	// ��client��д400״̬
	sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n");//sprintf();���ַ�д���ض��ļĴ�����
	send(client, buf, sizeof(buf), 0);
	sprintf(buf, "Content-type: text/html\r\n");
	send(client, buf, sizeof(buf), 0);
	sprintf(buf, "\r\n");
	send(client, buf, sizeof(buf), 0);
	sprintf(buf, "<P>Your browser sent a bad request, ");
	send(client, buf, sizeof(buf), 0);
	sprintf(buf, "such as a POST without a Content-Length.\r\n");
	send(client, buf, sizeof(buf), 0);
}

/**********************************************************************/
/* Put the entire contents of a file out on a socket.  This function
 * is named after the UNIX "cat" command, because it might have been
 * easier just to do something like pipe, fork, and exec("cat").
 * Parameters: the client socket descriptor
 *             FILE pointer for the file to cat */
 /**********************************************************************/
void cat(int client, FILE *resource)
{
	char buf[1024];
	// ��resource�ж�ȡһ��
	fgets(buf, sizeof(buf), resource);
	// ���ļ��е����ݷ��͵��ͻ���
	while (!feof(resource))
	{
		send(client, buf, strlen(buf), 0);
		fgets(buf, sizeof(buf), resource);
	}
}


void cannot_execute(int client)
{
	char buf[1024];
	// ��ͻ��˻�д500״̬,������ִ��CGI����
	sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "Content-type: text/html\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "<P>Error prohibited CGI execution.\r\n");
	send(client, buf, strlen(buf), 0);
}


void error_die(const char *sc)
{
	perror(sc);	// ���������Ϣ
	exit(1);	// �˳�����
}

/*
 * @brief	ִ��һ��CGI�ű�.������Ҫ�����ʵ��Ļ�������.
 CGIȫ���� Common Gate Intergace ���������ϣ�CGI��һ�γ�����������Server�ϣ��ṩͬ�ͻ��� Htmlҳ��Ľӿڡ�
 * @prama	client	�ͻ���socket�ļ�������
 * @prama	path	CGI �ű�·��
 * @prama	method	��������
 * @prama	query_string	��ѯ���
*/
void execute_cgi(int client, const char *path,
	const char *method, const char *query_string)
{
	char	buf[1024];
	int		cgi_output[2];	// CGI��������ܵ�
	int		cgi_input[2];	// CGI��������ܵ�

	pid_t	pid;       //����     
	int		status;
	int		i;
	char	c;
	int		numchars = 1;
	int		content_length = -1;

	buf[0] = 'A'; buf[1] = '\0';//????????

	if (strcasecmp(method, "GET") == 0) //�ַ����ȶԺ���
	{
		// ��client�ж�ȡ,ֱ��������������
		while ((numchars > 0) && strcmp("\n", buf))
		{ 
			numchars = get_line(client, buf, sizeof(buf));//
		}
	}
	else    /* POST */
	{
		// ��ȡ��������ĳ���
		numchars = get_line(client, buf, sizeof(buf));
		while ((numchars > 0) && strcmp("\n", buf))
		{
			buf[15] = '\0';
			if (strcasecmp(buf, "Content-Length:") == 0) {
				content_length = atoi(&(buf[16]));
			}
			numchars = get_line(client, buf, sizeof(buf));
		}
		if (content_length == -1) {
			bad_request(client);
			return;
		}
	}
  // ���϶�ȡ��Ӧ���ַ��������õĸ��Եķ���


	// ��client��д200 OK
	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	send(client, buf, strlen(buf), 0);

	// �������������ܵ�
	if (pipe(cgi_output) < 0) {
		cannot_execute(client);
		return;
	}
	if (pipe(cgi_input) < 0) {
		cannot_execute(client);
		return;
	}

	// �����ӽ���,ȥִ��CGI����
	if ((pid = fork()) < 0) {
		cannot_execute(client);
		return;
	}
	if (pid == 0)  /* child: CGI script */
	{
		char meth_env[255];
		char query_env[255];
		char length_env[255];

		//��׵����ض���output�ܵ���д��� stdout 1
		dup2(cgi_output[1], 1);	//����cgi_output[1](����)���ӽ��̵ı�׼���

		//��׵�����ض���input�ܵ���д��� stdin 0
		dup2(cgi_input[0], 0);	//����cgi_input[0](д��)���ӽ��̵ı�׼����

		close(cgi_output[0]);	//�رն����ļ�������
		close(cgi_input[1]);

		sprintf(meth_env, "REQUEST_METHOD=%s", method);
		putenv(meth_env);	// ���һ����������

		if (strcasecmp(method, "GET") == 0) {
			sprintf(query_env, "QUERY_STRING=%s", query_string);
			putenv(query_env);
		}
		else {   /* POST */
			sprintf(length_env, "CONTENT_LENGTH=%d", content_length);
			putenv(length_env);
		}
		// ִ��CGI����
		//����exec������ִ��CGI�ű���ͨ��dup2�ض���CGI�ı�׼������ݽ����ӽ��̹ܵ�output[1]������� (�ڸ������л��ȡ�ܵ�output[0],Ȼ�󽫴����ݷ��͸������)
		execl(path, path, NULL);
		exit(0);	// �ӽ����˳�
	}
	else
	{    /* parent ��ȡhttp��Դ������ͨ���ܵ�д���ӽ��̣�ʹ��ִ��execl ���� */
		close(cgi_output[1]);	// �ر�cgi_output����
		close(cgi_input[0]);	// �ر�cgi_inputд��

		if (strcasecmp(method, "POST") == 0) {
			// ��������ΪPOST��ʱ��,��POST���ݰ�������entity-body����
			// ͨ��cgi_input[1](д��)д�뵽CGI�ı�׼����
			for (i = 0; i < content_length; i++) {
				recv(client, &c, 1, 0);
				write(cgi_input[1], &c, 1);
			}
		}
		// ��ȡCGI�ı�׼���,���͵��ͻ���
		while (read(cgi_output[0], &c, 1) > 0) {
			send(client, &c, 1, 0);
		}


		// �رն����ļ�������
		close(cgi_output[0]); 
		close(cgi_input[1]);
		// �ȴ��ӽ��̽���
		waitpid(pid, &status, 0);
	}
}


int get_line(int sock, char *buf, int size)
{
	int		i = 0;
	char	c = '\0';
	int		n;

	while ((i < size - 1) && (c != '\n'))
	{
		// ��sock�ж�ȡһ���ֽ�
		n = recv(sock, &c, 1, 0);//recv����ԭ�ͣ�int recv��SOCKET s��char FAR *buf��int len��int flags����
	//	��һ�����������ն˵��׽�������������ʵ���ǿͻ��˶�Ӧ���׽���
	//	�ڶ���������ָ����ܵ��������ڵĻ�����
	//	�����������������������ߴ磬sizeof����
	//	���ĸ���������0��������
		if (n > 0)
		{
			// �� \r\n �� \r ת��Ϊ'\n'
			if (c == '\r')
			{
				// ������'\r'����Ԥ��һ���ֽ�
				n = recv(sock, &c, 1, MSG_PEEK);
				// �����ȡ������'\n',�Ͷ�ȡ,����c='\n'
				if ((n > 0) && (c == '\n'))
					recv(sock, &c, 1, 0);
				else
					c = '\n';
			}
			// ��ȡ���ݷ���buf
			buf[i] = c;
			i++;
		}
		else {
			c = '\n';
		}
	}
	buf[i] = '\0';
	// ����д��buf���ֽ���
	return(i);
}


void headers(int client, const char *filename)
{
	char buf[1024];
	(void)filename;  /* could use filename to determine file type */

	// ��ͻ��˻�д200Ӧ��
	strcpy(buf, "HTTP/1.0 200 OK\r\n");
	send(client, buf, strlen(buf), 0);
	strcpy(buf, SERVER_STRING);
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "Content-Type: text/html\r\n");
	send(client, buf, strlen(buf), 0);
	strcpy(buf, "\r\n");
	send(client, buf, strlen(buf), 0);
}


void not_found(int client)
{
	char buf[1024];
	// ��client��д404״̬
	sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, SERVER_STRING);
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "Content-Type: text/html\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "<BODY><P>The GuoQi_server could not fulfill\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "your request because the resource specified\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "is unavailable or nonexistent.\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "</BODY></HTML>\r\n");
	send(client, buf, strlen(buf), 0);
}


void serve_file(int client, const char *filename)
{
	FILE *resource = NULL;
	int numchars = 1;
	char buf[1024];

	buf[0] = 'A'; buf[1] = '\0';
	// ��client�ж�ȡ,ֱ��������������(��ʼ��start line���ײ�header�ļ��)
	while ((numchars > 0) && strcmp("\n", buf)) {  /* read & discard headers */
		numchars = get_line(client, buf, sizeof(buf));
	}
	// 
	resource = fopen(filename, "r");
	if (resource == NULL)
		not_found(client);	// 404����
	else
	{
		headers(client, filename);	// ��ͻ��˻�д200 OK
		cat(client, resource);		// ��resourceָ���ļ��е�����д��client
	}
	fclose(resource);
}


int startup(u_short *port)
{
	/*if (port < 0 || port > 65535)
		return -1;*/

	int listen_fd;
	struct sockaddr_in name;
	// ����һ��socket
	listen_fd = socket(PF_INET, SOCK_STREAM, 0);
	if (listen_fd == -1) {
		error_die("socket");
	}
	// ��д�󶨵�ַ
	memset(&name, 0, sizeof(name));
	name.sin_family = AF_INET;
	name.sin_port = htons(*port);
	name.sin_addr.s_addr = htonl(INADDR_ANY);
	// ��socket��ָ����ַ
	if (bind(listen_fd, (struct sockaddr *)&name, sizeof(name)) < 0) {
		error_die("bind");
	}
	// ��������������˿�,��ȡ��
	if (*port == 0)  /* if dynamically allocating a port */
	{
		socklen_t namelen = sizeof(name);
		if (getsockname(listen_fd, (struct sockaddr *)&name, &namelen) == -1)
			error_die("getsockname");
		*port = ntohs(name.sin_port);
	}
	// ����,�ȴ�����
	
	if (listen(listen_fd,100) < 0)
		error_die("listen");
	// �������ڼ������ļ�������
	return(listen_fd);
}


void unimplemented(int client)
{
	char buf[1024];
	// ��client��д501״̬
	sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, SERVER_STRING);
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "Content-Type: text/html\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "</TITLE></HEAD>\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "</BODY></HTML>\r\n");
	send(client, buf, strlen(buf), 0);
}


