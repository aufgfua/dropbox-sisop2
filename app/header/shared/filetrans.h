
typedef struct STR_DESIRED_FILE
{
    char filename[MAX_FILENAME_SIZE];
} DESIRED_FILE;

#define D_FILE_EXISTS 1
#define D_FILE_NOT_EXISTS 0
typedef struct STR_FILE_EXISTS
{
    uint8_t exists;
} FILE_EXISTS;

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

void receive_single_file(int sock_fd, char *directory)
{

    DATA_RETURN data = receive_data_with_packets(sock_fd);

    char *data_recovered = data.data;
    int bytes_size = data.bytes_size;

    FILE_EXISTS *file_exists = (FILE_EXISTS *)data_recovered;

    if (file_exists->exists == D_FILE_NOT_EXISTS)
    {
        printf("File doesn't exist\n");
        return;
    }

    UP_DOWN_COMMAND *up_down_command = receive_up_down_command(sock_fd);

    receive_file(sock_fd, up_down_command, directory);
}

void send_single_file(int sock_fd, const char *file_name, const char *file_path, int SYNC_MODE)
{
    char filename[MAX_FILENAME_SIZE];
    strcpy(filename, file_name);

    char directory[MAX_PATH_SIZE];
    strcpy(directory, file_path);

    USR_FILE *file = get_file_metadata(filename, directory);

    FILE_EXISTS *file_exists = (FILE_EXISTS *)malloc(sizeof(FILE_EXISTS));
    file_exists->exists = (file == NULL) ? D_FILE_NOT_EXISTS : D_FILE_EXISTS;

    send_data_with_packets(sock_fd, (char *)file_exists, sizeof(FILE_EXISTS));

    if (file_exists->exists == D_FILE_NOT_EXISTS)
    {
        printf("File doesn't exist\n");
        return;
    }

    UP_DOWN_COMMAND *up_down_command = create_up_down_command(filename, SYNC_MODE, file->size, file->last_modified);

    send_up_down_command(sock_fd, up_down_command);

    send_file(sock_fd, up_down_command, directory);
}