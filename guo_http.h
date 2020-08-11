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

// ��������,������
void *accept_request(void *);

// �޷���������,��д400��client
void bad_request(int);

// ���ļ����ݷ��͵�client
void cat(int client, FILE* fp);

// ������ִ��CGI����,��д500��client
void cannot_execute(int);

// ���������Ϣ
void error_die(const char *);

// ִ��cgi����
void execute_cgi(int, const char *, const char *, const char *);

// ��fd�ж�ȡһ��,������ȡ��'\r\n'��'\r'ת��Ϊ'\n'
int get_line(int fd, char* buf, int bufsize);

// ����200 OK���ͻ���
void headers(int clientsockfd, const char * filename);

// ����404״̬���ͻ���
void not_found(int);

// ����ͻ����ļ�����,����404��200+�ļ����ݵ�client
void serve_file(int client, const char* filename);

// ����soket����
int startup(u_short *);

// ����501״̬���ͻ���
void unimplemented(int);

#endif // !HTTPD_H

