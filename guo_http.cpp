#include"guo_http.h"

#define ISspace(x) isspace((int)(x))
#define SERVER_STRING "Server: jdbhttpd/0.1.0\r\n"


void *accept_request(void *client)
{
	char	buf[1024];
	int		numchars;
	char	method[255];	// 方法(请求类型)
	char	url[255];		// 请求的资源URL
	char	path[512];		// 请求资源的本地路径
	size_t i, j;
	struct stat st;//状态信息
	int cgi = 0;/* 为真时,表示服务器需调用一个CGI程序 */
	char *query_string = NULL; 

	// 1. 从客户端连接请求数据包中读取一行。返回 buf[] 中的字节数
	numchars = get_line((long)client, buf, sizeof(buf)); 
	// 2. 从读取的数据中提取出请求类型
	i = 0; j = 0;
	while (!ISspace(buf[j]) && (i < sizeof(method) - 1))
		//ISspace(int c); 判断字符c是否为空白符，是返回非0值，否返回0 
	{
		method[i] = buf[j];// 拷贝读取的信息
		i++; j++;
	}
	method[i] = '\0';
	// 3. 判断请求类型
	if (strcasecmp(method, "GET") && strcasecmp(method, "POST"))//仅支持这两种方法。
    //strcasecmp（const char *s1, const char *s2);判断字符串是否相等，（忽略大小写）。如果相同则返回0，
    //s1 长度大于s2 长度则返回大于0 的值，s1 长度若小于s2 长度则返回小于0 的值。
	{
		unimplemented((long)client);	// 不支持请求类型,向客户端返回501
		return	NULL;
	}

	if (strcasecmp(method, "POST") == 0) 
	{
		cgi = 1;	// 如果是POST请求, 直接将  cgi标志位 位真 -> 调用cgi程序
	}

	// 4. 提取请求的资源URL，GET方法用URL传递信息，所以需要提取相关资源
	//http请求报文首行格式：GET /mix/76.html?name=kelvin&password=123456 HTTP/1.1
	//请求的方法（GET）
	//请求的URL(/mix/76.html)
	//动态信息 ?name=kelvin&password=123456
	//协议类型及版本(HTTP/1.1)
	i = 0;
	while (ISspace(buf[j]) && (j < sizeof(buf))) {
		j++;	// 定位下一个非空格位置
	}
	while (!ISspace(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf)))//url = mix/76.html?name=kelvin&password=123456
	{
		url[i] = buf[j];
		i++; j++;
	}
	url[i] = '\0';

	if (strcasecmp(method, "GET") == 0)
	{
		query_string = url;	// 为GET请求,查询语句为url

		// 如果查询语句中含义'?',查询语句为'?'字符后面部分
		while ((*query_string != '?') && (*query_string != '\0')) {
			query_string++;
		}
		if (*query_string == '?')//url中有问号，说明是动态请求，所以cgi = 1, 否则就直接执行静态页面的程序
		{
			cgi = 1;
			*query_string = '\0';	// 从'?'处截断,前半截为url
			query_string++;
		}
	}

	// 5. 生成本地路径
	sprintf(path, "htdocs%s", url);//sprintf()函数是将参数输入到字符串中，产生的结果为  path =“htdocs + url”;
	if (path[strlen(path) - 1] == '/') {
		strcat(path, "index.html");	//如果请求url是以'/'结尾,则指定为该目录下的index.html文件
	}
	// 6. 判断请求资源的状态
	if (stat(path, &st) == -1) //获取文件信息，成功为 0，失败为 -1；
	{
		// 从client中读取,直到遇到两个换行(起始行startline和首部header之间间隔)
		while ((numchars > 0) && strcmp("\n", buf)) //strcmp（const char *s1,const char *s2）; 字符串比较，相等返回0，s1 > s2 ;返回正值，否则返回负值
		{
			numchars = get_line((long)client, buf, sizeof(buf)); //???????    有什么作用
		}
		not_found((long)client);	// 501
	}
	else
	{
		// 请求的是个目录,指定为该目录下的index.html文件
		if ((st.st_mode & S_IFMT) == S_IFDIR) {
			strcat(path, "/index.html");
		}
		// 请求的是一个可执行文件,作为CGI程序
		if ((st.st_mode & S_IXUSR) ||
			(st.st_mode & S_IXGRP) ||
			(st.st_mode & S_IXOTH)) {
			cgi = 1;
		}
		// 判断是执行一个CGI程序还是返回一个文件内容给客户端
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
	// 向client回写400状态
	sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n");//sprintf();将字符写进特定的寄存器中
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
	// 从resource中读取一行
	fgets(buf, sizeof(buf), resource);
	// 将文件中的内容发送到客户端
	while (!feof(resource))
	{
		send(client, buf, strlen(buf), 0);
		fgets(buf, sizeof(buf), resource);
	}
}


void cannot_execute(int client)
{
	char buf[1024];
	// 向客户端回写500状态,不可以执行CGI程序
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
	perror(sc);	// 输出错误信息
	exit(1);	// 退出程序
}

/*
 * @brief	执行一个CGI脚本.可能需要设置适当的环境变量.
 CGI全称是 Common Gate Intergace ，在物理上，CGI是一段程序，它运行在Server上，提供同客户端 Html页面的接口。
 * @prama	client	客户端socket文件描述符
 * @prama	path	CGI 脚本路径
 * @prama	method	请求类型
 * @prama	query_string	查询语句
*/
void execute_cgi(int client, const char *path,
	const char *method, const char *query_string)
{
	char	buf[1024];
	int		cgi_output[2];	// CGI程序输出管道
	int		cgi_input[2];	// CGI程序输入管道

	pid_t	pid;       //进程     
	int		status;
	int		i;
	char	c;
	int		numchars = 1;
	int		content_length = -1;

	buf[0] = 'A'; buf[1] = '\0';//????????

	if (strcasecmp(method, "GET") == 0) //字符串比对函数
	{
		// 从client中读取,直到遇到两个换行
		while ((numchars > 0) && strcmp("\n", buf))
		{ 
			numchars = get_line(client, buf, sizeof(buf));//
		}
	}
	else    /* POST */
	{
		// 获取请求主体的长度
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
  // 以上读取对应各种方法所采用的各自的方法


	// 向client回写200 OK
	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	send(client, buf, strlen(buf), 0);

	// 创建两个匿名管道
	if (pipe(cgi_output) < 0) {
		cannot_execute(client);
		return;
	}
	if (pipe(cgi_input) < 0) {
		cannot_execute(client);
		return;
	}

	// 创建子进程,去执行CGI程序
	if ((pid = fork()) < 0) {
		cannot_execute(client);
		return;
	}
	if (pid == 0)  /* child: CGI script */
	{
		char meth_env[255];
		char query_env[255];
		char length_env[255];

		//标椎输出重定向output管道的写入段 stdout 1
		dup2(cgi_output[1], 1);	//复制cgi_output[1](读端)到子进程的标准输出

		//标椎输入重定向input管道的写入段 stdin 0
		dup2(cgi_input[0], 0);	//复制cgi_input[0](写端)到子进程的标准输入

		close(cgi_output[0]);	//关闭多余文件描述符
		close(cgi_input[1]);

		sprintf(meth_env, "REQUEST_METHOD=%s", method);
		putenv(meth_env);	// 添加一个环境变量

		if (strcasecmp(method, "GET") == 0) {
			sprintf(query_env, "QUERY_STRING=%s", query_string);
			putenv(query_env);
		}
		else {   /* POST */
			sprintf(length_env, "CONTENT_LENGTH=%d", content_length);
			putenv(length_env);
		}
		// 执行CGI程序
		//调用exec函数，执行CGI脚本，通过dup2重定向，CGI的标准输出内容进入子进程管道output[1]的输入端 (在父进程中会读取管道output[0],然后将此内容发送给浏览器)
		execl(path, path, NULL);
		exit(0);	// 子进程退出
	}
	else
	{    /* parent 读取http资源，将其通过管道写给子进程，使其执行execl 程序 */
		close(cgi_output[1]);	// 关闭cgi_output读端
		close(cgi_input[0]);	// 关闭cgi_input写端

		if (strcasecmp(method, "POST") == 0) {
			// 请求类型为POST的时候,将POST数据包的主体entity-body部分
			// 通过cgi_input[1](写端)写入到CGI的标准输入
			for (i = 0; i < content_length; i++) {
				recv(client, &c, 1, 0);
				write(cgi_input[1], &c, 1);
			}
		}
		// 读取CGI的标准输出,发送到客户端
		while (read(cgi_output[0], &c, 1) > 0) {
			send(client, &c, 1, 0);
		}


		// 关闭多余文件描述符
		close(cgi_output[0]); 
		close(cgi_input[1]);
		// 等待子进程结束
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
		// 从sock中读取一个字节
		n = recv(sock, &c, 1, 0);//recv函数原型：int recv（SOCKET s，char FAR *buf，int len，int flags）；
	//	第一个参数：接收端的套接字描述符，其实就是客户端对应的套接字
	//	第二个参数：指向接受的数据所在的缓冲区
	//	第三个参数：缓冲区的最大尺寸，sizeof（）
	//	第四个参数：置0就完事了
		if (n > 0)
		{
			// 将 \r\n 或 \r 转换为'\n'
			if (c == '\r')
			{
				// 读到了'\r'就再预读一个字节
				n = recv(sock, &c, 1, MSG_PEEK);
				// 如果读取到的是'\n',就读取,否则c='\n'
				if ((n > 0) && (c == '\n'))
					recv(sock, &c, 1, 0);
				else
					c = '\n';
			}
			// 读取数据放入buf
			buf[i] = c;
			i++;
		}
		else {
			c = '\n';
		}
	}
	buf[i] = '\0';
	// 返回写入buf的字节数
	return(i);
}


void headers(int client, const char *filename)
{
	char buf[1024];
	(void)filename;  /* could use filename to determine file type */

	// 向客户端回写200应答
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
	// 向client回写404状态
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
	// 从client中读取,直到遇到两个换行(起始行start line和首部header的间隔)
	while ((numchars > 0) && strcmp("\n", buf)) {  /* read & discard headers */
		numchars = get_line(client, buf, sizeof(buf));
	}
	// 
	resource = fopen(filename, "r");
	if (resource == NULL)
		not_found(client);	// 404错误
	else
	{
		headers(client, filename);	// 向客户端回写200 OK
		cat(client, resource);		// 将resource指向文件中的内容写入client
	}
	fclose(resource);
}


int startup(u_short *port)
{
	/*if (port < 0 || port > 65535)
		return -1;*/

	int listen_fd;
	struct sockaddr_in name;
	// 创建一个socket
	listen_fd = socket(PF_INET, SOCK_STREAM, 0);
	if (listen_fd == -1) {
		error_die("socket");
	}
	// 填写绑定地址
	memset(&name, 0, sizeof(name));
	name.sin_family = AF_INET;
	name.sin_port = htons(*port);
	name.sin_addr.s_addr = htonl(INADDR_ANY);
	// 绑定socket到指定地址
	if (bind(listen_fd, (struct sockaddr *)&name, sizeof(name)) < 0) {
		error_die("bind");
	}
	// 如果采用随机分配端口,获取它
	if (*port == 0)  /* if dynamically allocating a port */
	{
		socklen_t namelen = sizeof(name);
		if (getsockname(listen_fd, (struct sockaddr *)&name, &namelen) == -1)
			error_die("getsockname");
		*port = ntohs(name.sin_port);
	}
	// 监听,等待连接
	
	if (listen(listen_fd,100) < 0)
		error_die("listen");
	// 返回正在监听的文件描述符
	return(listen_fd);
}


void unimplemented(int client)
{
	char buf[1024];
	// 向client回写501状态
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


