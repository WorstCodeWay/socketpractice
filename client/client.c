#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <unistd.h>

#include "client.h"

ClientInfo g_ClientInfo = 
{
    CLIENT_STOPPED,       //Status
    -1,                   //Fd
    DEFAULT_SERVER_IP,    //ServerIp
    DEFAULT_SERVER_PORT,  //ServerPort
    {
        {TIMEOUT_CONNECT_SEC,TIMEOUT_CONNECT_USEC},
        {TIMEOUT_RECEIVE_SEC,TIMEOUT_RECEIVE_USEC},
        {TIMEOUT_SEND_SEC   ,TIMEOUT_SEND_USEC}
    },                   //timeout
    {0},                  //Buffer
    NULL,                 //RecvHandler
    NULL                  //HandlerBuff
};

int InitClient()
{
    g_ClientInfo.Fd = socket(AF_INET, SOCK_STREAM, 0);
    if(g_ClientInfo.Fd < 0)
    {
        printf("Create socket error\n");
        return CLIENT_CREAT_SOCKET_ERROR;
    }

    memset(g_ClientInfo.Buffer, 0, CLIENT_BUFFER_SIZE);

    return CLIENT_OK;
}
/* int ConnectSever()
** 连接到指定服务器，（间接）采用超时连接机制。
*/
int ConnectSever()
{
    int ret, flag, connRet, selecRet, error;
    socklen_t len = sizeof(int);

    TimeInfo   connTimeout = g_ClientInfo.Timeout.Connect;//防止select函数修改超时计数器，使用本地变量
    fd_set     writeset;
    SockaddrIn serveraddr;


    memset(&serveraddr, 0, sizeof(SockaddrIn));
    serveraddr.sin_family      = AF_INET;
    serveraddr.sin_port        = htons(g_ClientInfo.Port);
    serveraddr.sin_addr.s_addr = inet_addr(g_ClientInfo.ServerIp);

/*step 1: non-blocking mode*/
    flag = fcntl(g_ClientInfo.Fd, F_GETFL, 0);
    fcntl(g_ClientInfo.Fd, F_SETFL, flag | O_NONBLOCK);    //non-blocking

    printf("connecting server %s %d\n", g_ClientInfo.ServerIp, g_ClientInfo.Port);
/*step 2: connect*/
    connRet = connect(g_ClientInfo.Fd, (Sockaddr *)&serveraddr, sizeof(serveraddr));

/*step 3: check return value and error number*/
    if(connRet == -1)
    {
        if(errno != EINPROGRESS)
        {
            printf("Connect error: %s\n", strerror(errno));
            if(errno == EISCONN)
            {
                close(g_ClientInfo.Fd);
            }
            ret = -1;
        }
        else
        {
/*step 4: use select to perferm timeout purpose*/
            FD_ZERO(&writeset);
            FD_SET(g_ClientInfo.Fd, &writeset);

            printf("connect timeout %ld sec\n", connTimeout.tv_sec);

            selecRet = select(g_ClientInfo.Fd+1, NULL, &writeset, NULL, &connTimeout);
            if(selecRet < 0)
            {
                printf("Connect error: %s\n", strerror(errno));
                ret = -1;
            }
            else if(selecRet == 0)
            {
                printf("connect timeout\n");
                ret = -1;
            }
            else 
            {
/*step 5: check if socket writeable*/
                if(FD_ISSET(g_ClientInfo.Fd, &writeset))
                {
                    if(getsockopt(g_ClientInfo.Fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
                    {
                        printf("[%d]%s\n",error, strerror(error));
                        ret = -1;
                    }
                    else
                    {
                        if(error == 0)
                        {
                            printf("Connect success\n");
                            ret = 0;
                        }
                        else
                        {
                            printf("Connect failed:[%d]%s\n",error, strerror(error));
                            ret = -1;
                        }
                    }
                }
                else
                {
                    printf("Socket is not writeable\n");
                    ret = -1;
                }
            }
        }
    }//if(connRet == -1)
    else
        ret = 0;
/*step 6: blocking mode*/
    fcntl(g_ClientInfo.Fd, F_SETFL, flag & ~O_NONBLOCK);   //blocking

    if(ret < 0)
    {
        close(g_ClientInfo.Fd);
    }
    return ret;
}
void SetRecvSendTimeout()
{
    if(setsockopt(g_ClientInfo.Fd, SOL_SOCKET,SO_SNDTIMEO, (char *)&g_ClientInfo.Timeout.Send,sizeof(TimeInfo)))
    {
        printf("Set send timeout failed\n");
    }
    
    if(setsockopt(g_ClientInfo.Fd, SOL_SOCKET,SO_RCVTIMEO, (char *)&g_ClientInfo.Timeout.Receive,sizeof(TimeInfo)))
    {
        printf("Set receiveb timeout failed\n");
    }

}
void * ClientSendFromTerminal(void *args)
{
    int len;
    char input[200] = {0};

    while(NULL != gets(input))//mac环境下gets 不包含'\n'？（开发平台有区别？）
    {
        len = send(g_ClientInfo.Fd, input, strlen(input), 0);
    }

    return NULL;
}
int ClientRecv()
{
    int recvNum = 0;

    recvNum = recv(g_ClientInfo.Fd, g_ClientInfo.Buffer, CLIENT_BUFFER_SIZE, 0);

    if(recvNum == 0)
    {
        printf("Connection is break\n");
        return CLIENT_CONNECTION_BREAK;
    }
    if(recvNum < 0)
    {
        return CLIENT_RECV_TIMEOUT;
    }
    if(recvNum > 0)
    {
        if(NULL == g_ClientInfo.RecvHandler)
            return CLIENT_RECV_HANDLER_UNSET;

        g_ClientInfo.RecvHandler((void *)g_ClientInfo.Buffer, recvNum);
        
    }
    return CLIENT_OK;
}
void ClientChangeServer(char *ip, unsigned short int port)
{
    strcpy(g_ClientInfo.ServerIp, ip);
    g_ClientInfo.Port = port;

    printf("change to server ip:%s port:%d\n", g_ClientInfo.ServerIp, g_ClientInfo.Port);
    g_ClientInfo.Status = CLIENT_CHANGE_SERVER;
}
void SetClientRecvHandler(Handler handler)
{
    g_ClientInfo.RecvHandler = handler;
}
void RunClient()
{
    g_ClientInfo.Status = CLIENT_INIT;
}
void StopClient()
{
    close(g_ClientInfo.Fd);
    g_ClientInfo.Status = CLIENT_STOPPED;
}
void ResumeClient()
{
    g_ClientInfo.Status = CLIENT_RUNNING;
}
void PauseClient()
{
    g_ClientInfo.Status = CLIENT_STOPPED;
}

void *ClientMain(void *arg)
{
    int recvRet = 0;

    while(1)
    {
        switch(g_ClientInfo.Status)
        {
            case CLIENT_INIT:
                printf("CLIENT_INIT\n");
                if(InitClient() == CLIENT_OK)
                    g_ClientInfo.Status = CLIENT_CONNECT;
                else
                    usleep(500000);//0.5 s
                break;

            case CLIENT_CONNECT:
                printf("CLIENT_CONNECT\n");
                if(ConnectSever() == CLIENT_OK)
                    g_ClientInfo.Status = CLIENT_RUNNING;
                else
                {
                    g_ClientInfo.Status = CLIENT_INIT;
                }
                usleep(500000);//0.5 s
                break;

            case CLIENT_RUNNING:
                SetRecvSendTimeout();
                recvRet = ClientRecv();

                if(CLIENT_CONNECTION_BREAK == recvRet)
                {
                    g_ClientInfo.Status = CLIENT_CONNECT;
                }
                else if(CLIENT_RECV_TIMEOUT == recvRet)
                {
                    usleep(100000);//100ms
                }
                break;

            case CLIENT_CHANGE_SERVER:
                close(g_ClientInfo.Fd);
                g_ClientInfo.Status = CLIENT_INIT;
                usleep(100000);
                break;

            default:
                usleep(100000);
                break;
        }

    }

}
