
void cli_sync_file(int sock_fd, UP_DOWN_COMMAND *up_down_command, char *directory)
{
    if (up_down_command->sync_type == CLIENT_SYNC_DOWNLOAD)
    {
        receive_file(sock_fd, up_down_command, directory);
    }
    else if (up_down_command->sync_type == CLIENT_SYNC_UPLOAD)
    {
        send_file(sock_fd, up_down_command, directory);
    }
}

void cli_transaction_loop(int sock_fd, char *directory)
{
    FILE_TRANSACTION_CONTROLLER *file_transaction_controller = receive_transaction_controller(sock_fd);

    while (file_transaction_controller->one_more == ONE_MORE_FILE)
    {
        UP_DOWN_COMMAND *up_down_command = receive_up_down_command(sock_fd);

        cli_sync_file(sock_fd, up_down_command, directory);

        file_transaction_controller = receive_transaction_controller(sock_fd);
    }
}