
#define NEW_RM_CONNECTIONS_PORT_OFFSET 1000

typedef struct STR_CONNECTION_DATA
{
    unsigned long cli_s_addr;
    int sock_fd, connection_id;
    char username[MAX_USERNAME_SIZE];
} CONNECTION_DATA;

typedef struct STR_RM_CONNECTION
{
    unsigned long s_addr;
    int sock_fd;
    in_port_t port;
} RM_CONNECTION;

vector<CONNECTION_DATA> connections;
mutex connections_list_mtx;

vector<RM_CONNECTION> rm_connections;
mutex rm_connections_list_mtx;

void insert_rm_connection(RM_CONNECTION *rm_conn)
{
    rm_connections_list_mtx.lock();
    rm_connections.push_back(*rm_conn);
    rm_connections_list_mtx.unlock();
}

void remove_rm_connection(int sock_fd)
{
    rm_connections_list_mtx.lock();
    for (int i = 0; i < rm_connections.size(); i++)
    {
        if (rm_connections[i].sock_fd == sock_fd)
        {
            rm_connections.erase(rm_connections.begin() + i);
            break;
        }
    }
    rm_connections_list_mtx.unlock();
}

void remove_rm_connection_s_addr(unsigned long s_addr)
{
    rm_connections_list_mtx.lock();
    for (int i = 0; i < rm_connections.size(); i++)
    {
        if (rm_connections[i].s_addr == s_addr)
        {
            close(rm_connections[i].sock_fd);
            cout << "Closed connection with RM " << inet_ntoa(*(struct in_addr *)&rm_connections[i].s_addr) << " => RM-FD:" << rm_connections[i].sock_fd << endl;
            rm_connections.erase(rm_connections.begin() + i);
            break;
        }
    }
    rm_connections_list_mtx.unlock();
}

void insert_client_connection(CONNECTION_DATA *conn_data)
{
    connections_list_mtx.lock();
    connections.push_back(*conn_data);
    connections_list_mtx.unlock();
}

void remove_client_connection(int sock_fd)
{
    connections_list_mtx.lock();
    for (int i = 0; i < connections.size(); i++)
    {
        if (connections[i].sock_fd == sock_fd)
        {
            connections.erase(connections.begin() + i);
            break;
        }
    }
    connections_list_mtx.unlock();
}

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
                if (local_file.last_modified > remote_file.last_modified && local_file.size > 0)
                {
                    UP_DOWN_COMMAND *sync_file = create_up_down_command(local_file.filename, SERVER_SYNC_UPLOAD, local_file.size, local_file.last_modified, local_file.path);

                    sync_files->push_back(*sync_file);
                }
                else if (local_file.last_modified < remote_file.last_modified && remote_file.size > 0)
                {
                    UP_DOWN_COMMAND *sync_file = create_up_down_command(remote_file.filename, SERVER_SYNC_DOWNLOAD, remote_file.size, remote_file.last_modified, remote_file.path);

                    sync_files->push_back(*sync_file);
                }
                else if (local_file.last_modified == remote_file.last_modified)
                {
                    UP_DOWN_COMMAND *sync_file = create_up_down_command(local_file.filename, SERVER_SYNC_KEEP_FILE, local_file.size, local_file.last_modified, local_file.path);

                    sync_files->push_back(*sync_file);
                }
            }
        }

        if (!found)
        {
            UP_DOWN_COMMAND *sync_file = create_up_down_command(local_file.filename, SERVER_SYNC_UPLOAD, local_file.size, local_file.last_modified, local_file.path);

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
            UP_DOWN_COMMAND *sync_file = create_up_down_command(remote_file.filename, SERVER_SYNC_DOWNLOAD, remote_file.size, remote_file.last_modified, remote_file.path);

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

char *get_last_folder_name(char *file_path_chars)
{
    char *char_folder_name = (char *)malloc(MAX_FILENAME_SIZE);

    string file_path(file_path_chars);
    size_t pos = file_path.find_last_of("/");
    if (pos == string::npos)
    {
        cout << "Error - couldn't find last folder name" << endl;
        strcpy(char_folder_name, file_path.c_str());
        return char_folder_name;
    }
    else if (pos == file_path.length() - 1)
    {
        pos = file_path.find_last_of("/", pos - 1);
    }

    strcpy(char_folder_name, file_path.substr(pos + 1).c_str());
    return char_folder_name;
}
