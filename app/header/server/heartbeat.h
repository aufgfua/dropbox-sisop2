#define HEARTBEAT_PORT_OFFSET 2000
#define HEARTBEAT_TIMEOUT 5
#define HEARTBEAT_BUFFER_SIZE 128
#define HEARTBEAT_MSG "HB"

void *primary_heartbeat_loop(void *data)
{
    int sock_fd = *((int *)data);
    while (TRUE)
    {
        sleep(HEARTBEAT_TIMEOUT);

        // SEND
        char heartbeat_message[HEARTBEAT_BUFFER_SIZE];
        strcpy(heartbeat_message, HEARTBEAT_MSG);
        if (send(sock_fd, heartbeat_message, HEARTBEAT_BUFFER_SIZE, 0) < 0)
        {
            std::cerr << "Error sending heartbeat message" << std::endl;
            continue;
        }

        cout << "HB> " << sock_fd << " ---- Send: <3 - ";

        // RECEIVE
        char response[HEARTBEAT_BUFFER_SIZE];
        int n = recv(sock_fd, response, HEARTBEAT_BUFFER_SIZE, 0);
        if (n < 0)
        {
            std::cerr << "Error receiving heartbeat response" << std::endl;
            break;
        }
        else if (n == 0)
        {
            std::cerr << "Heartbeat connection closed!!!!" << std::endl;
            // TODO PROCESS RM HEARTBEAT FAILURE
            break;
        }

        cout << "Receive: <3" << endl;
    }
    return NULL;
}

void *start_heartbeat_primary_rm(void *data)
{
    int port = *((int *)data);

    cout << "Heartbeat Primary RM -> ";
    int heartbeat_sock_fd = init_server(port);

    while (TRUE)
    {
        struct sockaddr_in secondary_rm_addr;
        socklen_t client_len = sizeof(secondary_rm_addr);
        int rm_sock_fd = accept(heartbeat_sock_fd, (struct sockaddr *)&secondary_rm_addr, &client_len);
        if (rm_sock_fd < 0)
        {
            std::cerr << "Error accepting secondary RM heartbeat connection" << std::endl;
            continue;
        }

        cout << "Connected to Heartbeat on " << rm_sock_fd << "!" << endl;

        pthread_t heartbeat_thread;
        pthread_create(&heartbeat_thread, NULL, primary_heartbeat_loop, (void *)&rm_sock_fd);
    }

    return NULL;
}

void *secondary_heartbeat_loop(void *data)
{
    int sock_fd = *((int *)data);
    while (TRUE)
    {
        sleep(HEARTBEAT_TIMEOUT);

        // RECEIVE
        char heartbeat_message[HEARTBEAT_BUFFER_SIZE];
        int n = recv(sock_fd, heartbeat_message, HEARTBEAT_BUFFER_SIZE, 0);
        if (n < 0)
        {
            std::cerr << "Error receiving heartbeat message" << std::endl;
            break;
        }
        else if (n == 0)
        {
            std::cerr << "Heartbeat connection closed!!!!" << std::endl;
            // TODO PROCESS RM HEARTBEAT FAILURE
            break;
        }
        cout << "HB> " << sock_fd << " ---- Receive: <3 - ";

        // SEND
        char response[HEARTBEAT_BUFFER_SIZE];
        strcpy(response, HEARTBEAT_MSG);
        if (send(sock_fd, response, HEARTBEAT_BUFFER_SIZE, 0) < 0)
        {
            std::cerr << "Error sending heartbeat response" << std::endl;
            continue;
        }
        cout << "Send: <3" << endl;
    }
    return NULL;
}

int start_heartbeat_secondary_rm(struct hostent *server, int port)
{
    cout << "Heartbeat Secondary RM -> ";

    int heartbeat_sock_fd = connect_socket(server, port);

    cout << "Connected to Heartbeat on " << heartbeat_sock_fd << "!" << endl;
    return heartbeat_sock_fd;
}