void redirectMessage(int sock_src, int sock_dst)
{
    char *data = read_all_bytes(sock_src, sizeof(packet));

    write_all_bytes(sock_dst, data, sizeof(packet));
}