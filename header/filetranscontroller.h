
#define ONE_MORE_FILE 1
#define NO_MORE_FILES 0

typedef struct STR_FILE_TRANSACTION_CONTROLLER
{
    bool one_more;
} FILE_TRANSACTION_CONTROLLER;

void send_transaction_controller(int sock_fd, int one_more)
{
    FILE_TRANSACTION_CONTROLLER file_transaction_controller;
    file_transaction_controller.one_more = one_more;

    send_data_with_packets(sock_fd, (char *)&file_transaction_controller, sizeof(FILE_TRANSACTION_CONTROLLER));
}

FILE_TRANSACTION_CONTROLLER *receive_transaction_controller(int sock_fd)
{
    DATA_RETURN data = receive_data_with_packets(sock_fd);

    char *data_recovered = data.data;
    int bytes_size = data.bytes_size;

    FILE_TRANSACTION_CONTROLLER *file_transaction_controller = (FILE_TRANSACTION_CONTROLLER *)data_recovered;

    return file_transaction_controller;
}
