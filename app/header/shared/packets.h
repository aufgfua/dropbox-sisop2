#define MAX_PACKET_PAYLOAD_SIZE 1024
#define PACKET_TYPE_DATA 1
#define PACKET_TYPE_CONTROL 2

#define OK_PACKET_QUIET 0
#define OK_PACKET_LOG 1

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
    cout << "Type: " << pck.type << ", Seqn: " << pck.seqn << ", Total size: " << pck.total_size << ", Length: " << pck.length << ", Value: " << pck._payload << endl;
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
    if (LOG_MODE)
        cout << "Reading " << bytes_to_read << " bytes from socket" << endl;

    int total_bytes_read = 0;
    char *buffer = (char *)malloc(bytes_to_read);
    if (buffer == NULL)
    {
        // Handle memory allocation error
        cout << "Error allocating memory" << endl;
        return NULL;
    }
    while (bytes_to_read > 0)
    {
        int bytes_received = recv(sockfd, buffer + total_bytes_read, bytes_to_read, 0);

        if (LOG_MODE)
            cout << "Bytes received - " << bytes_received << endl;

        if (bytes_received == 0)
        {
            cout << "!!! - Connection closed on socket " << sockfd << " - !!!" << endl;
            this_thread::sleep_for(chrono::milliseconds(1 * 1000));
            throw ConnectionLostException();
            break;
        }

        if (bytes_received < 0)
        {
            cout << "Error reading from socket" << endl;
            this_thread::sleep_for(chrono::milliseconds(1 * 1000));
            throw "Error reading from socket\n";
            break;
        }
        total_bytes_read += bytes_received;
        bytes_to_read -= bytes_received;
    }
    if (bytes_to_read != 0)
    {
        cout << "Invalid length reading from socket" << endl;
        this_thread::sleep_for(chrono::milliseconds(1 * 1000));
        throw "Invalid length reading from socket\n";
        return NULL;
    }
    return buffer;
}

int write_all_bytes(int sockfd, char *buffer, int bytes_to_write)
{
    if (LOG_MODE)
        cout << "Writing " << bytes_to_write << " bytes to socket " << endl;
    int total_bytes_written = 0;
    while (bytes_to_write > 0)
    {
        int bytes_sent = send(sockfd, buffer + total_bytes_written, bytes_to_write, 0);
        if (LOG_MODE)
            cout << "Bytes sent - " << bytes_sent << endl;
        cout << "Bytes sent - " << bytes_sent << " on socket " << sockfd << endl;
        if (bytes_sent <= 0)
        {
            // Handle error or connection closed
            cout << "Error writing to socket" << endl;
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

    cout << "SENDING RESTART PACKET TO " << sock_fd << endl;

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
        cout << "Error reading number of packets" << endl;
        this_thread::sleep_for(chrono::milliseconds(1 * 1000));
        throw "Error reading number of packets";
    }
    packet *number_of_packets_packet = (packet *)buffer;

    if (is_restart_packet(*number_of_packets_packet))
    {
        handle_restart_packet(sock_fd);
    }

    NUMBER_OF_PACKETS *number_of_packets = (NUMBER_OF_PACKETS *)number_of_packets_packet->_payload;

    vector<packet> *packets_vector = new vector<packet>();

    for (int i = 0; i < number_of_packets->pck_number; i++)
    {
        buffer = read_all_bytes(sock_fd, sizeof(packet));
        if (buffer == NULL)
        {
            cout << "Error reading packet" << endl;
            this_thread::sleep_for(chrono::milliseconds(1 * 1000));
            throw "Error reading packet";
        }
        packet *pck = (packet *)buffer;

        if (is_restart_packet(*number_of_packets_packet))
        {
            handle_restart_packet(sock_fd);
        }

        packets_vector->push_back(*pck);
    }

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

template <typename TSV>
void send_vector_of_data_with_packets(int sock_fd, vector<TSV> data_vector)
{
    size_t number_of_elements = data_vector.size();

    TSV *data = data_vector.data();

    send_data_with_packets(sock_fd, (char *)&number_of_elements, sizeof(size_t));

    for (int i = 0; i < number_of_elements; i++)
    {
        TSV *element = &data[i];
        send_data_with_packets(sock_fd, (char *)element, sizeof(TSV));
    }
}

template <typename TRV>
vector<TRV> receive_vector_of_data_with_packets(int sock_fd)
{
    int *number_of_elements = receive_converted_data_with_packets<int>(sock_fd);

    vector<TRV> result;
    for (int i = 0; i < *number_of_elements; i++)
    {
        TRV *element = receive_converted_data_with_packets<TRV>(sock_fd);

        result.push_back(*element);
    }

    return result;
}

void send_OK_packet(int sock_fd, int mode)
{
    packet ok_packet = {
        .type = PACKET_TYPE_CONTROL,
        .seqn = 0,
        .total_size = 1,
        .length = 0,
        .restart_loop = FALSE,
    };

    strcpy(ok_packet._payload, "OK");

    write_all_bytes(sock_fd, (char *)&ok_packet, sizeof(packet));

    if (mode == OK_PACKET_LOG)
    {
        cout << "OK packet sent" << endl;
    }
}

void wait_for_OK_packet(int sock_fd, int mode)
{
    bool is_ok_packet = FALSE;
    while (!is_ok_packet)
    {

        packet *possible_ok_packet = (packet *)read_all_bytes(sock_fd, sizeof(packet));
        char *data = possible_ok_packet->_payload;

        strcmp(data, "OK") == 0 ? is_ok_packet = TRUE : is_ok_packet = FALSE;
    }

    if (mode == OK_PACKET_LOG)
    {
        cout << "OK packet received" << endl;
    }
    return;
}

void handle_restart_packet(int sock_fd)
{
    send_OK_packet(sock_fd, OK_PACKET_LOG);
    wait_for_OK_packet(sock_fd, OK_PACKET_LOG);
    throw OutOfSyncException();
}