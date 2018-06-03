
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <netinet/in.h>
#include <unistd.h>
#include "server.h"

ClientInfo g_Clients[MAX_CLIENTS_NUM];
int g_ConnectedClientNum = 0;
int g_SendDataToClientIdx = 0;

void InitClientInfo()
{
	int i;

	for(i = 0; i < MAX_CLIENTS_NUM; i++)
	{
		g_Clients[i].IsValid      = SERVER_FALSE;
		g_Clients[i].ClientThread = 0;
		g_Clients[i].ClientFd     = -1;
		memset(g_Clients[i].RecvBuffer, 0, RECV_BUFFER_LEN);
	}
}
int GetAInvalidClientElementIdx()
{
	int i;

	for(i = 0; i < MAX_CLIENTS_NUM; i++)
	{
		if(g_Clients[i].IsValid == SERVER_FALSE)
		{
			return i;
		}
	}
	return SERVER_ERROR;
}
void * ServerRecvMain(void *arg)
{
	int length;
	ClientInfo *client = (ClientInfo *)arg;

	printf("client thread %lu is running...\n",(unsigned long int)pthread_self());
	pthread_detach(pthread_self());

	while(1)
	{
		length = recv(client->ClientFd, client->RecvBuffer, RECV_BUFFER_LEN, 0);

		if(length == 0)
		{
			printf("Connection is break\n");
			break;
		}
		else if(length > 0)
		{
			printf("server receives %d data:\n", length);
            printf("%s\n", client->RecvBuffer);
		}
		else
		{
			printf("receive error\n");
		}
	}

	close(client->ClientFd);
	client->IsValid  = SERVER_FALSE;
	printf("client thread %lu is stopped\n", (unsigned long int)pthread_self());
	return NULL;
}
void *ServerSendFromTerminal(void *args)
{
    int len;
    char input[200] = {0};

    while(NULL != gets(input))//mac环境下gets 不包含'\n'？（开发平台有区别？）
    {
        /****************************************
        此处添加用命令行控制向哪个client发送数据的idx
        *****************************************/

        if(!g_Clients[g_SendDataToClientIdx].IsValid)
            continue;

        len = send(g_Clients[g_SendDataToClientIdx].ClientFd, input, strlen(input), 0);
    }

    return NULL;
}
int CreateNewClientChannel()
{
	//调用socket函数返回的文件描述符  
    int serverFd, clientFd, clientIdx;   
    //声明两个套接字sockaddr_in结构体变量，分别表示客户端和服务器  
    struct sockaddr_in serverAddr;  
    struct sockaddr_in clientAddr;  
    socklen_t addressLen; 

      
    //socket函数，失败返回-1  
    //int socket(int domain, int type, int protocol);  
    //第一个参数表示使用的地址类型，一般都是ipv4，AF_INET  
    //第二个参数表示套接字类型：tcp：面向连接的稳定数据传输SOCK_STREAM  
    //第三个参数设置为0  
    if((serverFd = socket(AF_INET, SOCK_STREAM, 0)) < 0)  
    {  
        perror("socket");  
        return 1;  
    }  
  
    bzero(&serverAddr, sizeof(serverAddr));  
    //初始化服务器端的套接字，并用htons和htonl将端口和地址转成网络字节序  
    serverAddr.sin_family = AF_INET;  
    serverAddr.sin_port = htons(SERVER_PORT);  
    //ip可是是本服务器的ip，也可以用宏INADDR_ANY代替，代表0.0.0.0，表明所有地址  
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);  
    //对于bind，accept之类的函数，里面套接字参数都是需要强制转换成(struct sockaddr *)  
    //bind三个参数：服务器端的套接字的文件描述符，  
    if(bind(serverFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)  
    {  
        perror("connect");  
        return 1;  
    }  
    //设置服务器上的socket为监听状态  
    if(listen(serverFd, 5) < 0)   
    {  
        perror("listen");  
        return 1;  
    }  

    while(1)  
    {  
    	clientIdx = GetAInvalidClientElementIdx();
        if(clientIdx == SERVER_ERROR)
        {
        	printf("服务器连接客户端个数超过 %d 个，不接受新的连接请求\n", MAX_CLIENTS_NUM);
        	continue;
        }

        printf("监听端口: %d\n", SERVER_PORT);  
        //调用accept函数后，会进入阻塞状态  
        //accept返回一个套接字的文件描述符，这样服务器端便有两个套接字的文件描述符，  
        //serverFd和clientFd。  
        //serverFd仍然继续在监听状态，client则负责接收和发送数据  
        //clientAddr是一个传出参数，accept返回时，传出客户端的地址和端口号  
        //addrLen是一个传入-传出参数，传入的是调用者提供的缓冲区的clientAddr的长度，以避免缓冲区溢出。  
        //传出的是客户端地址结构体的实际长度。  
        //出错返回-1  
        clientFd= accept(serverFd, (struct sockaddr*)&clientAddr, &addressLen); 
        if(clientFd < 0)
        {
        	printf("accept error\n");
        	continue;
        }
        printf("receive client:%d\n", clientFd);

        g_Clients[clientIdx].ClientFd = clientFd;
        g_Clients[clientIdx].IsValid  = SERVER_TRUE;
        pthread_create(&g_Clients[g_ConnectedClientNum].ClientThread, NULL, ServerRecvMain, (void *)&g_Clients[clientIdx]);
    }

}
int main(void)
{
    pthread_t sendThread;

	InitClientInfo();
	CreateNewClientChannel();
    pthread_create(&sendThread, NULL, ServerSendFromTerminal, NULL);

    pthread_join(sendThread, NULL);
	return 0;
}