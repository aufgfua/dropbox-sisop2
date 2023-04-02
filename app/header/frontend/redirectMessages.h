void redirectMessage(int sock_src, int sock_dst)
{
    DATA_RETURN data = receive_data_with_packets(sock_src);

    char *data_recovered = data.data;
    int bytes_size = data.bytes_size;

    send_data_with_packets(sock_dst, data_recovered, bytes_size);
}