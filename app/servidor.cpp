#include "shared.h"

#define BUFFER_SIZE 256
#define MAX_WAITING_CONNECTIONS 10
#define BYTE_SIZE 8
#define SOCKET_DEFAULT_PROTOCOL 0
#define bool int
#define LEAVE_KEY 'q'

typedef struct STR_CONNECTION_DATA
{
	unsigned long cli_s_addr;
	int sock_fd, connection_id;
	char username[MAX_USERNAME_SIZE];
} CONNECTION_DATA;

int connection_id = 0;
int global_sock_fd;

map<int, long> last_sync;

mutex connections_list_mtx;
vector<CONNECTION_DATA> connections;

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
		printf("%d - Syncing files...\n\n", sock_fd);
		sync_files_procedure_srv(sock_fd, user_directory);
		last_sync[sock_fd] = get_now();
		break;
	case PROCEDURE_LIST_SERVER:
		printf("%d - Listing server files...\n\n", sock_fd);
		send_files_list(sock_fd, user_directory);
		break;
	case PROCEDURE_UPLOAD_TO_SERVER:
		get_sync_dir_control(user_directory);

		printf("%d - Receiving file...\n\n", sock_fd);
		receive_single_file(sock_fd, user_directory);

		release_sync_dir_control(user_directory);
		break;
	case PROCEDURE_DOWNLOAD_FROM_SERVER:
	{
		get_sync_dir_control(user_directory);

		printf("%d - Sending file...\n\n", sock_fd);

		DATA_RETURN data = receive_data_with_packets(sock_fd);

		char *data_recovered = data.data;
		int bytes_size = data.bytes_size;

		DESIRED_FILE *desired_file = (DESIRED_FILE *)data_recovered;

		send_single_file(sock_fd, desired_file->filename, user_directory, SERVER_SYNC_UPLOAD);

		release_sync_dir_control(user_directory);
	}
	break;

	case PROCEDURE_EXIT:
		printf("%d - Exiting...\n\n", sock_fd);
		return TRUE;
		break;
	}
	return FALSE;
}

int select_procedure(int sock_fd)
{
	if (get_now() - last_sync[sock_fd] > SYNC_WAIT)
	{
		printf("%d - Need to sync...\n", sock_fd);
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

void srv_connection_loop(int sock_fd, char *username)
{
	char turn = START_TURN;

	char *user_directory = mount_base_path(username, SERVER_BASE_DIR);
	create_folder_if_not_exists(user_directory);
	printf("User directory: %s\n\n", user_directory);

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
	char *buffer = (char *)receive_converted_data_with_packets<LOGIN>(sock_fd);
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
		strcpy(conn_data->username, username);

		// save to the global list of connections - TODO cap connections at 2 at most
		connections_list_mtx.lock();
		connections.push_back(*conn_data);
		connections_list_mtx.unlock();

		printf("Con #%d - logged in as %s\n", connection_id, username);

		srv_connection_loop(sock_fd, username);

		printf("Connection %d closed\n\n", conn_data->sock_fd);
		close(conn_data->sock_fd);
	}
	catch (exception e)
	{
		printf("Exception: %s\n", e.what());
	}
	return NULL;
}

void manage_connections(int sock_fd)
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
			// print all fields from cli_addr
			// TODO check if this connects correctly
			// cout << "Connection from " << cli_addr.sin_addr.s_addr << ":" << ntohs(cli_addr.sin_port) << " accepted" << endl;

			pthread_t connection_thread;
			CONNECTION_DATA *new_conn_data = (CONNECTION_DATA *)malloc(sizeof(CONNECTION_DATA));

			new_conn_data->sock_fd = new_conn_sock_fd;
			new_conn_data->connection_id = connection_id;
			new_conn_data->cli_s_addr = cli_addr.sin_addr.s_addr;
			connection_id++;

			printf("Connection %d accepted\n\n", new_conn_data->connection_id);

			pthread_create(&connection_thread, NULL, start_connection, (void *)new_conn_data);
		}
	}
}

// returns socket file descriptor
int init_server(int port)
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

int main(int argc, char *argv[])
{
	int sock_fd;

	int port = argc > 1 ? atoi(argv[1]) : SERVER_PORT;

	sock_fd = init_server(port);

	manage_connections(sock_fd);

	close(sock_fd);
	return 0;
}
