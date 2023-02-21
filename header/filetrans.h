
void send_file(int sock_fd, UP_DOWN_COMMAND *up_down_command, char *directory)
{
    char *file_name = up_down_command->filename;
    char *file_path = (char *)malloc(strlen(directory) + strlen(file_name) + 1);
    strcpy(file_path, directory);
    strcat(file_path, file_name);

    char buffer[MAX_PACKET_PAYLOAD_SIZE];
    ifstream file(file_path, std::ios::binary);

    if (file.is_open())
    {
        while (file.read(buffer, sizeof(buffer)))
        {
            send_data_with_packets(sock_fd, buffer, file.gcount());
        }

        send_data_with_packets(sock_fd, buffer, file.gcount());

        file.close();
    }
    else
    {
        throw "Unable to open file\n";
    }
}

void receive_file(int sock_fd, UP_DOWN_COMMAND *up_down_command, char *directory)
{
    char *file_name = up_down_command->filename;
    char *file_path = (char *)malloc(strlen(directory) + strlen(file_name) + 1);
    strcpy(file_path, directory);
    strcat(file_path, file_name);

    ofstream file(file_path, ios::binary | ios::out | ofstream::trunc);

    uint64_t missing_bytes = up_down_command->size;

    if (file.is_open())
    {
        while (missing_bytes > 0)
        {
            DATA_RETURN data = receive_data_with_packets(sock_fd);

            char *data_recovered = data.data;
            int bytes_size = data.bytes_size;

            missing_bytes -= bytes_size;

            file.write(data_recovered, bytes_size);
        }
        file.close();

        struct utimbuf new_times;
        new_times.actime = up_down_command->last_modified;
        new_times.modtime = up_down_command->last_modified;
        utime(file_path, &new_times);
    }
    else
    {
        throw "Unable to open file\n";
    }
}
