int new_rm_connections_sock_fd;
int new_rm_connections_port;

vector<USR_FILE> get_all_files_rm_up_down_command()
{

    vector<USR_FILE> all_local_files;

    vector<USR_FOLDER> folders;
    char *srv_base_folder = mount_srv_folders_path(SERVER_ALL_FOLDERS_DIR);

    get_all_folders(srv_base_folder, folders);
    for (USR_FOLDER folder : folders)
    {
        char folder_path[MAX_FILENAME_SIZE];
        strcpy(folder_path, srv_base_folder);
        strcat(folder_path, folder.foldername);
        strcat(folder_path, "/");

        vector<USR_FILE> local_files = *list_files(folder_path);

        for (USR_FILE file : local_files)
        {
            all_local_files.push_back(file);
        }
    }

    return all_local_files;
}

void share_connection_data(int sock_fd)
{
    send_vector_of_data_with_packets(sock_fd, connections);
    send_vector_of_data_with_packets(sock_fd, rm_connections);
}

void share_connection_data_with_all()
{
    for (RM_CONNECTION rm : rm_connections)
    {
        try
        {
            cout << "Sending connection data to RM " << rm.sock_fd << endl;
            send_rm_procedure_select(rm.sock_fd, RM_PROC_CONTROL_DATA);
            share_connection_data(rm.sock_fd);
        }
        catch (ConnectionLostException e)
        {
            cout << "Connection Lost -> RM " << rm.sock_fd << " is down" << endl;
            remove_rm_connection(rm.sock_fd);
            close(rm.sock_fd);
        }
    }
}

void send_file_to_secondary_rm(int sock_fd, UP_DOWN_COMMAND *file)
{
    send_rm_procedure_select(sock_fd, RM_PROC_FILE);
    send_up_down_command(sock_fd, file);
    send_file(sock_fd, file, file->path);
}

void send_files_to_secondary_rm(int sock_fd, vector<UP_DOWN_COMMAND> files)
{
    for (UP_DOWN_COMMAND file : files)
    {
        send_file_to_secondary_rm(sock_fd, &file);
    }
}

void send_files_to_all_rms(vector<UP_DOWN_COMMAND> files)
{
    for (RM_CONNECTION rm : rm_connections)
    {
        try
        {
            send_files_to_secondary_rm(rm.sock_fd, files);
            cout << "Sent " << files.size() << " files to RM " << rm.sock_fd << endl;
        }
        catch (ConnectionLostException e)
        {
            cout << "Connection Lost -> RM " << rm.sock_fd << " is down" << endl;
            remove_rm_connection(rm.sock_fd);
            close(rm.sock_fd);
        }
    }

    cout << "Release!! Finish sending to all RMs" << endl;
}

void primary_rm_replicate_state(int sock_fd)
{
    vector<USR_FILE> all_local_files = get_all_files_rm_up_down_command();

    vector<USR_FILE> empty_remote_files;

    vector<UP_DOWN_COMMAND> sync_files = define_sync_files(all_local_files, empty_remote_files);

    // print_up_down_commands(sync_files);

    // cout << "Sending " << sync_files.size() << " files to RM" << endl;
    for (UP_DOWN_COMMAND file : sync_files)
    {
        send_transaction_controller(sock_fd, ONE_MORE_FILE);
        send_up_down_command(sock_fd, &file);
        send_file(sock_fd, &file, file.path);
    }
    send_transaction_controller(sock_fd, NO_MORE_FILES);
}

void primary_rm_replicate_state_controller(int sock_fd)
{
    cout << "Replicating state with RM - " << sock_fd << endl;

    increment_want_to_sync_RM();

    while (get_syncing_clients() > 0)
    {
        cout << "Waiting for clients to sync..." << endl;
        this_thread::sleep_for(chrono::milliseconds(1 * 1000));
    }

    primary_rm_replicate_state(sock_fd);

    decrement_want_to_sync_RM();
}

void *handle_new_rm_connection(void *data)
{
    RM_CONNECTION *rm_data = (RM_CONNECTION *)data;
    rm_data->id = get_rm_connection_greatest_id() + 1;

    int sock_fd = rm_data->sock_fd;
    int port = rm_data->port;
    unsigned long s_addr = rm_data->s_addr;

    insert_rm_connection(rm_data);
    send_data_with_packets(sock_fd, (char *)&(rm_data->id), sizeof(int));
    send_data_with_packets(sock_fd, (char *)&(rm_data->s_addr), sizeof(unsigned long));
    cout << "New RM connection " << sock_fd << " handled" << endl
         << endl;

    primary_rm_replicate_state_controller(sock_fd);

    return NULL;
}

void *manage_new_rm_connections(void *data)
{
    int *int_data = (int *)data;
    int sock_fd = *int_data;

    int new_conn_sock_fd;
    struct sockaddr_in new_rm_addr;
    socklen_t new_rm_len;

    new_rm_len = sizeof(struct sockaddr_in);

    while (TRUE)
    {

        new_conn_sock_fd = accept(sock_fd, (struct sockaddr *)&new_rm_addr, &new_rm_len);

        if (new_conn_sock_fd == -1)
        {
            cout << "ERROR on accept" << endl;
        }
        else
        {

            pthread_t new_rm_connection_thread;
            RM_CONNECTION *new_rm_data = (RM_CONNECTION *)malloc(sizeof(RM_CONNECTION));

            new_rm_data->sock_fd = new_conn_sock_fd;
            new_rm_data->s_addr = new_rm_addr.sin_addr.s_addr;
            new_rm_data->port = new_rm_addr.sin_port;

            cout << "New RM connection " << new_rm_data->sock_fd << " accepted" << endl
                 << endl;

            pthread_create(&new_rm_connection_thread, NULL, handle_new_rm_connection, (void *)new_rm_data);
        }
    }

    return NULL;
}
