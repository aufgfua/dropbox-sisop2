int syncing_clients = 0;
mutex syncing_clients_mtx;

bool can_sync_clients = true;
int want_to_sync_RM = 0;
mutex want_to_sync_RM_mtx;

int get_syncing_clients()
{
    return 0;
    syncing_clients_mtx.lock();
    int syncing_clients_copy = syncing_clients;
    syncing_clients_mtx.unlock();
    return syncing_clients_copy;
}

int get_want_to_sync_RM()
{
    return 0;
    want_to_sync_RM_mtx.lock();
    int want_to_sync_RM_copy = want_to_sync_RM;
    want_to_sync_RM_mtx.unlock();
    return want_to_sync_RM_copy;
}

void increment_syncing_clients()
{
    return;
    while (get_want_to_sync_RM() > 0)
    {
        this_thread::sleep_for(chrono::milliseconds(1 * 1000));
    }
    syncing_clients_mtx.lock();

    syncing_clients++;
    syncing_clients_mtx.unlock();
}

void decrement_syncing_clients()
{
    return;
    syncing_clients_mtx.lock();
    syncing_clients--;
    syncing_clients_mtx.unlock();
}

void increment_want_to_sync_RM()
{
    return;
    want_to_sync_RM_mtx.lock();
    want_to_sync_RM++;
    can_sync_clients = FALSE;
    want_to_sync_RM_mtx.unlock();
}

void decrement_want_to_sync_RM()
{
    return;
    want_to_sync_RM_mtx.lock();
    want_to_sync_RM--;
    if (want_to_sync_RM == 0)
        can_sync_clients = TRUE;
    want_to_sync_RM_mtx.unlock();
}
