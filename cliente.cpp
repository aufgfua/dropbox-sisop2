#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "msgstruct.h"

#define PORT 4000
#define BUFFER_SIZE 256
#define MAX_CONNECTIONS 10
#define BYTE_SIZE 8
#define SOCKET_DEFAULT_PROTOCOL 0
#define TRUE 1
#define FALSE 0
#define bool int

int conectar_socket(struct hostent *server, int port, char *username)
{
	int sock_fd, read_len, write_len;

	struct sockaddr_in serv_addr;

	if (server == NULL)
	{
		fprintf(stderr, "ERROR, no such host\n");
		exit(0);
	}

	sock_fd = socket(AF_INET, SOCK_STREAM, SOCKET_DEFAULT_PROTOCOL);
	if (sock_fd == -1)
	{
		printf("ERROR opening socket\n");
		exit(0);
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	serv_addr.sin_addr = *((struct in_addr *)server->h_addr);

	bzero(&(serv_addr.sin_zero), BYTE_SIZE);

	int conn_return = connect(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	if (conn_return < 0)
	{
		printf("ERROR connecting\n");
		exit(0);
	}
	return sock_fd;
}

void gerencia_conexao(int sock_fd)
{
	while (TRUE)
	{
		PROCEDURE_SELECT procedure;
		printf("Select procedure: ");

		int proc = 0;
		scanf("%d", &proc);

		procedure.proc_id = proc;

		write_all_bytes(sock_fd, (char *)&procedure, sizeof(PROCEDURE_SELECT));
	}
}

int main(int argc, char *argv[])
{
	int sock_fd;

	struct sockaddr_in serv_addr;
	struct hostent *server;
	char *username;
	int port;

	if (argc < 3)
	{
		fprintf(stderr, "usage '%s <username> <server_ip_address> <port>'\n", argv[0]);
		exit(0);
	}

	server = gethostbyname(argv[2]);
	username = argv[1];
	port = atoi(argv[3]);

	sock_fd = conectar_socket(server, port, username);

	gerencia_conexao(sock_fd);

	close(sock_fd);

	return 0;
}
