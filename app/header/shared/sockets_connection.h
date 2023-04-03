
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