#ifndef __CLIENT_H__
#define __CLIENT_H__

#define CLIENT_OK                 0
#define CLIENT_CREAT_SOCKET_ERROR 1 /*create socket failed*/
#define CLIENT_CONNECTION_BREAK   2 /*connection is break*/
#define CLIENT_RECV_TIMEOUT       3 /*receive data timeout*/
#define CLIENT_RECV_HANDLER_UNSET 4 /*the receive handler is not set*/

#define CLIENT_BUFFER_SIZE 1024	  //1k

#define DEFAULT_SERVER_IP   "127.0.0.1"
#define DEFAULT_SERVER_PORT 8000

#define TIMEOUT_CONNECT_SEC    1     //1 s
#define TIMEOUT_CONNECT_USEC   0     //0 us

#define TIMEOUT_RECEIVE_SEC    0     //0 s
#define TIMEOUT_RECEIVE_USEC   (5e5) //500,000 us

#define TIMEOUT_SEND_SEC       0     //0 s
#define TIMEOUT_SEND_USEC      (2e5) //200,000 us

/*re-define standard type of lib*/
typedef struct timeval       TimeInfo;
typedef struct sockaddr_in   SockaddrIn;
typedef struct sockaddr      Sockaddr;


typedef int (*Handler)(void *, int);

typedef enum client_status
{
	CLIENT_STOPPED       = 0,
	CLIENT_INIT          = 1,
	CLIENT_CONNECT       = 2,
	CLIENT_RUNNING       = 3,
	CLIENT_CHANGE_SERVER = 4

}ClientSts;

typedef struct client_timeout
{
	TimeInfo Connect;
	TimeInfo Receive;
	TimeInfo Send;
}ClientTimeout;

typedef struct client_info
{
	ClientSts Status;
	int Fd;
	char ServerIp[20];
	unsigned short int Port;
	ClientTimeout Timeout;
	char Buffer[CLIENT_BUFFER_SIZE];
	Handler RecvHandler;
	char * HandlerBuff;
}ClientInfo;


extern void *ClientMain(void *arg);
extern void * ClientSendFromTerminal(void *args);
extern void ClientChangeServer(char *ip, unsigned short int port);
extern void SetClientRecvHandler(Handler handler);
extern void RunClient();
extern void StopClient();
extern void ResumeClient();
extern void PauseClient();

#endif