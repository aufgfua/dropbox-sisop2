void redirectMessage(int sock_src, int sock_dst)
{
    cout << "Redirecting message from " << sock_src << " to " << sock_dst << endl;

    char *data = read_all_bytes(sock_src, sizeof(packet));

    cout << "FE>";
    write_all_bytes(sock_dst, data, sizeof(packet));
}