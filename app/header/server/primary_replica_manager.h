int new_rm_connections_sock_fd;
int new_rm_connections_port;

void *handle_new_rm_connection(void *data)
{
    RM_CONNECTION *rm_data = (RM_CONNECTION *)data;
    int sock_fd = rm_data->sock_fd;
    int port = rm_data->port;
    int s_addr = rm_data->s_addr;

    cout << "New RM connection " << sock_fd << " handled" << endl
         << endl;
    sleep(10);

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
            rm_connections.push_back(*new_rm_data);

            pthread_create(&new_rm_connection_thread, NULL, handle_new_rm_connection, (void *)new_rm_data);
        }
    }

    return NULL;
}
