#include <arpa/inet.h>
#include <ctime>
#include <dirent.h>
#include <errno.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <pwd.h>
#include <queue>
#include <regex>
#include <signal.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>
#include <vector>

using namespace std;

#define TRUE 1
#define FALSE 0

#define LOG_MODE_OFF 0
#define LOG_MODE_ON 1
int LOG_MODE = LOG_MODE_OFF;

char *read_all_bytes(int sockfd, int bytes_to_read);
int write_all_bytes(int sockfd, char *buffer, int bytes_to_write);

void send_data_with_packets(int sock_fd, char *data, int bytes_size);
template <typename T>
T *receive_converted_data_with_packets(int sock_fd);

#define MAX_FILENAME_SIZE 256
#define MAX_PATH_SIZE 1024
#define MAX_IP_SIZE 256

#define CLI_TURN 0
#define SRV_TURN 1
#define START_TURN CLI_TURN

#define SYNC_WAIT 4

#define FE_PORT 50000
#define SERVER_PORT 40001

bool started_connection_loop = FALSE;

#include "header/shared/exceptions.h"

#include "header/shared/appflow.h"
#include "header/shared/packets.h"

#include "header/server/threadssync.h"

#include "header/shared/fileslist.h"
#include "header/shared/updowncmd.h"
#include "header/shared/filetranscontroller.h"
#include "header/shared/filetrans.h"

#include "header/client/sync_file_cli.h"
#include "header/server/sync_file_srv.h"

#include "header/server/server_utils.h"

#include "header/frontend/redirectMessages.h"
#include "frontend.h"

time_t
get_now()
{
    time_t t = time(0); // get time now
    return t;
}
