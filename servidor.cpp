#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "msgstruct.h"

#define PORT 4000
#define BUFFER_SIZE 256
#define MAX_CONNECTIONS 10
#define BYTE_SIZE 8
#define SOCKET_DEFAULT_PROTOCOL 0
#define TRUE 1
#define FALSE 0
#define bool int

typedef struct STRUCT_CONNECTION_DATA
{
	int socket_fd, connection_id;
} CONNECTION_DATA;

int connection_id = 0;

void *connection_loop(void *data)
{
	CONNECTION_DATA *conn_data = (CONNECTION_DATA *)data;

	int connection_id = conn_data->connection_id;
	int socket_fd = conn_data->socket_fd;

	while (TRUE)
	{
		char *buffer = read_all_bytes(socket_fd, sizeof(PROCEDURE_SELECT));
		if (buffer == NULL)
		{
			break;
		}

		PROCEDURE_SELECT *procedure = (PROCEDURE_SELECT *)buffer;

		switch (procedure->proc_id)
		{
		case 1:
			printf("Procedure 1 received\n");
			break;
		case 2:
			printf("Procedure 2 received\n");
			break;
		}
	}

	printf("Connection %d closed\n", conn_data->connection_id);
	close(conn_data->socket_fd);

	return NULL;
}

// returns socket file descriptor
int inicializar_servidor()
{
	struct sockaddr_in serv_addr;
	char buffer[BUFFER_SIZE];
	int socket_fd;

	socket_fd = socket(AF_INET, SOCK_STREAM, SOCKET_DEFAULT_PROTOCOL);

	if (socket_fd == -1)
	{
		printf("ERROR opening socket\n");
		exit(0);
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	bzero(&(serv_addr.sin_zero), BYTE_SIZE);

	int bind_return = bind(socket_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

	if (bind_return < 0)
	{
		printf("ERROR on binding\n");
		exit(0);
	}

	listen(socket_fd, MAX_CONNECTIONS);

	printf("Server listening on port %d\n", PORT);

	return socket_fd;
}

void gerenciador_de_conexoes(int socket_fd)
{
	int new_conn_socket_fd;
	struct sockaddr_in cli_addr;
	socklen_t cli_len;

	cli_len = sizeof(struct sockaddr_in);

	while (TRUE)
	{
		new_conn_socket_fd = accept(socket_fd, (struct sockaddr *)&cli_addr, &cli_len);
		if (new_conn_socket_fd == -1)
		{
			printf("ERROR on accept");
		}
		else
		{
			pthread_t connection_thread;
			CONNECTION_DATA *new_conn_data = (CONNECTION_DATA *)malloc(sizeof(CONNECTION_DATA));

			new_conn_data->socket_fd = new_conn_socket_fd;
			new_conn_data->connection_id = connection_id;
			connection_id++;

			printf("Connection %d accepted\n", new_conn_data->connection_id);

			pthread_create(&connection_thread, NULL, connection_loop, (void *)new_conn_data);
		}
	}
}

int main(int argc, char *argv[])
{
	int socket_fd;

	socket_fd = inicializar_servidor();
	gerenciador_de_conexoes(socket_fd);

	close(socket_fd);
	return 0;
}
