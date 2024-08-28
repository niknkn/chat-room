#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#define PORT 8888
#define IP "192.168.10.118"
 
//线程接收服务器消息
void* recv_info(void* newfd){
    int fd = *(int*)newfd;
    char shou[1024];
    while (1){
        memset(shou, 0, 1024);
        int r = recv(fd, shou, 1024, 0);
        if (r == -1){
            printf("recv error:%m\n");
            exit(1);
        }
        printf("%s\n",shou);
    }
}
 
//线程用来向服务器发送信息
void* send_info(void* newfd){
    int fd = *(int*)newfd;
    char fa[1024];
    while (1){
        memset(fa, 0, 1024);
        scanf("%s", fa);
        int r=send(fd, fa, strlen(fa), 0);
        if ( r == -1){
            printf("send error:%m\n");
            exit(1);
        }

        if (strcmp(fa, "end") == 0){//发送end结束客户端程序
            printf("欢迎使用,下次再见\n");
            close(fd);//关闭
            exit(0);
        }
    }
}
 
 
int main()
{
    int sockfd = 0;//定义并初始化
    int len = sizeof(struct sockaddr);
    struct sockaddr_in otheraddr;
    memset(&otheraddr, 0, len);
    //创建连接
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        printf("sockfd error:%m\n");
        return -1;
    }
    else{
        printf("socket success\n");
    }

    otheraddr.sin_family = AF_INET;
    otheraddr.sin_port = htons(PORT);
    otheraddr.sin_addr.s_addr = inet_addr(IP);

    //连接服务器
    if (connect(sockfd, (struct sockaddr*)&otheraddr, len) == -1){
        printf("connect error:%m\n");
        return -1;
    }else{
        printf("connect success\n");
        printf("客户端已连接，ip为%s\n", inet_ntoa(otheraddr.sin_addr));
    }

    //创建线程
    pthread_t id1, id2;
    char recvbuf[1024];
    char sendbuf[1024];
    memset(recvbuf, 0, 1024);
    memset(sendbuf, 0, 1024);
    //给服务器发送信息，判断是否建立连接		
    strcpy(sendbuf, "客户端上线 \n");

    if (send(sockfd, sendbuf, strlen(sendbuf), 0) == -1){
        printf("send error:%m\n");
        return -1;
    }
    if (recv(sockfd, recvbuf, 1024, 0) == -1){
        printf("recv error:%m\n");
        return -1;
    }
    else{
        //登录成功收到服务器发来的主页面
        printf("登陆成功:%s\n", recvbuf);
    }
    //启动客户端线程的收发功能
    pthread_create(&id2, NULL, send_info, &sockfd);
    pthread_create(&id1, NULL, recv_info, &sockfd);
    //等待发送线程结束，退出客户端
    pthread_join(id2, NULL);
    return 0;
}
 
 
 