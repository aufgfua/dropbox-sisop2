#define HEARTBEAT_PORT_OFFSET 2000
#define HEARTBEAT_TIMEOUT 5
#define HEARTBEAT_BUFFER_SIZE 128
#define HEARTBEAT_MSG "HB"

bool primary_server_died = FALSE;
typedef struct STR_HEARTBEAT_CONNECTION
{
    int heartbeat_sock_fd;
    int rm_connection_sock_fd;
} HEARTBEAT_CONNECTION;

void *primary_heartbeat_loop(void *data)
{

    RM_CONNECTION *rm_data = (RM_CONNECTION *)data;

    int heartbeat_sock_fd = rm_data->sock_fd;

    while (TRUE)
    {
        this_thread::sleep_for(chrono::milliseconds(HEARTBEAT_TIMEOUT * 1000));

        // SEND
        char heartbeat_message[HEARTBEAT_BUFFER_SIZE];
        strcpy(heartbeat_message, HEARTBEAT_MSG);
        if (send(heartbeat_sock_fd, heartbeat_message, HEARTBEAT_BUFFER_SIZE, 0) < 0)
        {
            std::cerr << "Error sending heartbeat message" << std::endl;
            continue;
        }

        cout << "HB> " << heartbeat_sock_fd << " ---- Send: <3 - ";

        // RECEIVE
        char response[HEARTBEAT_BUFFER_SIZE];
        int n = recv(heartbeat_sock_fd, response, HEARTBEAT_BUFFER_SIZE, 0);
        if (n < 0)
        {
            std::cerr << "Error receiving heartbeat response" << std::endl;
            break;
        }
        else if (n == 0)
        {
            std::cerr << "Heartbeat connection closed!!!! HB-FD: " << heartbeat_sock_fd << std::endl;

            close(heartbeat_sock_fd);
            remove_rm_connection_s_addr(rm_data->s_addr);

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
        int new_hb_connection_sock_fd = accept(heartbeat_sock_fd, (struct sockaddr *)&secondary_rm_addr, &client_len);
        if (new_hb_connection_sock_fd < 0)
        {
            std::cerr << "Error accepting secondary RM heartbeat connection" << std::endl;
            continue;
        }

        RM_CONNECTION *new_heartbeat_conn = (RM_CONNECTION *)malloc(sizeof(RM_CONNECTION));

        new_heartbeat_conn->sock_fd = new_hb_connection_sock_fd;
        new_heartbeat_conn->s_addr = secondary_rm_addr.sin_addr.s_addr;
        new_heartbeat_conn->port = secondary_rm_addr.sin_port;

        cout << "Connected to Heartbeat on " << new_hb_connection_sock_fd << "!" << endl;

        pthread_t heartbeat_thread;
        pthread_create(&heartbeat_thread, NULL, primary_heartbeat_loop, (void *)new_heartbeat_conn);
    }

    return NULL;
}

void *secondary_heartbeat_loop(void *data)
{
    try
    {
        HEARTBEAT_CONNECTION *hb_conn = (HEARTBEAT_CONNECTION *)data;
        int sock_fd = hb_conn->heartbeat_sock_fd;

        int rm_connection_sock_fd = hb_conn->rm_connection_sock_fd;

        while (TRUE)
        {
            this_thread::sleep_for(chrono::milliseconds(HEARTBEAT_TIMEOUT * 1000));

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
                primary_server_died = TRUE;
                throw PrimaryRMDiedException();
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
    }
    catch (PrimaryRMDiedException e)
    {
        cout << "HB-ERR> Primary RM died!" << endl;
        begin_election();
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