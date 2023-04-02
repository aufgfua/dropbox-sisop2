#define BUFFER_SIZE 256
#define BYTE_SIZE 8
#define MAX_WAITING_CLIENT_CONNECTIONS 1
#define SOCKET_DEFAULT_PROTOCOL 0
#define bool int

// FOR NOW - TODO READ THIS FROM DOCUMENT
#define SERVER_IP "localhost"

typedef struct STR_FE_SERVER_ADDRESS
{
    char ip_addr[MAX_IP_SIZE];
    int port;
} FE_SERVER_ADDRESS;

typedef struct STR_FE_RUN_DATA
{
    int fe_port;
    FE_SERVER_ADDRESS srv_address;
} FE_RUN_DATA;

typedef struct STR_FE_CONNECTION_DATA
{
    unsigned long conn_s_addr;
    int sock_fd;
    char username[MAX_USERNAME_SIZE];
} FE_CONNECTION_DATA;

FE_CONNECTION_DATA cli_connection_data, srv_connection_data;

int global_fe_sock_fd;
FE_SERVER_ADDRESS global_fe_server_address;

void *handle_client_to_server_messages(void *data)
{
    while (TRUE)
    {
        redirectMessage(cli_connection_data.sock_fd, srv_connection_data.sock_fd);
    }
}

void *handle_server_to_client_messages(void *data)
{
    while (TRUE)
    {
        redirectMessage(srv_connection_data.sock_fd, cli_connection_data.sock_fd);
    }
}

// TODO - get this data from document
FE_SERVER_ADDRESS get_server_address()
{
    return global_fe_server_address;
}

void connect_to_server(FE_SERVER_ADDRESS srv_address)
{
    int sock_fd, read_len, write_len;
    struct hostent *server = gethostbyname(srv_address.ip_addr);
    int port = srv_address.port;

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

    int conn_return = connect(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (conn_return < 0)
    {
        printf("ERROR connecting\n");
        exit(0);
    }

    FE_CONNECTION_DATA *srv_conn_data = (FE_CONNECTION_DATA *)malloc(sizeof(FE_CONNECTION_DATA));

    srv_conn_data->sock_fd = sock_fd;
    srv_conn_data->conn_s_addr = serv_addr.sin_addr.s_addr;

    // save server connection data globally
    srv_connection_data = *srv_conn_data;
}

void handle_cli_connection(int sock_fd)
{
    int cli_conn_sock_fd;
    struct sockaddr_in cli_addr;
    socklen_t cli_len;

    cli_len = sizeof(struct sockaddr_in);

    cli_conn_sock_fd = accept(sock_fd, (struct sockaddr *)&cli_addr, &cli_len);

    while (cli_conn_sock_fd == -1)
    {
        printf("ERROR on accept\n");
    }
    // successfully accepted connection

    pthread_t connection_thread;
    FE_CONNECTION_DATA *cli_conn_data = (FE_CONNECTION_DATA *)malloc(sizeof(FE_CONNECTION_DATA));

    cli_conn_data->sock_fd = cli_conn_sock_fd;
    cli_conn_data->conn_s_addr = cli_addr.sin_addr.s_addr;

    // save cllient connection data globally
    cli_connection_data = *cli_conn_data;
}

// returns socket file descriptor
int init_frontend(int port)
{
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];
    int sock_fd;

    sock_fd = socket(AF_INET, SOCK_STREAM, SOCKET_DEFAULT_PROTOCOL);

    if (sock_fd == -1)
    {
        printf("ERROR opening socket\n\n");
        exit(0);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    bzero(&(serv_addr.sin_zero), BYTE_SIZE);

    int bind_return = bind(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    if (bind_return < 0)
    {
        printf("ERROR on binding\n\n");
        exit(0);
    }

    listen(sock_fd, MAX_WAITING_CLIENT_CONNECTIONS);

    printf("Frontend Server listening on port %d\n\n", port);

    global_fe_sock_fd = sock_fd;
    return sock_fd;
}

void frontend_connection_procedure(int port, int sock_fd)
{
    sock_fd = init_frontend(port);

    handle_cli_connection(sock_fd);

    FE_SERVER_ADDRESS srv_connection_data = get_server_address();

    connect_to_server(srv_connection_data);

    pthread_t cli_thread, srv_thread;

    printf("Start cli-fe connection\n");
    pthread_create(&cli_thread, NULL, handle_client_to_server_messages, (void *)NULL);

    printf("Start fe-srv connection\n");
    pthread_create(&srv_thread, NULL, handle_server_to_client_messages, (void *)NULL);

    // TODO - create loop to switch between client and server communication
}

void *frontend_main(void *data)
{
    FE_RUN_DATA *run_data = (FE_RUN_DATA *)data;

    int sock_fd;

    int port = run_data->fe_port;
    global_fe_server_address = run_data->srv_address;

    frontend_connection_procedure(port, sock_fd);

    close(sock_fd);
    return 0;
}
