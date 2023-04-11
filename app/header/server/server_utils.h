
#define NEW_RM_CONNECTIONS_PORT_OFFSET 1000

#define RM_PROC_FILE 1
#define RM_PROC_CONTROL_DATA 2

typedef struct STR_RM_PROCEDURE_SELECT
{
    int procedure_id;
} RM_PROCEDURE_SELECT;

typedef struct STR_CONNECTION_DATA
{
    unsigned long cli_s_addr;
    int sock_fd, connection_id;
    char username[MAX_USERNAME_SIZE];
} CONNECTION_DATA;
void print_connection_data(CONNECTION_DATA connection_data)
{
    cout << "Connection data: ";
    cout << " Connection ID: " << connection_data.connection_id;
    cout << ", Socket FD: " << connection_data.sock_fd;
    cout << ", Username: " << connection_data.username;
    cout << ", Client IP: " << inet_ntoa(*(struct in_addr *)&connection_data.cli_s_addr) << endl
         << endl;
}

void print_connections_data(vector<CONNECTION_DATA> connections)
{
    for (CONNECTION_DATA conn : connections)
    {
        print_connection_data(conn);
    }
}

typedef struct STR_RM_CONNECTION
{
    unsigned long s_addr;
    int sock_fd;
    in_port_t port;
    in_port_t rm_reconnection_port;
    int id;
    char str_ip[INET_ADDRSTRLEN];
} RM_CONNECTION;
void print_rm_connection(RM_CONNECTION rm_connection)
{
    cout << "RM Connection data: ";
    cout << " Socket FD: " << rm_connection.sock_fd;
    cout << ", RM IP: " << inet_ntoa(*(struct in_addr *)&rm_connection.s_addr);
    cout << ", RM Port: " << rm_connection.port << endl
         << " RM ID: " << rm_connection.id
         << ", RM String IP: " << rm_connection.str_ip
         << endl
         << endl;
}

void print_rm_connections(vector<RM_CONNECTION> rm_connections)
{
    for (RM_CONNECTION rm_conn : rm_connections)
    {
        print_rm_connection(rm_conn);
    }
}

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

RM_CONNECTION *get_next_greatest_id_rm_connection(int id)
{
    rm_connections_list_mtx.lock();
    bool found = FALSE;

    if (rm_connections.size() == 0 || rm_connections.size() == 1)
    {
        rm_connections_list_mtx.unlock();
        return NULL;
    }

    sort(rm_connections.begin(), rm_connections.end(), [](const RM_CONNECTION &a, const RM_CONNECTION &b)
         { if(a.id != b.id) return a.id < b.id;
        return a.s_addr < b.s_addr; });

    int chosen_index = 0;
    for (RM_CONNECTION rm_conn : rm_connections)
    {
        if (rm_conn.id > id)
        {
            found = TRUE;
            break;
        }
        chosen_index++;
    }

    rm_connections_list_mtx.unlock();

    if (!found)
    {
        cout << "NOT FOUND - Returning first RM connection" << endl;
        return &rm_connections.at(0);
    }

    cout << "FOUND - Returning RM connection with ID: " << (&rm_connections.at(chosen_index))->id << endl;
    return &rm_connections.at(chosen_index);
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

int get_rm_connection_greatest_id()
{
    int greatest_id = 0;
    rm_connections_list_mtx.lock();
    for (RM_CONNECTION rm_conn : rm_connections)
    {
        if (rm_conn.id > greatest_id)
            greatest_id = rm_conn.id;
    }
    rm_connections_list_mtx.unlock();
    return greatest_id;
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
        cout << "Error - couldn't find last folder name on : " << file_path << endl;

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

void send_rm_procedure_select(int sock_fd, int procedure_id)
{
    RM_PROCEDURE_SELECT *rm_proc_select = (RM_PROCEDURE_SELECT *)malloc(sizeof(RM_PROCEDURE_SELECT));
    rm_proc_select->procedure_id = procedure_id;

    send_data_with_packets(sock_fd, (char *)rm_proc_select, sizeof(RM_PROCEDURE_SELECT));
}

RM_PROCEDURE_SELECT *receive_rm_procedure_select(int sock_fd)
{
    RM_PROCEDURE_SELECT *rm_proc_select = receive_converted_data_with_packets<RM_PROCEDURE_SELECT>(sock_fd);

    return rm_proc_select;
}