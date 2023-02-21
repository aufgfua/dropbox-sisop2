#define CLIENT_BASE_DIR "/PROJECT/client/.sync_dir_"
#define SERVER_BASE_DIR "/PROJECT/server/.sync_dir_"

#define PROCEDURE_NOP 0
#define PROCEDURE_SYNC_FILES 1
#define PROCEDURE_LIST_SERVER 2
#define PROCEDURE_UPLOAD_TO_SERVER 3
#define PROCEDURE_DOWNLOAD_FROM_SERVER 4
#define PROCEDURE_EXIT 99

typedef struct STR_PROCEDURE_SELECT
{
    uint16_t proc_id;
} PROCEDURE_SELECT;

typedef struct STR_LOGIN
{
    char username[32];
} LOGIN;

PROCEDURE_SELECT *send_procedure(int sock_fd, int proc_id)
{
    PROCEDURE_SELECT *procedure = (PROCEDURE_SELECT *)malloc(sizeof(PROCEDURE_SELECT));
    procedure->proc_id = proc_id;

    write_all_bytes(sock_fd, (char *)procedure, sizeof(PROCEDURE_SELECT));
    return procedure;
}

PROCEDURE_SELECT *receive_procedure(int sock_fd)
{
    char *buffer = read_all_bytes(sock_fd, sizeof(PROCEDURE_SELECT));
    return (PROCEDURE_SELECT *)buffer;
}
