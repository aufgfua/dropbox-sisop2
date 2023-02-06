#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "shared.h"

#define BUFFER_SIZE 256
#define MAX_CONNECTIONS 10
#define BYTE_SIZE 8
#define SOCKET_DEFAULT_PROTOCOL 0
#define TRUE 1
#define FALSE 0
#define bool int

char *user_directory = (char *)malloc(MAX_PATH_SIZE);

void send_username(int sock_fd, char *username)
{
	LOGIN login;
	strcpy(login.username, username);
	write_all_bytes(sock_fd, (char *)&login, sizeof(LOGIN));

	printf("Logged in as %s\n", username);
}

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

	send_username(sock_fd, username);

	strcpy(user_directory, mount_base_path(username, CLIENT_BASE_DIR));
	create_folder_if_not_exists(user_directory);

	printf("User directory: %s\n", user_directory);

	return sock_fd;
}

void gerencia_conexao(int sock_fd, char *username)
{

	while (TRUE)
	{
		printf("\n\nStarting data send\n");
		PROCEDURE_SELECT procedure;

		procedure.proc_id = PROCEDURE_LIST_FILES;

		write_all_bytes(sock_fd, (char *)&procedure, sizeof(PROCEDURE_SELECT));

		vector<USR_FILE> *files_vector = list_files(user_directory);

		USR_FILE *files = files_vector->data();

		int files_bytes_size = files_vector->size() * sizeof(USR_FILE);
		printf("Files size - %d\n", files_bytes_size);
		vector<packet> *packets_vector = fragment_data((char *)files, files_bytes_size);
		packet *packets = packets_vector->data();
		printf("Files fragmented\n");

		NUMBER_OF_PACKETS number_of_packets;
		number_of_packets.pck_number = get_number_of_packets(files_bytes_size);
		number_of_packets.total_size = files_bytes_size;
		printf("Sending number of packets to server - %d\n", number_of_packets.pck_number);
		write_all_bytes(sock_fd, (char *)&number_of_packets, sizeof(NUMBER_OF_PACKETS));

		int i;
		printf("Sending files to server\n");
		for (i = 0; i < number_of_packets.pck_number; i++)
		{
			print_packet(packets[i]);
			write_all_bytes(sock_fd, (char *)&packets[i], sizeof(packet));
		}
		printf("Files sent to server!!!\n");
		getchar();
	}
}

int main(int argc, char *argv[])
{
	int sock_fd;

	struct sockaddr_in serv_addr;
	struct hostent *server;
	char *username;
	int port;

	if (argc < 4)
	{
		fprintf(stderr, "usage '%s <username> <server_ip_address> <port>'\n", argv[0]);
		exit(0);
	}

	server = gethostbyname(argv[2]);
	username = argv[1];
	port = atoi(argv[3]);

	sock_fd = conectar_socket(server, port, username);

	printf("Connected to server\n");

	gerencia_conexao(sock_fd, username);

	close(sock_fd);

	return 0;
}
