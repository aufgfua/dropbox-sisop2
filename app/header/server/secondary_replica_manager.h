

void secondary_replica_manager_start(int argc, char *argv[])
{

    int sock_fd;

    int port = atoi(argv[1]);
    struct hostent *main_server = gethostbyname(argv[2]);
    int main_server_port = atoi(argv[3]);

    sock_fd = init_server(port);

    // TODO connect to main server

    manage_connections(sock_fd);

    close(sock_fd);
    return;
}