

int my_random_id[3];

void generate_random_id()
{
    srand(time(NULL));
    my_random_id[0] = rand() % 10;
    my_random_id[1] = rand() % 10;
    my_random_id[2] = rand() % 10;
}

char digit_to_asc(int digit)
{
    int ASC_zero = 48;
    return digit + ASC_zero;
}
char *get_LOCAL_FIX(int sock_fd)
{
    char LOCAL_FIX[MAX_PATH_SIZE];
    strcpy(LOCAL_FIX, "/RM-");

    char my_path_id[5];
    my_path_id[0] = digit_to_asc(sock_fd);
    my_path_id[1] = digit_to_asc(my_random_id[0]);
    my_path_id[2] = digit_to_asc(my_random_id[1]);
    my_path_id[3] = digit_to_asc(my_random_id[2]);
    my_path_id[4] = 0;

    strcat(LOCAL_FIX, my_path_id);

    char *LOCAL_FIX_ptr = LOCAL_FIX;
    return LOCAL_FIX_ptr;
}

char *get_rm_folders_path(int sock_fd)
{
    char *folders_def_path = (char *)malloc(MAX_PATH_SIZE);

    strcpy(folders_def_path, get_LOCAL_FIX(sock_fd));

    strcat(folders_def_path, SERVER_ALL_FOLDERS_DIR);
    return folders_def_path;
}

void files_download_loop(int sock_fd)
{

    FILE_TRANSACTION_CONTROLLER *file_transaction_controller = receive_transaction_controller(sock_fd);

    while (file_transaction_controller->one_more)
    {
        UP_DOWN_COMMAND *up_down_command = receive_up_down_command(sock_fd);

        char *base_path = mount_srv_folders_path(get_rm_folders_path(sock_fd));

        char *folder_name = get_last_folder_name(up_down_command->path);

        char *final_path = (char *)malloc(MAX_FILENAME_SIZE);
        strcpy(final_path, base_path);
        strcat(final_path, folder_name);

        // cout << "Initial path: " << up_down_command->path << endl;
        // cout << "Creating folder: " << final_path << " to receive File: " << up_down_command->filename << endl;

        create_folder_if_not_exists(final_path);

        receive_file(sock_fd, up_down_command, final_path);

        file_transaction_controller = receive_transaction_controller(sock_fd);
    }
}

void secondary_rm_replicate_state(int sock_fd)
{
    cout << "My folder: " << get_rm_folders_path(sock_fd) << endl;
    files_download_loop(sock_fd);
    cout << "Replication finished" << endl;

    cout << "Start cloning changes" << endl;
    while (TRUE)
    {
        files_download_loop(sock_fd);
    }
}

void secondary_replica_manager_start(int port, struct hostent *main_server, int primary_rm_server_port)
{
    generate_random_id();

    int sock_fd, heartbeat_sock_fd;

    int primary_rm_handler_port = primary_rm_server_port + NEW_RM_CONNECTIONS_PORT_OFFSET;
    sock_fd = connect_socket(main_server, primary_rm_handler_port);

    int primary_heartbeat_server_port = primary_rm_server_port + HEARTBEAT_PORT_OFFSET;
    heartbeat_sock_fd = start_heartbeat_secondary_rm(main_server, primary_heartbeat_server_port);

    HEARTBEAT_CONNECTION *heartbeat_connection = (HEARTBEAT_CONNECTION *)malloc(sizeof(HEARTBEAT_CONNECTION));
    heartbeat_connection->rm_connection_sock_fd = sock_fd;
    heartbeat_connection->heartbeat_sock_fd = heartbeat_sock_fd;

    pthread_t heartbeat_thread;
    pthread_create(&heartbeat_thread, NULL, secondary_heartbeat_loop, (void *)heartbeat_connection);

    cout << "RM Server -> ";
    secondary_rm_replicate_state(sock_fd);

    while (TRUE)
    {
        this_thread::sleep_for(chrono::milliseconds(1 * 1000));
    }
    close(sock_fd);
    return;
}

// Not used for now
void create_replica_folders(vector<USR_FOLDER> folders, int sock_fd)
{

    char *base_path = mount_srv_folders_path(get_rm_folders_path(sock_fd));

    for (int i = 0; i < folders.size(); i++)
    {
        char final_path[MAX_FILENAME_SIZE];
        strcpy(final_path, base_path);
        strcat(final_path, folders[i].foldername);
        cout << "Creating folder: " << final_path << endl;
        create_folder_if_not_exists(final_path);
    }
}