#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

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
	int socket_fd, socket_id;
} CONNECTION_DATA;

int connection_id = 0;

// TODO multiple connections get misconfigured and server thinks all of them are the same. Fix that

void *read_from_client(void *data)
{
	CONNECTION_DATA *conn_data = (CONNECTION_DATA *)data;
	int read_len, write_len;
	char buffer[BUFFER_SIZE];
	bool first_run = TRUE;

	while (strlen(buffer) != 0 || first_run)
	{
		first_run = FALSE;
		bzero(buffer, BUFFER_SIZE);

		/* read from the socket */
		read_len = read(conn_data->socket_fd, buffer, BUFFER_SIZE);

		if (read_len < 0)
		{
			printf("ERROR reading from socket");
		}

		if (strlen(buffer) == 0)
		{
			break;
		}

		printf("%d - Here is the message: %s\n", conn_data->socket_id, buffer);

		/* write in the socket */
		write_len = write(conn_data->socket_fd, "I got your message", 18);

		if (write_len < 0)
		{
			printf("ERROR writing to socket");
		}
	}

	printf("Connection %d closed\n", conn_data->socket_id);
	close(conn_data->socket_fd);
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
		printf("ERROR opening socket");
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	bzero(&(serv_addr.sin_zero), BYTE_SIZE);

	int bind_return = bind(socket_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

	if (bind_return < 0)
	{
		printf("ERROR on binding");
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
			CONNECTION_DATA *new_conn_data = malloc(sizeof(CONNECTION_DATA));

			new_conn_data->socket_fd = new_conn_socket_fd;
			new_conn_data->socket_id = connection_id;
			connection_id++;

			printf("Connection %d accepted\n", new_conn_data->socket_id);

			pthread_create(&connection_thread, NULL, read_from_client, (void *)new_conn_data);
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
