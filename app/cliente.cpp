#include "shared.h"

#define BUFFER_SIZE 256
#define BYTE_SIZE 8
#define SOCKET_DEFAULT_PROTOCOL 0
#define bool int

typedef struct STR_CLI_CONNECTION_DATA
{
	int sock_fd;
	char username[MAX_USERNAME_SIZE];
} CLI_CONNECTION_DATA;

queue<int>
	orders;

pthread_t fe_thread;
pthread_t cli_connection_thread;

char *user_directory = (char *)malloc(MAX_PATH_SIZE);

long last_sync = 0;

string upload_target;
string download_target;

void *get_console_input_order_loop(void *data);

void cli_handle_procedure(int sock_fd, PROCEDURE_SELECT *procedure)
{
	switch (procedure->proc_id)
	{
	case PROCEDURE_NOP:
		// printf("NOP\n");
		break;

	case PROCEDURE_SYNC_FILES:
		// printf("Syncing files...\n\n");
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

		char current_dir[MAX_PATH_SIZE];
		strcpy(current_dir, get_current_path());

		send_single_file(sock_fd, upload_target.c_str(), current_dir, CLIENT_SYNC_UPLOAD);

		orders.push(PROCEDURE_SYNC_FILES);
	}
	break;
	case PROCEDURE_DOWNLOAD_FROM_SERVER:
	{
		printf("Downloading %s from server...\n\n", download_target.c_str());
		char current_dir[MAX_PATH_SIZE];
		strcpy(current_dir, get_current_path());

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

void cli_turn(int sock_fd)
{
	int proc_id = select_procedure(sock_fd);
	// printf("Selected procedure: %d - ", proc_id);
	PROCEDURE_SELECT *procedure = send_procedure(sock_fd, proc_id);
	cli_handle_procedure(sock_fd, procedure);
}

void *cli_connection_loop(void *data)
{
	CLI_CONNECTION_DATA *cli_conn_data = (CLI_CONNECTION_DATA *)data;
	int sock_fd = cli_conn_data->sock_fd;
	char *username = cli_conn_data->username;

	char turn = START_TURN;

	uint32_t run = 0;

	started_connection_loop = TRUE;
	while (TRUE)
	{
		try
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
		catch (OutOfSyncException e)
		{
			printf("Out of sync exception: %s\n", e.what());
		}
	}

	close(sock_fd);
}

void send_username(int sock_fd, char *username)
{
	LOGIN login;
	strcpy(login.username, username);
	send_data_with_packets(sock_fd, (char *)&login, sizeof(LOGIN));

	printf("Logged in as %s\n", username);
}

void manage_server_connection(int sock_fd, char *username)
{
	send_username(sock_fd, username);

	strcpy(user_directory, mount_base_path(username, CLIENT_BASE_DIR));
	create_folder_if_not_exists(user_directory);
	printf("User directory: %s\n\n", user_directory);

	CLI_CONNECTION_DATA cli_conn_data;
	cli_conn_data.sock_fd = sock_fd;
	strcpy(cli_conn_data.username, username);

	pthread_create(&cli_connection_thread, NULL, cli_connection_loop, (void *)&cli_conn_data);
}

int connect_socket(struct hostent *server, int port)
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

	int conn_return = -1;
	while (conn_return < 0)
	{
		conn_return = connect(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
		sleep(0.5);
	}

	if (conn_return < 0)
	{
		printf("ERROR connecting\n");
		exit(0);
	}

	return sock_fd;
}

void run_frontend(int fe_port, char *srv_ip, int srv_port)
{
	FE_SERVER_ADDRESS fe_srv_address;
	strcpy(fe_srv_address.ip_addr, srv_ip);
	fe_srv_address.port = srv_port;

	FE_RUN_DATA fe_run_data;
	fe_run_data.fe_port = fe_port;
	fe_run_data.srv_address = fe_srv_address;

	pthread_create(&fe_thread, NULL, frontend_main, (void *)&fe_run_data);
	pthread_detach(fe_thread);
}

int main(int argc, char *argv[])
{
	if (argc < 4)
	{
		fprintf(stderr, "usage '%s <username> <server_ip_address> <port> [frontend_port]'\n", argv[0]);
		exit(0);
	}

	int sock_fd;

	char *username;
	int srv_port, fe_port;
	struct hostent *server;
	char server_ip_string[MAX_IP_SIZE];

	username = argv[1];
	strcpy(server_ip_string, argv[2]);
	server = gethostbyname(argv[2]);
	srv_port = atoi(argv[3]);
	fe_port = argc > 4 ? atoi(argv[4]) : FE_PORT;

	run_frontend(fe_port, server_ip_string, srv_port);

	sock_fd = connect_socket(server, fe_port);

	printf("Connected to server\n");

	pthread_t input_thread;
	pthread_create(&input_thread, NULL, get_console_input_order_loop, (void *)NULL);

	manage_server_connection(sock_fd, username);

	pthread_join(cli_connection_thread, NULL);
	pthread_join(fe_thread, NULL);
}

void *get_console_input_order_loop(void *arg)
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