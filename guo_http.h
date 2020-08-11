#pragma once
#ifndef HTTPD_H
#define HTTPD_H
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>
#include<sys/epoll.h>
#include<errno.h>
#include <cstdlib>
#include <fcntl.h>

// 接受请求,并处理
void *accept_request(void *);

// 无法处理请求,回写400到client
void bad_request(int);

// 将文件内容发送到client
void cat(int client, FILE* fp);

// 不可以执行CGI程序,回写500到client
void cannot_execute(int);

// 输出错误信息
void error_die(const char *);

// 执行cgi程序
void execute_cgi(int, const char *, const char *, const char *);

// 从fd中读取一行,并将读取的'\r\n'和'\r'转换为'\n'
int get_line(int fd, char* buf, int bufsize);

// 返回200 OK给客户端
void headers(int clientsockfd, const char * filename);

// 返回404状态给客户端
void not_found(int);

// 处理客户端文件请求,发送404或200+文件内容到client
void serve_file(int client, const char* filename);

// 开启soket监听
int startup(u_short *);

// 返回501状态给客户端
void unimplemented(int);

#endif // !HTTPD_H

