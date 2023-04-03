char *get_LOCAL_FIX(int sock_fd)
{
    char LOCAL_FIX[MAX_PATH_SIZE];
    strcpy(LOCAL_FIX, "/RM-");

    int ASC_zero = 48;
    int sock_fd_to_asc_int = sock_fd + ASC_zero;
    char sock_fd_to_asc[2];
    sock_fd_to_asc[0] = sock_fd_to_asc_int;
    sock_fd_to_asc[1] = 0;
    strcat(LOCAL_FIX, sock_fd_to_asc);

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
    files_download_loop(sock_fd);
    cout << "Replication finished" << endl;
}

void secondary_replica_manager_start(int port, struct hostent *main_server, int primary_rm_server_port)
{

    int sock_fd, heartbeat_sock_fd;

    int primary_rm_handler_port = primary_rm_server_port + NEW_RM_CONNECTIONS_PORT_OFFSET;
    sock_fd = connect_socket(main_server, primary_rm_handler_port);

    int primary_heartbeat_server_port = primary_rm_server_port + HEARTBEAT_PORT_OFFSET;
    heartbeat_sock_fd = start_heartbeat_secondary_rm(main_server, primary_heartbeat_server_port);
    pthread_t heartbeat_thread;
    pthread_create(&heartbeat_thread, NULL, secondary_heartbeat_loop, (void *)&heartbeat_sock_fd);

    cout << "RM Server -> ";
    secondary_rm_replicate_state(sock_fd);

    while (TRUE)
    {
        sleep(1);
    }
    close(sock_fd);
    return;
}

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