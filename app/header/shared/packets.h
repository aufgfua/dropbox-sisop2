#define MAX_PACKET_PAYLOAD_SIZE 1024
#define PACKET_TYPE_DATA 1
#define PACKET_TYPE_CONTROL 2

typedef struct STR_NUMBER_OF_PACKETS
{
    uint32_t pck_number;
    uint64_t total_size;
} NUMBER_OF_PACKETS;

typedef struct packet
{
    uint16_t type;       // Tipo do pacote (p.ex. DATA | CMD)
    uint16_t seqn;       // Número de sequência
    uint64_t total_size; // Número total de fragmentos
    uint16_t length;     // Comprimento do payload
    uint16_t restart_loop;
    char _payload[MAX_PACKET_PAYLOAD_SIZE]; // Dados do pacote
} packet;

typedef struct STR_DATA_RETURN
{
    char *data;
    uint64_t bytes_size;
} DATA_RETURN;

void handle_restart_packet(int sock_fd);

void print_packet(packet pck)
{
    printf("Type: %u, Seqn: %u, Total size: %lu, Length: %u, Value: %s\n", pck.type, pck.seqn, pck.total_size, pck.length, pck._payload);
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
            sleep(1);
            throw "Error reading from socket\n";
            break;
        }
        total_bytes_read += bytes_received;
        bytes_to_read -= bytes_received;
    }
    if (bytes_to_read != 0)
    {
        printf("Invalid length reading from socket\n");
        sleep(1);
        throw "Invalid length reading from socket\n";
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

void send_restart_packet(int sock_fd)
{
    packet restart_packet = {
        .type = PACKET_TYPE_CONTROL,
        .seqn = 0,
        .total_size = 1,
        .length = 0,
        .restart_loop = TRUE,
    };

    printf("SENDING RESTART PACKET TO %d\n", sock_fd);

    write_all_bytes(sock_fd, (char *)&restart_packet, sizeof(packet));
}

bool is_restart_packet(packet p)
{
    return p.restart_loop == TRUE;
}

int convert_type_size(int size, int init_type_size, int final_type_size)
{
    return (size * init_type_size) / final_type_size;
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
            p.restart_loop = FALSE;
            memcpy(p._payload, data, p.length);
            packets->push_back(p);
            data += p.length;
            data_size -= p.length;
        }
    }

    return packets;
}

void send_data_with_packets(int sock_fd, char *data, int bytes_size)
{
    vector<packet> *packets_vector = fragment_data(data, bytes_size);
    packet *packets = packets_vector->data();

    NUMBER_OF_PACKETS number_of_packets;
    number_of_packets.pck_number = get_number_of_packets(bytes_size);
    number_of_packets.total_size = bytes_size;

    packet number_of_packets_packet = {
        .type = PACKET_TYPE_DATA,
        .seqn = 0,
        .total_size = 1,
        .length = sizeof(NUMBER_OF_PACKETS),
        .restart_loop = FALSE,
    };

    memcpy(number_of_packets_packet._payload, &number_of_packets, sizeof(NUMBER_OF_PACKETS));

    write_all_bytes(sock_fd, (char *)&number_of_packets_packet, sizeof(packet));

    int i;
    for (i = 0; i < number_of_packets.pck_number; i++)
    {
        // print_packet(packets[i]);
        write_all_bytes(sock_fd, (char *)&packets[i], sizeof(packet));
    }
}

DATA_RETURN receive_data_with_packets(int sock_fd)
{

    // read NUMBER_OF_PACKETS
    char *buffer = read_all_bytes(sock_fd, sizeof(packet));
    if (buffer == NULL)
    {
        printf("Error reading number of packets\n");
        sleep(1);
        throw "Error reading number of packets";
    }
    packet *number_of_packets_packet = (packet *)buffer;

    if (is_restart_packet(*number_of_packets_packet))
    {
        handle_restart_packet(sock_fd);
    }

    NUMBER_OF_PACKETS *number_of_packets = (NUMBER_OF_PACKETS *)number_of_packets_packet->_payload;

    // printf("Number of packets: %d\n", number_of_packets->pck_number);

    vector<packet> *packets_vector = new vector<packet>();

    for (int i = 0; i < number_of_packets->pck_number; i++)
    {
        buffer = read_all_bytes(sock_fd, sizeof(packet));
        if (buffer == NULL)
        {
            printf("Error reading packet\n");
            sleep(1);
            throw "Error reading packet";
        }
        packet *pck = (packet *)buffer;

        if (is_restart_packet(*number_of_packets_packet))
        {
            handle_restart_packet(sock_fd);
        }

        packets_vector->push_back(*pck);

        // printf("Packet %d received\n", pck->seqn);
    }

    // printf("All packets received - %d\n", number_of_packets->pck_number);

    packet *packets = (packet *)packets_vector;

    char *data_recovered = (char *)malloc(number_of_packets->pck_number * MAX_PACKET_PAYLOAD_SIZE);

    uint64_t read_size = 0;
    for (int i = 0; i < packets_vector->size(); i++)
    {
        packet pck = packets_vector->at(i);
        memcpy(data_recovered + read_size, pck._payload, pck.length);
        read_size += pck.length;
    }

    DATA_RETURN data = {data_recovered, read_size};

    return data;
}

template <typename T>
T *receive_converted_data_with_packets(int sock_fd)
{

    DATA_RETURN data = receive_data_with_packets(sock_fd);

    char *data_recovered = data.data;
    int bytes_size = data.bytes_size;

    T *result = reinterpret_cast<T *>(data_recovered);

    return result;
}

void send_OK_packet(int sock_fd)
{
    packet ok_packet = {
        .type = PACKET_TYPE_CONTROL,
        .seqn = 0,
        .total_size = 1,
        .length = 0,
        .restart_loop = FALSE,
    };

    strcpy(ok_packet._payload, "OK");

    // printf("SENDING OK PACKET TO %d\n", sock_fd);

    write_all_bytes(sock_fd, (char *)&ok_packet, sizeof(packet));
}

void wait_for_OK_packet(int sock_fd)
{
    bool is_ok_packet = FALSE;
    while (!is_ok_packet)
    {
        // printf("Waiting for OK packet from %d\n", sock_fd);
        DATA_RETURN data_return = receive_data_with_packets(sock_fd);
        char *data = data_return.data;
        strcmp(data, "OK") == 0 ? is_ok_packet = TRUE : is_ok_packet = FALSE;
    }
    return;
}

void handle_restart_packet(int sock_fd)
{
    // printf("RECEIVED RESTART PACKET\n");
    send_OK_packet(sock_fd);
    wait_for_OK_packet(sock_fd);
    throw OutOfSyncException();
}