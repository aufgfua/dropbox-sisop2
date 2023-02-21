
#define CONTROL_FILE ".control-lock"
#define CONTROL_PREFIX ".control"

mutex lock_creation_mtx;

void create_lock_file(char *user_dir)
{
    char control_file_path[MAX_PATH_SIZE];
    strcpy(control_file_path, user_dir);
    strcat(control_file_path, CONTROL_FILE);

    ofstream control_file;
    control_file.open(control_file_path);
    control_file << "locked";
    control_file.close();
}

void get_sync_dir_control(char *user_dir)
{
    char control_file_path[MAX_PATH_SIZE];
    strcpy(control_file_path, user_dir);
    strcat(control_file_path, CONTROL_FILE);

    ifstream control_file;
    bool is_locked = TRUE;

    while (is_locked)
    {
        lock_creation_mtx.lock();

        control_file.open(control_file_path);
        if (control_file.is_open())
        {
            is_locked = true;
            control_file.close();
        }
        else
        {
            printf("Creating lock file...\n");
            create_lock_file(user_dir);
            is_locked = false;
        }
        lock_creation_mtx.unlock();
    }
}

void release_sync_dir_control(char *user_dir)
{
    printf("Releasing lock file...\n");
    char control_file_path[MAX_PATH_SIZE];
    strcpy(control_file_path, user_dir);
    strcat(control_file_path, CONTROL_FILE);

    remove(control_file_path);
}