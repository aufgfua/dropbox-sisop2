

typedef struct STR_PROCEDURE_SELECT
{
    int proc_id;
} PROCEDURE_SELECT;

#define LOG_MODE_OFF 0
#define LOG_MODE_ON 1

int LOG_MODE = LOG_MODE_OFF;

// function to read a generic structure from the client using sockets
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