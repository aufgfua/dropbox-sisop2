
void srv_sync_file(int sock_fd, UP_DOWN_COMMAND *up_down_command, char *directory)
{
    send_transaction_controller(sock_fd, ONE_MORE_FILE);

    send_up_down_command(sock_fd, up_down_command);

    if (up_down_command->sync_type == SERVER_SYNC_UPLOAD)
    {
        send_file(sock_fd, up_down_command, directory);
    }
    else if (up_down_command->sync_type == SERVER_SYNC_DOWNLOAD)
    {
        receive_file(sock_fd, up_down_command, directory);
    }
}

void srv_sync_files_list(int sock_fd, vector<UP_DOWN_COMMAND> up_down_command, char *directory)
{
    int number_of_files = up_down_command.size();

    while (number_of_files > 0)
    {

        UP_DOWN_COMMAND file = up_down_command.back();
        up_down_command.pop_back();

        srv_sync_file(sock_fd, &file, directory);

        number_of_files--;
    }

    send_transaction_controller(sock_fd, NO_MORE_FILES);
}
