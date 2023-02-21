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

#define LOG_MODE_OFF 0
#define LOG_MODE_ON 1
int LOG_MODE = LOG_MODE_OFF;

char *read_all_bytes(int sockfd, int bytes_to_read);
int write_all_bytes(int sockfd, char *buffer, int bytes_to_write);

#define MAX_FILENAME_SIZE 256
#define MAX_PATH_SIZE 1024

#define CLI_TURN 0
#define SRV_TURN 1
#define START_TURN CLI_TURN

#define SYNC_WAIT 20

#include "header/appflow.h"
#include "header/packets.h"

#include "header/fileslist.h"
#include "header/updowncmd.h"
#include "header/filetranscontroller.h"
#include "header/filetrans.h"

#include "header/sync_file_cli.h"
#include "header/sync_file_srv.h"

using namespace std;

time_t get_now()
{
    time_t t = time(0); // get time now
    return t;
}
