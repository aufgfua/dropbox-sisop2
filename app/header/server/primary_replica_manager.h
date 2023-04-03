int new_rm_connections_sock_fd;
int new_rm_connections_port;

void *manage_new_rm_connections(void *data)
{
    int sock_fd = *((int *)data);
    int new_conn_sock_fd;
    struct sockaddr_in new_rm_addr;
    socklen_t new_rm_len;

    new_rm_len = sizeof(struct sockaddr_in);

    while (TRUE)
    {
        new_conn_sock_fd = accept(sock_fd, (struct sockaddr *)&new_rm_addr, &new_rm_len);

        if (new_conn_sock_fd == -1)
        {
            printf("ERROR on accept\n");
        }
        else
        {

            pthread_t new_rm_connection_thread;
            RM_CONNECTION *new_rm_data = (RM_CONNECTION *)malloc(sizeof(RM_CONNECTION));

            new_rm_data->sock_fd = new_conn_sock_fd;
            new_rm_data->s_addr = new_rm_addr.sin_addr.s_addr;
            new_rm_data->port = new_rm_addr.sin_port;

            printf("New RM connection %d accepted\n\n", new_rm_data->sock_fd);

            pthread_create(&new_rm_connection_thread, NULL, start_connection, (void *)new_rm_data);
        }
    }
}
