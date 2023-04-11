#define MAX_WAITING_CLIENT_CONNECTIONS 1
#define FRONTEND_SOCKET_OFFSET 10000

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

int cont = 0;

mutex cli_read_mtx;
mutex srv_read_mtx;

void *handle_server_to_client_messages(void *data);

void *handle_client_to_server_messages(void *data)
{

    while (!srv_connection_data.sock_fd || !cli_connection_data.sock_fd)
    {
        cout << "Waiting" << endl;
    }
    while (TRUE)
    {
        redirectMessage(cli_connection_data.sock_fd, srv_connection_data.sock_fd);
    }
}

void *handle_server_to_client_messages(void *data)
{
    while (!srv_connection_data.sock_fd || !cli_connection_data.sock_fd)
    {
        cout << "Waiting" << endl;
    }
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
    cout << "FE CONNECTING TO SERVER ON IP: " << srv_address.ip_addr << " PORT: " << srv_address.port << endl;
    struct hostent *server = gethostbyname(srv_address.ip_addr);
    int port = srv_address.port;

    struct sockaddr_in serv_addr;

    if (server == NULL)
    {
        cout << "ERROR, no such host" << endl;
        exit(0);
    }

    sock_fd = socket(AF_INET, SOCK_STREAM, SOCKET_DEFAULT_PROTOCOL);
    if (sock_fd == -1)
    {
        cout << "ERROR opening socket" << endl;
        exit(0);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr = *((struct in_addr *)server->h_addr);

    bzero(&(serv_addr.sin_zero), BYTE_SIZE);

    int conn_return = connect(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (conn_return < 0)
    {
        cout << "ERROR connecting" << endl;
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
        cout << "ERROR on accept" << endl;
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
        cout << "ERROR opening socket" << endl
             << endl;
        exit(0);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    bzero(&(serv_addr.sin_zero), BYTE_SIZE);

    int bind_return = bind(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    if (bind_return < 0)
    {
        cout << "ERROR on binding" << endl
             << endl;
        exit(0);
    }

    listen(sock_fd, MAX_WAITING_CLIENT_CONNECTIONS);

    cout << "Frontend Server listening on port " << port << endl
         << endl;

    global_fe_sock_fd = sock_fd;
    return sock_fd;
}

int frontend_connection_procedure(int port, int sock_fd)
{
    sock_fd = init_frontend(port);

    handle_cli_connection(sock_fd);

    pthread_t cli_thread;

    cout << "Start cli-fe connection" << endl;
    pthread_create(&cli_thread, NULL, handle_client_to_server_messages, (void *)NULL);

    FE_SERVER_ADDRESS srv_connection_data = get_server_address();

    cout << "Start fe-srv connection" << endl;
    pthread_t srv_thread;
    pthread_create(&srv_thread, NULL, handle_server_to_client_messages, (void *)NULL);

    connect_to_server(srv_connection_data);

    return sock_fd;
}

void *frontend_main(void *data)
{
    FE_RUN_DATA *run_data = (FE_RUN_DATA *)data;

    int sock_fd;

    int port = run_data->fe_port;
    global_fe_server_address = run_data->srv_address;

    sock_fd = frontend_connection_procedure(port, sock_fd);

    close(sock_fd);
    return 0;
}
