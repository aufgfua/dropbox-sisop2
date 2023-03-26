void define_sync_local_files(vector<UP_DOWN_COMMAND> *sync_files, vector<USR_FILE> local_files, vector<USR_FILE> remote_files)
{

    for (int i = 0; i < local_files.size(); i++)
    {
        USR_FILE local_file = local_files[i];

        if (local_file.size == 0)
            continue;

        bool found = FALSE;

        for (int j = 0; j < remote_files.size(); j++)
        {
            USR_FILE remote_file = remote_files[j];

            if (strcmp(local_file.filename, remote_file.filename) == 0)
            {
                found = TRUE;
                if (local_file.last_modified > remote_file.last_modified && local_file.size > 0)
                {
                    UP_DOWN_COMMAND *sync_file = create_up_down_command(local_file.filename, SERVER_SYNC_UPLOAD, local_file.size, local_file.last_modified);

                    sync_files->push_back(*sync_file);
                }
                else if (local_file.last_modified < remote_file.last_modified && remote_file.size > 0)
                {
                    UP_DOWN_COMMAND *sync_file = create_up_down_command(remote_file.filename, SERVER_SYNC_DOWNLOAD, remote_file.size, remote_file.last_modified);

                    sync_files->push_back(*sync_file);
                }
                else if (local_file.last_modified == remote_file.last_modified)
                {
                    UP_DOWN_COMMAND *sync_file = create_up_down_command(local_file.filename, SERVER_SYNC_KEEP_FILE, local_file.size, local_file.last_modified);

                    sync_files->push_back(*sync_file);
                }
            }
        }

        if (!found)
        {
            UP_DOWN_COMMAND *sync_file = create_up_down_command(local_file.filename, SERVER_SYNC_UPLOAD, local_file.size, local_file.last_modified);

            sync_files->push_back(*sync_file);
        }
    }
}

void define_sync_remote_files(vector<UP_DOWN_COMMAND> *sync_files, vector<USR_FILE> local_files, vector<USR_FILE> remote_files)
{
    for (int i = 0; i < remote_files.size(); i++)
    {
        USR_FILE remote_file = remote_files[i];

        if (remote_file.size == 0)
            continue;

        bool found = FALSE;

        for (int j = 0; j < sync_files->size(); j++)
        {
            UP_DOWN_COMMAND mapped_file = sync_files->at(j);

            if (strcmp(mapped_file.filename, remote_file.filename) == 0)
            {
                found = TRUE;
                break;
            }
        }

        if (!found)
        {
            UP_DOWN_COMMAND *sync_file = create_up_down_command(remote_file.filename, SERVER_SYNC_DOWNLOAD, remote_file.size, remote_file.last_modified);

            sync_files->push_back(*sync_file);
        }
    }
}

vector<UP_DOWN_COMMAND> define_sync_files(vector<USR_FILE> local_files, vector<USR_FILE> remote_files)
{
    vector<UP_DOWN_COMMAND> sync_files;

    define_sync_local_files(&sync_files, local_files, remote_files);

    define_sync_remote_files(&sync_files, local_files, remote_files);

    return sync_files;
}