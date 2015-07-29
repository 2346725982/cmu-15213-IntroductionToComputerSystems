#include <stdio.h>
#include <sys/socket.h>
#include "csapp.h"
#include "cache.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* Global Variables */
cache_list* cl;

/* You won't lose style points for including these long lines in your code */
//static const char *user_agent_hdr =
//		"User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
//static const char *accept_hdr =
//		"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
//static const char *accept_encoding_hdr = "Accept-Encoding: gzip, deflate\r\n";

/* Function Definition */
void *thread(void* vargp);
void response(int connfd);
void forward_request(int client_fd, char* host, char* port, char* request,
		char* data);

int main(int argc, char** argv) {
	int listenfd;
	int* connfd;
	socklen_t clientlen;
	struct sockaddr_storage clientaddr; /* Enough room for any addr */
	char client_hostname[MAXLINE], client_port[MAXLINE];
	pthread_t tid;

	/* Check command line args */
	if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
	}

	cl = malloc(sizeof(cache_list));
	init_list(cl);
	listenfd = Open_listenfd(argv[1]);

	while (1) {
		clientlen = sizeof(struct sockaddr_storage); /* Important! */
		connfd = (int *) malloc(sizeof(int));
		*connfd = Accept(listenfd, (SA *) &clientaddr, &clientlen);
		Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE,
				client_port, MAXLINE, 0);
		printf("Connected from (%s, %s)\n", client_hostname, client_port);

		/* Get data from server */
		Pthread_create(&tid, NULL, thread, connfd);

		printf("Disconnected from (%s, %s)\n", client_hostname, client_port);

	}
	free_list(cl);
	exit(0);
}

void *thread(void* vargp) {
	int connfd = *((int *) vargp);
	free(vargp);

	Pthread_detach(Pthread_self());
	response(connfd);
	Close(connfd);
	return NULL;
}

void response(int client_fd) {

	/* Initialize client_rio */
	rio_t client_rio;
	char buf[MAXLINE];

	Rio_readinitb(&client_rio, client_fd);
	Rio_readlineb(&client_rio, buf, MAXLINE);

	/* Parse request */
	char* p;
	char method[10];
	char version[10];
	char host[MAXLINE];
	char port[10] = "80";
	char path[MAXLINE];
	cache_block* cb;

	sscanf(buf, "%s http://%15[^/]%s %s", method, host, path, version);

	if ((p = strstr(host, ":")) != NULL) {
		strcpy(port, p + 1);
		sscanf(host, "%[^:]", host);
	}

	/* Generate request */
	char request[MAXLINE];
	request[0] = '\0';
	strcat(request, method);
	strcat(request, " ");
	strcat(request, path);
	strcat(request, " ");
	strcat(request, "HTTP/1.0");
	strcat(request, "  \r\n\r\n");

	/* Cache hit, read_cache */
	if ((cb = find(cl, request)) != NULL) {
		Rio_writen(client_fd, cb->content, cb->size);
		return;
	}

	/* Cache miss, forward_request */
	char data[MAX_OBJECT_SIZE];
	data[0] = '\0';
	forward_request(client_fd, host, port, request, data);

}

void forward_request(int client_fd, char* host, char* port, char* request,
		char* data) {
	size_t n;
	size_t total_bytes;
	char buf[MAX_OBJECT_SIZE];
	char tmp_data[MAX_OBJECT_SIZE];
	int server_fd;
	rio_t server_rio;

	total_bytes = 0;

	/* Connect to server */
	server_fd = Open_clientfd(host, port);

	/* Send request to server through Rio */
	Rio_readinitb(&server_rio, server_fd);
	Rio_writen(server_fd, request, strlen(request));

	/* Read data from server */
	while ((n = Rio_readnb(&server_rio, buf, MAX_OBJECT_SIZE)) != 0) {
		memcpy(tmp_data + total_bytes, buf, sizeof(char) * n);

		total_bytes += n;

		Rio_writen(client_fd, buf, n);
	}

	write_cache(cl, request, tmp_data, total_bytes);

	Close(server_fd);

}
