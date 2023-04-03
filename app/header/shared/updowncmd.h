
using namespace std;

typedef struct STR_UP_DOWN_COMMAND
{
    uint16_t sync_type;
    char filename[MAX_FILENAME_SIZE];
    uint32_t last_modified;
    uint64_t size;
    char path[MAX_FILENAME_SIZE];
} UP_DOWN_COMMAND;

#define SERVER_SYNC_UPLOAD 1
#define SERVER_SYNC_DOWNLOAD 2

#define CLIENT_SYNC_DOWNLOAD 1
#define CLIENT_SYNC_UPLOAD 2

#define SERVER_SYNC_KEEP_FILE 0
#define CLIENT_SYNC_KEEP_FILE 0

void print_up_down_command(UP_DOWN_COMMAND up_down_command)
{

    cout << "Filename: " << up_down_command.filename << ", Size: " << up_down_command.size << ", Type: " << up_down_command.sync_type << ", Path: " << up_down_command.path << endl;
}

void print_up_down_commands(vector<UP_DOWN_COMMAND> up_down_commands)
{
    cout << up_down_commands.size() << "d Sync files:" << endl;
    for (int i = 0; i < up_down_commands.size(); i++)
    {
        UP_DOWN_COMMAND up_down_command = up_down_commands[i];
        print_up_down_command(up_down_command);
    }
}

UP_DOWN_COMMAND *create_up_down_command(char *filename, int sync_type, int size, uint32_t last_modified, char *path)
{
    UP_DOWN_COMMAND *up_down_command = (UP_DOWN_COMMAND *)malloc(sizeof(UP_DOWN_COMMAND));
    strcpy(up_down_command->filename, filename);
    strcpy(up_down_command->path, path);
    up_down_command->sync_type = sync_type;
    up_down_command->size = size;
    up_down_command->last_modified = last_modified;

    return up_down_command;
}

void send_up_down_command(int sock_fd, UP_DOWN_COMMAND *up_down_command)
{
    send_data_with_packets(sock_fd, (char *)up_down_command, sizeof(UP_DOWN_COMMAND));
}

UP_DOWN_COMMAND *receive_up_down_command(int sock_fd)
{
    DATA_RETURN data = receive_data_with_packets(sock_fd);

    char *data_recovered = data.data;
    int bytes_size = data.bytes_size;

    UP_DOWN_COMMAND *up_down_command = (UP_DOWN_COMMAND *)data_recovered;

    return up_down_command;
}