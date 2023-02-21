#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <queue>
#include <string>
#include <functional>

#include <regex>
#include <sstream>

#include "shared.h"

#define BUFFER_SIZE 256
#define MAX_CONNECTIONS 10
#define BYTE_SIZE 8
#define SOCKET_DEFAULT_PROTOCOL 0
#define bool int

queue<int> orders;

char *user_directory = (char *)malloc(MAX_PATH_SIZE);

long last_sync = 0;

string upload_target;
string download_target;

void send_username(int sock_fd, char *username)
{
	LOGIN login;
	strcpy(login.username, username);
	write_all_bytes(sock_fd, (char *)&login, sizeof(LOGIN));

	printf("Logged in as %s\n", username);
}

int connect_socket(struct hostent *server, int port, char *username)
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

int select_procedure(int sock_fd)
{

	if (get_now() - last_sync > SYNC_WAIT)
	{
		printf("Need to sync...\n");
		return PROCEDURE_SYNC_FILES;
	}

	if (orders.size() > 0)
	{
		int order = orders.front();
		orders.pop();
		return order;
	}

	return PROCEDURE_NOP;
}

void *get_input_order_loop(void *data)
{
	string order;
	while (TRUE)
	{
		getline(cin, order);

		if (order.find("download") != string::npos)
		{

			regex substring("download (.*)");
			smatch match;
			regex_search(order, match, substring);
			string file_name = match[1];

			printf("Downloading %s\n\n", file_name.c_str());

			download_target = file_name;
			orders.push(PROCEDURE_DOWNLOAD_FROM_SERVER);

			continue;
		}

		if (order.find("upload") != string::npos)
		{

			regex substring("upload (.*)");
			smatch match;
			regex_search(order, match, substring);
			string file_path = match[1];

			printf("Uploading %s\n\n", file_path.c_str());

			upload_target = file_path;
			orders.push(PROCEDURE_UPLOAD_TO_SERVER);

			continue;
		}

		if (order.find("delete") != string::npos)
		{

			regex substring("delete (.*)");
			smatch match;
			regex_search(order, match, substring);
			string file_name = match[1];

			printf("Deleting %s\n\n", file_name.c_str());
			continue;
		}

		if (order == "list_server")
		{
			orders.push(PROCEDURE_LIST_SERVER);
			continue;
		}

		if (order == "list_client")
		{
			vector<USR_FILE> *files_vector = list_files(user_directory);
			print_usr_files(*files_vector);
			printf("\n");
			continue;
		}

		if (order == "get_sync_dir")
		{
			printf("Sync dir: %s ready!\n\n", user_directory);
			continue;
		}

		if (order == "exit")
		{
			orders.push(PROCEDURE_EXIT);
			continue;
		}
	}
	return NULL;
}

void cli_handle_procedure(int sock_fd, PROCEDURE_SELECT *procedure)
{
	switch (procedure->proc_id)
	{
	case PROCEDURE_NOP:
		// printf("NOP\n");
		break;

	case PROCEDURE_SYNC_FILES:
		printf("Syncing files...\n\n");
		send_files_list(sock_fd, user_directory);
		cli_transaction_loop(sock_fd, user_directory);
		last_sync = get_now();
		break;

	case PROCEDURE_LIST_SERVER:
	{
		printf("Listing server files...\n\n");
		vector<USR_FILE> remote_files = receive_files_list(sock_fd);
		print_usr_files(remote_files);
	}
	break;
	case PROCEDURE_UPLOAD_TO_SERVER:
	{
		printf("Uploading %s to server...\n\n", upload_target.c_str());

		char current_cwd[MAX_PATH_SIZE];
		getcwd(current_cwd, MAX_PATH_SIZE);
		strcat(current_cwd, "/");

		send_single_file(sock_fd, upload_target.c_str(), current_cwd, CLIENT_SYNC_UPLOAD);

		orders.push(PROCEDURE_SYNC_FILES);
	}
	break;
	case PROCEDURE_DOWNLOAD_FROM_SERVER:
	{
		printf("Downloading %s from server...\n\n", download_target.c_str());
		char current_dir[MAX_PATH_SIZE];
		getcwd(current_dir, MAX_PATH_SIZE);
		strcat(current_dir, "/");

		DESIRED_FILE desired_file;

		strcpy(desired_file.filename, download_target.c_str());

		send_data_with_packets(sock_fd, (char *)&desired_file, sizeof(DESIRED_FILE));

		receive_single_file(sock_fd, current_dir);
	}
	break;
	case PROCEDURE_EXIT:
		printf("Exiting...\n\n");
		break;
	}
}

void cli_turn(int sock_fd)
{
	int proc_id = select_procedure(sock_fd);
	// printf("Selected procedure: %d - ", proc_id);
	PROCEDURE_SELECT *procedure = send_procedure(sock_fd, proc_id);
	cli_handle_procedure(sock_fd, procedure);
}

void cli_connection_loop(int sock_fd, char *username)
{
	char turn = START_TURN;

	uint32_t run = 0;

	while (TRUE)
	{
		// printf("\nRun %d - Turn %d\n", run, turn);
		run++;
		switch (turn)
		{
		case CLI_TURN:
		{
			cli_turn(sock_fd);
		}
		break;
		case SRV_TURN:
		{
			PROCEDURE_SELECT *procedure = receive_procedure(sock_fd);
			// printf("Received procedure: %d - ", procedure->proc_id);
			cli_handle_procedure(sock_fd, procedure);
		}
		break;
		}

		turn = (turn == CLI_TURN) ? SRV_TURN : CLI_TURN;
	}
}

void gerencia_conexao(int sock_fd, char *username)
{
	send_username(sock_fd, username);

	strcpy(user_directory, mount_base_path(username, CLIENT_BASE_DIR));
	create_folder_if_not_exists(user_directory);
	printf("User directory: %s\n\n", user_directory);

	cli_connection_loop(sock_fd, username);
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

	sock_fd = connect_socket(server, port, username);

	printf("Connected to server\n");

	pthread_t input_thread;
	pthread_create(&input_thread, NULL, get_input_order_loop, (void *)NULL);

	gerencia_conexao(sock_fd, username);

	close(sock_fd);

	return 0;
}
