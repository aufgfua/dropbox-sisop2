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
#include <utime.h>
#include <arpa/inet.h>
#include <map>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <queue>
#include <string>
#include <functional>
#include <regex>
#include <sstream>

#include <mutex>

using namespace std;

#define TRUE 1
#define FALSE 0

#define LOG_MODE_OFF 0
#define LOG_MODE_ON 1
int LOG_MODE = LOG_MODE_OFF;

char *read_all_bytes(int sockfd, int bytes_to_read);
int write_all_bytes(int sockfd, char *buffer, int bytes_to_write);

#define MAX_FILENAME_SIZE 256
#define MAX_PATH_SIZE 1024
#define MAX_IP_SIZE 256

#define CLI_TURN 0
#define SRV_TURN 1
#define START_TURN CLI_TURN

#define SYNC_WAIT 4

#define FE_PORT 50000
#define SERVER_PORT 40001

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

time_t get_now()
{
    time_t t = time(0); // get time now
    return t;
}
