#include <ctime>
#include <iostream>
#include <dirent.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <vector>
#include <errno.h>
#include <pwd.h>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace std;

typedef struct STR_FILE
{
    char filename[MAX_FILENAME_SIZE];
    uint64_t size;
    uint32_t last_modified;
    uint32_t last_accessed;
    uint32_t last_changed;
} USR_FILE;

void print_usr_file(USR_FILE file)
{
    printf("Filename: %s, Size: %lu, Last modified: %u, Last accessed: %u, Last changed: %u\n", file.filename, file.size, file.last_modified, file.last_accessed, file.last_changed);
}

void print_usr_files(vector<USR_FILE> files)
{
    for (int i = 0; i < files.size(); i++)
    {
        print_usr_file(files[i]);
    }
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

USR_FILE *get_file_metadata(char *filename, char *directory)
{
    USR_FILE *file = (USR_FILE *)malloc(sizeof(USR_FILE));

    bool found = FALSE;

    DIR *dir = opendir(directory);
    if (dir == NULL)
    {
        perror("opendir");
        return NULL;
    }

    struct dirent *ent;

    while ((ent = readdir(dir)) != NULL)
    {
        if (ent->d_type == DT_REG)
        {
            string curr_file_name = ent->d_name;

            if (curr_file_name.find(CONTROL_PREFIX))
                continue;

            if (strcmp(filename, ent->d_name) == 0)
            {
                found = TRUE;

                char path[MAX_FILENAME_SIZE];
                strcpy(file->filename, ent->d_name);

                strcpy(path, directory);
                strcat(path, ent->d_name);

                struct stat buf;
                if (stat(path, &buf) == 0)
                {
                    file->size = buf.st_size;
                    file->last_modified = buf.st_mtime;
                    file->last_accessed = buf.st_atime;
                    file->last_changed = buf.st_ctime;
                    break;
                }
            }
        }
    }

    closedir(dir);

    if (!found)
    {
        return NULL;
    }
    return file;
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
            string curr_file_name = ent->d_name;
            if (curr_file_name.find(CONTROL_PREFIX))
                continue;

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

            if (LOG_MODE)
                print_usr_file(file);
        }
    }

    closedir(dir);

    return files;
}

vector<USR_FILE> receive_files_list(int sock_fd)
{
    DATA_RETURN data = receive_data_with_packets(sock_fd);

    char *data_recovered = data.data;
    int bytes_size = data.bytes_size;

    USR_FILE *file_list = (USR_FILE *)data_recovered;
    int number_of_files = convert_type_size(bytes_size, sizeof(char), sizeof(USR_FILE));

    vector<USR_FILE>
        return_vector;
    return_vector.assign(file_list, file_list + number_of_files);

    return return_vector;
}

void send_files_list(int sock_fd, char *directory)
{
    vector<USR_FILE> *files_vector = list_files(directory);

    USR_FILE *files = files_vector->data();

    int files_bytes_size = files_vector->size() * sizeof(USR_FILE);

    send_data_with_packets(sock_fd, (char *)files, files_bytes_size);
}
