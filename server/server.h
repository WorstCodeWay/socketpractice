#ifndef __SERVER_H__
#define __SERVER_H__

#define SERVER_STATUS int
#define SERVER_OK        0
#define SERVER_ERROR     -1

#define SERVER_TRUE      1
#define SERVER_FALSE     0

#define MAX_CLIENTS_NUM  5
#define RECV_BUFFER_LEN  1024

#define SERVER_PORT 8000

/*re-define standard type of lib*/
typedef struct timeval       TimeInfo;
typedef struct sockaddr_in   SockaddrIn;
typedef struct sockaddr      Sockaddr;

typedef struct client_info
{
	char      IsValid;
	pthread_t ClientThread;
	int       ClientFd;
	char      RecvBuffer[RECV_BUFFER_LEN];
}ClientInfo;


extern void InitClientInfo();


#endif
