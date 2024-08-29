
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <time.h>
#include <pthread.h>

#define PORT 8888
#define IP "192.168.10.118"
int sockfd = 0; 
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


        shou[r] = '\0'; // 确保字符串以 '\0' 结尾  
        
        if (strncmp(shou, "start:\n", 7) == 0)
        {   
            printf("开始接收文件\n");

            //接收文件大小
            memset(shou, 0, 1024);  
            recv(fd, shou, 1023, 0);
            int fileSize = atoi(shou);
            printf("接收到的文件大小是%d\n",fileSize);

            // 接收文件名  
            char filename[256];
            memset(filename, 0, 256);  
            memset(shou, 0, 1024);  
            if ((r = recv(fd, shou, 1023, 0)) <= 0) {  
                if (r == -1) exit(1);  
                break;  
            }  
            shou[r] = '\0';  
            strncpy(filename, shou, strlen(filename)+7);  
            printf("接收到的文件名是%s\n",filename);

            //创建并打开文件
            int newFd = open(filename,O_WRONLY|O_CREAT,0666);
            if(-1 == newFd){
                printf("文件创建失败\n");
                exit(-1);
            }
            printf("文件创建成功\n");

            // 循环接收数据并写入文件
            int cnt = 0;
            while (cnt < fileSize) {
                memset(shou, 0, 1024);
              
                int r = recv(fd, shou, 1024, 0);
                if (r > 0) {
                    write(newFd, shou, r);
                    cnt += r;
                } else if (r == 0) {
                    printf("File transmission finished by the server.\n");
                    break;
                } else {
                    perror("recv failed");
                    break;
                }
            }
            printf("文件接收结束,总共接收 %d 字节\n", cnt);
            close(newFd); 
        }else {
            printf("%s\n",shou);
        }    
    }
}
 
//线程用来向服务器发送信息
void* send_info(void* newfd){
    int fd = *(int*)newfd;
    char fa[1024];
    while (1){
        memset(fa, 0, 1024);
        scanf("%s", fa);

        if (strcmp(fa, "send") == 0){
            printf("客户端准备发送文件\n");

            memset(fa, 0, 1024);
            strcpy(fa,"send");
            send(sockfd, fa, strlen(fa), 0);

            sleep(2);

            printf("输入你要发送的文件名: 输入quit退出\n");
            memset(fa, 0, 1024);
            scanf("%s", fa);

            if (strcmp(fa, "quit") == 0) continue;;

            char filname[256];
            memset(filname,0,256);
            strcpy(filname,fa);
            int rfd = open(filname,O_RDONLY);
            if(-1 == rfd){
                printf("请检查文件是否正确\n");
                continue;
            }
            printf("打开文件成功\n");

            send(sockfd, fa, strlen(fa), 0);

            sleep(2);

            //获取文件大小
            int fileSize = lseek(rfd,0,SEEK_END);
            lseek(rfd,0,SEEK_SET);
            int temp = fileSize;
            //发送文件大小
            memset(fa, 0, 1024);
            sprintf(fa, "%d", fileSize);
            send(sockfd, fa, strlen(fa), 0);

            sleep(2);

            //循环读取并发送文件内容
            char buff[256];
            int count = 0;
            while(count < temp){
                memset(buff,0,255);
                int r = read(rfd,buff,255);
                count += r;
                if(r <= 0) break;
                memset(fa, 0, 1024);
                strcpy(fa, buff);
                send(sockfd, fa, strlen(fa), 0);
            }
            printf("文件发送完成\n");
            close(rfd);  

        }else{

            int r=send(fd, fa, strlen(fa), 0);
            if ( r == -1){
            printf("send error:%m\n");
            exit(1);
        }

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
    //定义并初始化
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
 
 
 