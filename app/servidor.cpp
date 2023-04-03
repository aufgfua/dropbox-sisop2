#include "shared.h"

#define LEAVE_KEY 'q'

int connection_id = 0;
int global_sock_fd;

map<int, long> last_sync;

void sync_files_procedure_srv(int sock_fd, char *user_directory)
{
	vector<USR_FILE> remote_files = receive_files_list(sock_fd);

	vector<USR_FILE> local_files = *list_files(user_directory);

	vector<UP_DOWN_COMMAND> sync_files = define_sync_files(local_files, remote_files);

	srv_sync_files_list(sock_fd, sync_files, user_directory);

	vector<UP_DOWN_COMMAND> sync_different_files;
	for (int i = 0; i < sync_files.size(); i++)
	{
		if (sync_files[i].sync_type == SERVER_SYNC_UPLOAD || sync_files[i].sync_type == SERVER_SYNC_DOWNLOAD)
		{
			sync_different_files.push_back(sync_files[i]);
		}
	}

	send_files_to_all_rms(sync_different_files);
}

int srv_handle_procedure(int sock_fd, PROCEDURE_SELECT *procedure, char *user_directory)
{
	switch (procedure->proc_id)
	{
	case PROCEDURE_NOP:

		break;
	case PROCEDURE_SYNC_FILES:
		cout << sock_fd << " - Syncing files..." << endl
			 << endl;
		sync_files_procedure_srv(sock_fd, user_directory);
		last_sync[sock_fd] = get_now();
		send_OK_packet(sock_fd, OK_PACKET_QUIET);
		break;
	case PROCEDURE_LIST_SERVER:
		cout << sock_fd << " - Listing server files..." << endl
			 << endl;
		send_files_list(sock_fd, user_directory);
		break;
	case PROCEDURE_UPLOAD_TO_SERVER:
	{
		get_sync_dir_control(user_directory);

		cout << sock_fd << " - Receiving file..." << endl
			 << endl;

		UP_DOWN_COMMAND *new_file_up_down_command = receive_single_file(sock_fd, user_directory);

		if (new_file_up_down_command != NULL)
		{

			vector<UP_DOWN_COMMAND> rm_sync_up_down_command_vector;
			rm_sync_up_down_command_vector.push_back(*new_file_up_down_command);
			send_files_to_all_rms(rm_sync_up_down_command_vector);
		}
		release_sync_dir_control(user_directory);
		send_OK_packet(sock_fd, OK_PACKET_QUIET);
	}
	break;
	case PROCEDURE_DOWNLOAD_FROM_SERVER:
	{
		get_sync_dir_control(user_directory);

		cout << sock_fd << " - Sending file..." << endl
			 << endl;

		DATA_RETURN data = receive_data_with_packets(sock_fd);

		char *data_recovered = data.data;
		int bytes_size = data.bytes_size;

		DESIRED_FILE *desired_file = (DESIRED_FILE *)data_recovered;

		UP_DOWN_COMMAND *new_file_up_down_command = send_single_file(sock_fd, desired_file->filename, user_directory, SERVER_SYNC_UPLOAD);

		if (new_file_up_down_command != NULL)
		{

			vector<UP_DOWN_COMMAND> rm_sync_up_down_command_vector;
			rm_sync_up_down_command_vector.push_back(*new_file_up_down_command);
			send_files_to_all_rms(rm_sync_up_down_command_vector);
		}
		release_sync_dir_control(user_directory);
		send_OK_packet(sock_fd, OK_PACKET_QUIET);
	}
	break;

	case PROCEDURE_EXIT:
		cout << sock_fd << " - Exiting..." << endl
			 << endl;
		send_OK_packet(sock_fd, OK_PACKET_QUIET);
		this_thread::sleep_for(chrono::milliseconds(2 * 1000));
		return TRUE;
		break;
	}
	return FALSE;
}

int select_procedure(int sock_fd)
{
	if (get_now() - last_sync[sock_fd] > SYNC_WAIT)
	{
		cout << sock_fd << " - Need to sync..." << endl;
		return PROCEDURE_SYNC_FILES;
	}
	return PROCEDURE_NOP;
}

void srv_turn(int sock_fd, char *user_directory)
{
	int proc_id = select_procedure(sock_fd);
	PROCEDURE_SELECT *procedure = send_procedure(sock_fd, proc_id);

	srv_handle_procedure(sock_fd, procedure, user_directory);
}

void srv_connection_loop(int sock_fd, char *username)
{
	char turn = START_TURN;

	char *user_directory = mount_base_path(username, SERVER_BASE_DIR);
	create_folder_if_not_exists(user_directory);
	cout << "User directory: " << user_directory << endl
		 << endl;

	started_connection_loop = TRUE;
	while (TRUE)
	{
		while (!can_sync_clients)
		{
			this_thread::sleep_for(chrono::milliseconds(500));
		}
		increment_syncing_clients();
		try
		{

			switch (turn)
			{
			case CLI_TURN:
			{
				PROCEDURE_SELECT *procedure = receive_procedure(sock_fd);
				cout << "Procedure: " << procedure->proc_id << endl
					 << endl;

				bool EXIT_NOW = srv_handle_procedure(sock_fd, procedure, user_directory);
				if (EXIT_NOW)
				{
					cout << "Exit loop now - " << sock_fd << endl
						 << endl;
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

			share_connection_data_with_all();

			this_thread::sleep_for(chrono::milliseconds(1 * 1000));
			turn = (turn == CLI_TURN) ? SRV_TURN : CLI_TURN;
		}
		catch (OutOfSyncException e)
		{
			cout << "Out of sync exception: " << e.what() << endl;
		}
		decrement_syncing_clients();
	}
}

char *get_username(int sock_fd)
{
	LOGIN *login;
	cout << "Waiting for username..." << endl;
	char *buffer = (char *)receive_converted_data_with_packets<LOGIN>(sock_fd);
	cout << "Username received" << endl;

	if (buffer == NULL)
	{
		cout << "Connection " << sock_fd << " closed" << endl
			 << endl;
		close(sock_fd);
		return NULL;
	}

	login = (LOGIN *)buffer;

	return login->username;
}

void *start_connection(void *data)
{
	CONNECTION_DATA *conn_data = (CONNECTION_DATA *)data;
	int connection_id = conn_data->connection_id;
	int sock_fd = conn_data->sock_fd;

	try
	{
		char *username = get_username(sock_fd);
		strcpy(conn_data->username, username);

		// save to the global list of connections - TODO cap connections at 2 at most

		insert_client_connection(conn_data);

		cout << "Con #" << connection_id << " - logged in as " << username << endl;

		srv_connection_loop(sock_fd, username);

		cout << "Connection " << conn_data->sock_fd << " closed" << endl
			 << endl;
		// close(conn_data->sock_fd);
	}
	catch (ConnectionLostException e)
	{
		cout << "Connection lost exception: " << e.what() << endl;
		remove_client_connection(sock_fd);
		close(sock_fd);
	}
	catch (exception e)
	{
		cout << "Exception managing socket: " << sock_fd << "  -  " << e.what() << endl;
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
			cout << "ERROR on accept" << endl;
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

			cout << "Connection " << new_conn_data->connection_id << " accepted" << endl
				 << endl;

			pthread_create(&connection_thread, NULL, start_connection, (void *)new_conn_data);
		}
	}
}

// returns socket file descriptor

void primary_replica_manager_start(int port)
{
	int sock_fd;

	new_rm_connections_port = port + NEW_RM_CONNECTIONS_PORT_OFFSET;

	sock_fd = init_server(port);
	global_sock_fd = sock_fd;

	cout << "RM Server -> ";
	new_rm_connections_sock_fd = init_server(new_rm_connections_port);

	pthread_t new_rm_connections_thread;
	pthread_create(&new_rm_connections_thread, NULL, manage_new_rm_connections, (void *)&new_rm_connections_sock_fd);

	pthread_t primary_rm_heartbeat_thread;
	int heartbeat_port = port + HEARTBEAT_PORT_OFFSET;
	pthread_create(&primary_rm_heartbeat_thread, NULL, start_heartbeat_primary_rm, (void *)&heartbeat_port);

	manage_connections(sock_fd);

	close(sock_fd);
	return;
}

int main(int argc, char *argv[])
{

	if (argc != 2 && argc != 4)
	{
		cout << "Usage:\n	> " << argv[0] << " <port> // to run as primary RM" << endl
			 << "	" << argv[0] << " <port> <primary_server_ip> <primary_server_port> // to run as secondary RM" << endl;
	}

	if (argc == 2)
	{
		int port = argc > 1 ? atoi(argv[1]) : SERVER_PORT;
		primary_replica_manager_start(port);
		return 0;
	}
	else if (argc == 4)
	{
		int port = atoi(argv[1]);
		struct hostent *main_server = gethostbyname(argv[2]);
		int main_server_port = atoi(argv[3]);

		secondary_replica_manager_start(port, main_server, main_server_port);
		return 0;
	}
}
