#define ELECTION_PORT_OFFSET 3000

typedef struct STR_RING_ELECTION_VOTE
{
    bool election_finished;
    unsigned long election_winner_s_addr;
    in_port_t election_winner_port;
    int election_winner_id;

    int election_participant_greatest_id;

} RING_ELECTION_VOTE;

typedef struct STR_ELECTION_PARTICIPANT
{
    unsigned long s_addr;
    int main_mode_port;
    int id;
} ELECTION_PARTICIPANT;

bool sent_vote = FALSE;
int my_id_global;

void become_new_primary()
{
}

void send_vote(RING_ELECTION_VOTE *vote)
{
    sent_vote = TRUE;

    cout << "CK1" << endl;
    RM_CONNECTION next_node = *(get_next_greatest_id_rm_connection(my_id_global));
    cout << "CK2" << endl;
    cout << "Next node IP: " << next_node.str_ip << " and port: " << SERVER_PORT + ELECTION_PORT_OFFSET << endl;
    int sock_fd = connect_socket(gethostbyname(next_node.str_ip), SERVER_PORT + ELECTION_PORT_OFFSET);
    cout << "CK3" << endl;
    send_data_with_packets(sock_fd, (char *)&vote, sizeof(RING_ELECTION_VOTE));
    cout << "CK4" << endl;
}

void *run_election_server(void *data)
{
    ELECTION_PARTICIPANT *my_election_participant = (ELECTION_PARTICIPANT *)data;
    int election_port = my_election_participant->main_mode_port + ELECTION_PORT_OFFSET;
    int my_id = my_election_participant->id;
    my_id_global = my_id;
    int election_sock_fd = init_server(election_port);
    cout << "Election server started on port: " << election_port << endl;

    while (TRUE)
    {
        struct sockaddr_in election_participant_addr;
        socklen_t client_len = sizeof(election_participant_addr);
        int participant_fd = accept(election_sock_fd, (struct sockaddr *)&election_participant_addr, &client_len);
        if (participant_fd < 0)
        {
            std::cerr << "Error accepting election connection" << std::endl;
            continue;
        }

        RM_CONNECTION *participant_conn = (RM_CONNECTION *)malloc(sizeof(RM_CONNECTION));

        participant_conn->sock_fd = participant_fd;
        participant_conn->s_addr = election_participant_addr.sin_addr.s_addr;
        participant_conn->port = SERVER_PORT;
        strcpy(participant_conn->str_ip, inet_ntoa(election_participant_addr.sin_addr));

        cout << "Received connection to participate on election - " << participant_fd << "!" << endl;

        RING_ELECTION_VOTE *vote = receive_converted_data_with_packets<RING_ELECTION_VOTE>(participant_fd);

        if (vote->election_finished)
        {
            if (vote->election_winner_id != my_id)
            {
                // adapt to new primary
                cout << "New primary data: " << vote->election_winner_s_addr << ":" << vote->election_winner_port << " - with id: " << vote->election_winner_id << endl;
            }
            else
            {
                // Ok, it came back to me
                // Become primary

                cout << "I am the new primary!" << endl;
            }
        }
        else if (!vote->election_finished)
        {
            if (vote->election_participant_greatest_id == my_id)
            {
                vote->election_finished = TRUE;
                vote->election_winner_s_addr = my_election_participant->s_addr;
                vote->election_winner_port = my_election_participant->main_mode_port;
                send_vote(vote);
            }
            else if (vote->election_participant_greatest_id > my_id)
            {
                send_vote(vote);
            }
            else
            {
                if (!sent_vote)
                {
                    vote->election_participant_greatest_id = my_id;
                    send_vote(vote);
                }
            }
        }
    }

    return NULL;
}

void begin_election()
{
    cout << "Starting election..." << endl;
    RING_ELECTION_VOTE *vote = (RING_ELECTION_VOTE *)malloc(sizeof(RING_ELECTION_VOTE));
    vote->election_finished = FALSE;
    vote->election_participant_greatest_id = my_id_global;
    if (!sent_vote)
    {
        sent_vote = TRUE;
        send_vote(vote);
    }
}