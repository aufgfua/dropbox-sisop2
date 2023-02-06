#include <dirent.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <vector>
#include <errno.h>
#include <pwd.h>

using namespace std;

#define LOG_MODE_OFF 0
#define LOG_MODE_ON 1
#define MAX_PACKET_PAYLOAD_SIZE 1024
#define MAX_FILENAME_SIZE 256
#define MAX_PATH_SIZE 1024

#define CLIENT_BASE_DIR "/PROJECT/client/.sync_dir_"
#define SERVER_BASE_DIR "/PROJECT/server/.sync_dir_"

#define PACKET_TYPE_DATA 1

#define PROCEDURE_LIST_FILES 1

int LOG_MODE = LOG_MODE_OFF;

typedef struct STR_PROCEDURE_SELECT
{
    uint16_t proc_id;
} PROCEDURE_SELECT;

typedef struct STR_LOGIN
{
    char username[32];
} LOGIN;

typedef struct STR_NUMBER_OF_PACKETS
{
    uint32_t pck_number;
    uint32_t total_size;
} NUMBER_OF_PACKETS;

typedef struct packet
{
    uint16_t type;                          // Tipo do pacote (p.ex. DATA | CMD)
    uint16_t seqn;                          // Número de sequência
    uint32_t total_size;                    // Número total de fragmentos
    uint16_t length;                        // Comprimento do payload
    char _payload[MAX_PACKET_PAYLOAD_SIZE]; // Dados do pacote
} packet;

void print_packet(packet pck)
{
    printf("Type: %u, Seqn: %u, Total size: %u, Length: %u, Value: %s\n", pck.type, pck.seqn, pck.total_size, pck.length, pck._payload);
}

typedef struct STR_FILE
{
    char filename[MAX_FILENAME_SIZE];
    uint32_t size;
    uint32_t last_modified;
    uint32_t last_accessed;
    uint32_t last_changed;
} USR_FILE;

void print_usr_file(USR_FILE file)
{
    printf("Filename: %s, Size: %d, Last modified: %u, Last accessed: %u, Last changed: %u\n", file.filename, file.size, file.last_modified, file.last_accessed, file.last_changed);
}

int get_number_of_packets(int data_size)
{
    int number_of_packets = data_size / MAX_PACKET_PAYLOAD_SIZE;
    if (data_size % MAX_PACKET_PAYLOAD_SIZE != 0)
    {
        number_of_packets++;
    }
    return number_of_packets;
}

vector<packet> *fragment_data(char *data, int data_size)
{
    int number_of_packets = get_number_of_packets(data_size);
    vector<packet> *packets = new vector<packet>;

    if (data_size > 0)
    {
        for (int i = 0; i < number_of_packets; i++)
        {
            packet p;
            p.type = PACKET_TYPE_DATA;
            p.seqn = i;
            p.total_size = number_of_packets;
            p.length = data_size > MAX_PACKET_PAYLOAD_SIZE ? MAX_PACKET_PAYLOAD_SIZE : data_size;
            memcpy(p._payload, data, p.length);
            packets->push_back(p);
            data += p.length;
            data_size -= p.length;
        }
    }

    return packets;
}

vector<USR_FILE> *list_files(const char *folder)
{
    DIR *dir = opendir(folder);
    if (dir == NULL)
    {
        perror("opendir");
        return NULL;
    }

    vector<USR_FILE> *files = new vector<USR_FILE>;

    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL)
    {
        USR_FILE file;
        if (ent->d_type == DT_REG)
        {
            char path[MAX_FILENAME_SIZE];
            strcpy(file.filename, ent->d_name);

            strcpy(path, folder);
            strcat(path, ent->d_name);

            struct stat buf;
            if (stat(path, &buf) == 0)
            {
                file.size = buf.st_size;
                file.last_modified = buf.st_mtime;
                file.last_accessed = buf.st_atime;
                file.last_changed = buf.st_ctime;

                files->push_back(file);
            }
        }
    }

    closedir(dir);

    return files;
}

char *get_home_path()
{
    char *homedir;

    if ((homedir = getenv("HOME")) == NULL)
    {
        homedir = getpwuid(getuid())->pw_dir;
    }

    return homedir;
}

char *mount_base_path(char *username, const char *base_dir)
{
    char *base_path = (char *)malloc(MAX_PATH_SIZE);
    strcpy(base_path, get_home_path());
    strcat(base_path, base_dir);
    strcat(base_path, username);
    strcat(base_path, "/");

    return base_path;
}

void create_folder_if_not_exists(char *path)
{
    struct stat st = {0};
    if (stat(path, &st) == -1)
    {
        if (errno == ENOENT)
        {
            // create the parent folder first
            char parent[1024];
            strcpy(parent, path);
            char *slash = strrchr(parent, '/');
            if (slash != NULL)
            {
                *slash = '\0';
                create_folder_if_not_exists(parent);
            }
            // create the folder
            char *last_slash = strrchr(path, '/');
            if (last_slash != path + strlen(path) - 1)
            {
                mkdir(path, 0700);
            }
        }
        else
        {
            perror("stat");
        }
    }
}

char *read_all_bytes(int sockfd, int bytes_to_read)
{
    LOG_MODE ? printf("Reading %d bytes from socket\n", bytes_to_read) : 0;

    int total_bytes_read = 0;
    char *buffer = (char *)malloc(bytes_to_read);
    if (buffer == NULL)
    {
        // Handle memory allocation error
        printf("Error allocating memory\n");
        return NULL;
    }
    while (bytes_to_read > 0)
    {
        int bytes_received = recv(sockfd, buffer + total_bytes_read, bytes_to_read, 0);

        LOG_MODE ? printf("Bytes received - %d\n", bytes_received) : 0;

        if (bytes_received <= 0)
        {
            printf("Error reading from socket\n");
            // Handle error or connection closed
            break;
        }
        total_bytes_read += bytes_received;
        bytes_to_read -= bytes_received;
    }
    if (bytes_to_read != 0)
    {
        printf("Invalid length reading from socket\n");
        // Handle case where not all the desired bytes were read
        free(buffer);
        return NULL;
    }
    return buffer;
}

int write_all_bytes(int sockfd, char *buffer, int bytes_to_write)
{
    LOG_MODE ? printf("Writing %d bytes to socket\n", bytes_to_write) : 0;
    int total_bytes_written = 0;
    while (bytes_to_write > 0)
    {
        int bytes_sent = send(sockfd, buffer + total_bytes_written, bytes_to_write, 0);
        LOG_MODE ? printf("Bytes sent - %d\n", bytes_sent) : 0;
        if (bytes_sent <= 0)
        {
            // Handle error or connection closed
            printf("Error writing to socket\n");
            return bytes_sent;
        }

        total_bytes_written += bytes_sent;
        bytes_to_write -= bytes_sent;
    }
    return total_bytes_written;
}