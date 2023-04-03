

void secondary_replica_manager_start(int port, struct hostent *main_server, int main_server_port)
{

    int sock_fd;

    // TODO connect to main server

    sock_fd = connect_socket(main_server, main_server_port);

    sleep(10);
    close(sock_fd);
    return;
}