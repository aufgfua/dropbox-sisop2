#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "shared.h"

#define PORT 40001
#define BUFFER_SIZE 256
#define MAX_CONNECTIONS 10
#define BYTE_SIZE 8
#define SOCKET_DEFAULT_PROTOCOL 0
#define TRUE 1
#define FALSE 0
#define bool int
#define LEAVE_KEY 'q'

typedef struct STRUCT_CONNECTION_DATA
{
	int socket_fd, connection_id;
} CONNECTION_DATA;

int connection_id = 0;
int global_socket_fd;

void receive_files_list(int socket_fd)
{
	// read NUMBER_OF_PACKETS
	char *buffer = read_all_bytes(socket_fd, sizeof(NUMBER_OF_PACKETS));
	if (buffer == NULL)
	{
		return;
	}
	NUMBER_OF_PACKETS *number_of_packets = (NUMBER_OF_PACKETS *)buffer;

	printf("Number of packets: %d\n", number_of_packets->pck_number);

	vector<packet> *packets_vector = new vector<packet>();

	uint32_t remaining_buffer_size = number_of_packets->total_size;
	for (int i = 0; i < number_of_packets->pck_number; i++)
	{
		buffer = read_all_bytes(socket_fd, sizeof(packet));
		if (buffer == NULL)
		{
			return;
		}
		packet *pck = (packet *)buffer;
		packets_vector->push_back(*pck);

		printf("Packet %d received\n", pck->seqn);
	}

	printf("All packets received - %d\n", number_of_packets->pck_number);

	packet *packets = (packet *)packets_vector;

	char *data_recovered = (char *)malloc(number_of_packets->pck_number * MAX_PACKET_PAYLOAD_SIZE);

	int read_size = 0;
	for (int i = 0; i < packets_vector->size(); i++)
	{
		packet pck = packets_vector->at(i);
		print_packet(pck);
		memcpy(data_recovered + read_size, pck._payload, pck.length);
		read_size += pck.length;
	}

	USR_FILE *file_list = (USR_FILE *)data_recovered;

	for (int i = 0; i < number_of_packets->pck_number; i++)
	{
		USR_FILE file = file_list[i];
		print_usr_file(file);
	}
}

void handle_procedure_selection(int socket_fd, PROCEDURE_SELECT *procedure)
{
	switch (procedure->proc_id)
	{
	case 1:
		receive_files_list(socket_fd);
		break;
	case 2:
		printf("Procedure 2 received\n");
		break;
	}
}

void connection_loop(int socket_fd, char *username, char *user_directory)
{
	while (TRUE)
	{

		char *buffer = read_all_bytes(socket_fd, sizeof(PROCEDURE_SELECT));
		printf("Procedure received\n");

		if (buffer == NULL)
		{
			break;
		}

		PROCEDURE_SELECT *procedure = (PROCEDURE_SELECT *)buffer;

		handle_procedure_selection(socket_fd, procedure);
	}
}

char *get_username(int socket_fd)
{
	LOGIN *login;
	printf("Waiting for username...\n");
	char *buffer = read_all_bytes(socket_fd, sizeof(LOGIN));
	printf("Username received\n");

	if (buffer == NULL)
	{
		printf("Connection %d closed\n", connection_id);
		close(socket_fd);
		return NULL;
	}

	login = (LOGIN *)buffer;

	return login->username;
}

void *start_connection(void *data)
{

	CONNECTION_DATA *conn_data = (CONNECTION_DATA *)data;

	int connection_id = conn_data->connection_id;
	int socket_fd = conn_data->socket_fd;

	char *username = get_username(socket_fd);

	char *user_directory = (char *)malloc(MAX_PATH_SIZE);

	printf("Con #%d - logged in as %s\n", connection_id, username);

	strcpy(user_directory, mount_base_path(username, SERVER_BASE_DIR));
	create_folder_if_not_exists(user_directory);
	printf("User directory: %s\n", user_directory);

	connection_loop(socket_fd, username, user_directory);

	printf("Connection %d closed\n", conn_data->connection_id);
	close(conn_data->socket_fd);

	return NULL;
}

// returns socket file descriptor
int inicializar_servidor(int port)
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
	serv_addr.sin_port = htons(port);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	bzero(&(serv_addr.sin_zero), BYTE_SIZE);

	int bind_return = bind(socket_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

	if (bind_return < 0)
	{
		printf("ERROR on binding\n");
		exit(0);
	}

	listen(socket_fd, MAX_CONNECTIONS);

	printf("Server listening on port %d\n", port);

	global_socket_fd = socket_fd;
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

			pthread_create(&connection_thread, NULL, start_connection, (void *)new_conn_data);
		}
	}
}

int main(int argc, char *argv[])
{
	int socket_fd;

	int port = argc > 1 ? atoi(argv[1]) : PORT;

	socket_fd = inicializar_servidor(port);

	gerenciador_de_conexoes(socket_fd);

	close(socket_fd);
	return 0;
}
