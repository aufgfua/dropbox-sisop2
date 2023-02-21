#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <map>
#include "shared.h"

#define PORT 40001
#define BUFFER_SIZE 256
#define MAX_WAITING_CONNECTIONS 10
#define BYTE_SIZE 8
#define SOCKET_DEFAULT_PROTOCOL 0
#define bool int
#define LEAVE_KEY 'q'

typedef struct STRUCT_CONNECTION_DATA
{
	int sock_fd, connection_id;
} CONNECTION_DATA;

int connection_id = 0;
int global_sock_fd;

map<int, long> last_sync;

void define_sync_local_files(vector<UP_DOWN_COMMAND> *sync_files, vector<USR_FILE> local_files, vector<USR_FILE> remote_files)
{

	for (int i = 0; i < local_files.size(); i++)
	{
		USR_FILE local_file = local_files[i];

		if (local_file.size == 0)
			continue;

		bool found = FALSE;

		for (int j = 0; j < remote_files.size(); j++)
		{
			USR_FILE remote_file = remote_files[j];

			if (strcmp(local_file.filename, remote_file.filename) == 0)
			{
				found = TRUE;
				if (local_file.last_modified > remote_file.last_modified)
				{
					UP_DOWN_COMMAND *sync_file = create_up_down_command(local_file.filename, SERVER_SYNC_UPLOAD, local_file.size, local_file.last_modified);

					sync_files->push_back(*sync_file);
				}
				else if (local_file.last_modified < remote_file.last_modified)
				{
					UP_DOWN_COMMAND *sync_file = create_up_down_command(local_file.filename, SERVER_SYNC_DOWNLOAD, local_file.size, local_file.last_modified);

					sync_files->push_back(*sync_file);
				}
				else if (local_file.last_modified == remote_file.last_modified)
				{
					UP_DOWN_COMMAND *sync_file = create_up_down_command(local_file.filename, SERVER_SYNC_KEEP_FILE, local_file.size, local_file.last_modified);

					sync_files->push_back(*sync_file);
				}
			}
		}

		if (!found)
		{
			UP_DOWN_COMMAND *sync_file = create_up_down_command(local_file.filename, SERVER_SYNC_UPLOAD, local_file.size, local_file.last_modified);

			sync_files->push_back(*sync_file);
		}
	}
}

void define_sync_remote_files(vector<UP_DOWN_COMMAND> *sync_files, vector<USR_FILE> local_files, vector<USR_FILE> remote_files)
{
	for (int i = 0; i < remote_files.size(); i++)
	{
		USR_FILE remote_file = remote_files[i];

		if (remote_file.size == 0)
			continue;

		bool found = FALSE;

		for (int j = 0; j < sync_files->size(); j++)
		{
			UP_DOWN_COMMAND mapped_file = sync_files->at(j);

			if (strcmp(mapped_file.filename, remote_file.filename) == 0)
			{
				found = TRUE;
				break;
			}
		}

		if (!found)
		{
			UP_DOWN_COMMAND *sync_file = create_up_down_command(remote_file.filename, SERVER_SYNC_DOWNLOAD, remote_file.size, remote_file.last_modified);

			sync_files->push_back(*sync_file);
		}
	}
}

vector<UP_DOWN_COMMAND> define_sync_files(vector<USR_FILE> local_files, vector<USR_FILE> remote_files)
{
	vector<UP_DOWN_COMMAND> sync_files;

	define_sync_local_files(&sync_files, local_files, remote_files);

	define_sync_remote_files(&sync_files, local_files, remote_files);

	return sync_files;
}

void sync_files_procedure_srv(int sock_fd, char *user_directory)
{
	vector<USR_FILE> remote_files = receive_files_list(sock_fd);

	vector<USR_FILE> local_files = *list_files(user_directory);

	vector<UP_DOWN_COMMAND> sync_files = define_sync_files(local_files, remote_files);

	srv_sync_files_list(sock_fd, sync_files, user_directory);
}

int srv_handle_procedure(int sock_fd, PROCEDURE_SELECT *procedure, char *user_directory)
{
	switch (procedure->proc_id)
	{
	case PROCEDURE_NOP:
		// printf("NOP\n");
		break;
	case PROCEDURE_SYNC_FILES:
		printf("Syncing files...\n\n");
		sync_files_procedure_srv(sock_fd, user_directory);
		last_sync[sock_fd] = get_now();
		break;
	case PROCEDURE_LIST_SERVER:
		printf("Listing server files...\n\n");
		send_files_list(sock_fd, user_directory);
		break;
	case PROCEDURE_UPLOAD_TO_SERVER:
		get_sync_dir_control(user_directory);

		printf("Receiving file...\n\n");
		receive_single_file(sock_fd, user_directory);

		release_sync_dir_control(user_directory);
		break;
	case PROCEDURE_DOWNLOAD_FROM_SERVER:
	{
		get_sync_dir_control(user_directory);

		printf("Sending file...\n\n");

		DATA_RETURN data = receive_data_with_packets(sock_fd);

		char *data_recovered = data.data;
		int bytes_size = data.bytes_size;

		DESIRED_FILE *desired_file = (DESIRED_FILE *)data_recovered;

		send_single_file(sock_fd, desired_file->filename, user_directory, SERVER_SYNC_UPLOAD);

		release_sync_dir_control(user_directory);
	}
	break;

	case PROCEDURE_EXIT:
		printf("Exiting...\n\n");
		return TRUE;
		break;
	}
	return FALSE;
}

int select_procedure(int sock_fd)
{
	if (get_now() - last_sync[sock_fd] > SYNC_WAIT)
	{
		printf("Need to sync...\n");
		return PROCEDURE_SYNC_FILES;
	}
	return PROCEDURE_NOP;
}

void srv_turn(int sock_fd, char *user_directory)
{
	int proc_id = select_procedure(sock_fd);
	PROCEDURE_SELECT *procedure = send_procedure(sock_fd, proc_id);
	// printf("Selected procedure: %d - ", proc_id);
	srv_handle_procedure(sock_fd, procedure, user_directory);
}

void srv_connection_loop(int sock_fd, char *username, char *user_directory)
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
			PROCEDURE_SELECT *procedure = receive_procedure(sock_fd);
			// printf("Received procedure: %d - ", procedure->proc_id);
			bool EXIT_NOW = srv_handle_procedure(sock_fd, procedure, user_directory);
			if (EXIT_NOW)
			{
				return;
			}
		}
		break;
		case SRV_TURN:
		{
			srv_turn(sock_fd, user_directory);
		}
		break;
		}

		sleep(1);
		turn = (turn == CLI_TURN) ? SRV_TURN : CLI_TURN;
	}
}

char *get_username(int sock_fd)
{
	LOGIN *login;
	printf("Waiting for username...\n");
	char *buffer = read_all_bytes(sock_fd, sizeof(LOGIN));
	printf("Username received\n");

	if (buffer == NULL)
	{
		printf("Connection %d closed\n\n", sock_fd);
		close(sock_fd);
		return NULL;
	}

	login = (LOGIN *)buffer;

	return login->username;
}

void *start_connection(void *data)
{
	try
	{
		CONNECTION_DATA *conn_data = (CONNECTION_DATA *)data;

		int connection_id = conn_data->connection_id;
		int sock_fd = conn_data->sock_fd;

		char *username = get_username(sock_fd);

		char *user_directory = (char *)malloc(MAX_PATH_SIZE);

		printf("Con #%d - logged in as %s\n", connection_id, username);

		strcpy(user_directory, mount_base_path(username, SERVER_BASE_DIR));
		create_folder_if_not_exists(user_directory);
		printf("User directory: %s\n\n", user_directory);

		srv_connection_loop(sock_fd, username, user_directory);

		printf("Connection %d closed\n\n", conn_data->sock_fd);
		close(conn_data->sock_fd);
	}
	catch (exception e)
	{
		printf("Exception: %s\n", e.what());
	}
	return NULL;
}

// returns socket file descriptor
int inicializar_servidor(int port)
{
	struct sockaddr_in serv_addr;
	char buffer[BUFFER_SIZE];
	int sock_fd;

	sock_fd = socket(AF_INET, SOCK_STREAM, SOCKET_DEFAULT_PROTOCOL);

	if (sock_fd == -1)
	{
		printf("ERROR opening socket\n\n");
		exit(0);
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	bzero(&(serv_addr.sin_zero), BYTE_SIZE);

	int bind_return = bind(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

	if (bind_return < 0)
	{
		printf("ERROR on binding\n\n");
		exit(0);
	}

	listen(sock_fd, MAX_WAITING_CONNECTIONS);

	printf("Server listening on port %d\n\n", port);

	global_sock_fd = sock_fd;
	return sock_fd;
}

void gerenciador_de_conexoes(int sock_fd)
{
	int new_conn_sock_fd;
	struct sockaddr_in cli_addr;
	socklen_t cli_len;

	cli_len = sizeof(struct sockaddr_in);

	while (TRUE)
	{
		new_conn_sock_fd = accept(sock_fd, (struct sockaddr *)&cli_addr, &cli_len);
		if (new_conn_sock_fd == -1)
		{
			printf("ERROR on accept\n");
		}
		else
		{
			pthread_t connection_thread;
			CONNECTION_DATA *new_conn_data = (CONNECTION_DATA *)malloc(sizeof(CONNECTION_DATA));

			new_conn_data->sock_fd = new_conn_sock_fd;
			new_conn_data->connection_id = connection_id;
			connection_id++;

			printf("Connection %d accepted\n\n", new_conn_data->connection_id);

			pthread_create(&connection_thread, NULL, start_connection, (void *)new_conn_data);
		}
	}
}

int main(int argc, char *argv[])
{
	int sock_fd;

	int port = argc > 1 ? atoi(argv[1]) : PORT;

	sock_fd = inicializar_servidor(port);

	gerenciador_de_conexoes(sock_fd);

	close(sock_fd);
	return 0;
}
