#include <sys/time.h>
#include <pthread.h>
#include "client.h"

int main(void)
{
    pthread_t clientThread;
    pthread_t clientSendThread;

    pthread_create(&clientThread, NULL, ClientMain, NULL);
    pthread_create(&clientSendThread, NULL, ClientSendFromTerminal, NULL);

    RunClient();
    pthread_join(clientThread, NULL);
    pthread_join(clientSendThread, NULL);
    return 0;
}
