#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>
#include <memory.h>
#define PORT 8888
#define IP "192.168.10.118"
#define GLY "管理员"
#define MM  "密码"

typedef struct xinxi{
	char id[20];          //每个账号唯一id
	char passwd[20];	 //账号密码
	char name[20];	 //账号昵称
	int  fd;              //存放客户端成功连接后accept产生的新的套接字
	struct xinxi* next;	 //下一条链表的首地址	
}User;
#define SIZE sizeof(User)   //定义一个结构体大小空间

int count = 0;            //保存帐号个数
User* head = NULL;

//保存账户信息到文件
void Save_account(){
	FILE* fp;
	fp = fopen("123", "w");    //123用于保存所有帐号信息
	if (fp == NULL){
		printf("open error:%m\n");
		return;
	}

	fwrite(&count, sizeof(int), 1, fp);		//先保存账号是第几个	
	User* p;
	p = head;
	if (p->next == NULL){		//如果账号链表为空，关闭文件并退出函数
		fclose(fp);
		return;
	}
	p = p->next;
	while (p)                //按结构体大小保存账号信息
	{
		fwrite(p, SIZE, 1, fp);
		p = p->next;
	}
	printf("账号信息保存成功\n");
	fclose(fp);
}
 
//创建账号信息的头指针，并从文件里读取保存已注册的账号信息
User* create(){
	User* x;
	x = (User*)malloc(SIZE);                    //定义一个结构体大小的空间
	if (x == NULL){
		printf("create error\n");
		return NULL;                   //如果没有创建成功指针返回NULL
	}

	x->next = NULL;
	FILE* fp;
	fp = fopen("123", "a+");	     //a+方式打开，读取文件，以附加方式打开可读写的文件。若文件不存在，则会建立该文件，如果文件存在，写入的                                                               数据会被加到文件尾后，即文件原先的内容会被保留。
	if (fp == NULL){
		printf("open error\n");
		return NULL;
	}

	if (fgetc(fp) == -1){   	//如果文件为空，返回头指针
		fclose(fp);
		return x;
	}

	fseek(fp,0,SEEK_SET); 		//文件指针重返文件头
	int n;
	fread(&n, sizeof(int), 1, fp);		//先读取4字节个数长度
	//printf("n=%d\n", n);
	count = n;                   //全局变量获取账号长度
	User t; 					//新建临时结构体存储
	User*p, *p1;
	for (int i = 0; i < n; i++){				//按账号结构体长度读取信息
		
		fread(&t,SIZE, 1, fp);  			//临时结构体存储从文件中读取到的数据
		p1 = x;
		while (p1->next){
			p1 = p1->next;
		} 									//链表指针到链表尾

		p = (User*)malloc(SIZE);         //每次创建一个新的账号结构体大小空间来存信息
		p->next = NULL;
		strcpy(p->id, t.id);				//账号，昵称，密码读取保存
		strcpy(p->name, t.name);
		strcpy(p->passwd, t.passwd);
		p->fd = -5;                        //每次服务器新连接字置-5，不在线
		p1->next = p;
	}
	fclose(fp);
	return x;
}
 
//注册功能函数
void Sign_in(int fd){
	User*p1, *p;
	int flag = 0;                             //标识符，账号是否能正确注册
	p = (User*)malloc(SIZE);			//开辟一个账号信息结构体大小的空间
	if (p == NULL) return;

	char str[256];
	char str1[256];
	memset(str, 0, 256);
	memset(str1, 0, 256);

	strcpy(str, "请输入你想注册的账号");
	send(fd, str, strlen(str), 0);
	memset(str, 0, 256);
	recv(fd, str, 256, 0); 			 	//有注册需求时临时发送并接受存储账号
	strcpy(str1, str);

	p1 = head;
	while (p1->next){
		if (strcmp(p1->next->id, str) == 0){ //id相同则取消该行为
			flag = 1;
			break;
		}
		p1 = p1->next;
	}
	if (flag == 1){
		memset(str, 0, 256);
		strcpy(str, "账号重复\n");
		send(fd, str, strlen(str), 0);
		return;
	}

	//注册账号信息并赋初值
	strcpy(p->id, str);
	memset(str, 0, 256);

	strcpy(str, "请输入密码");   		//每次都是临时发送临时接收,存入链表
	send(fd, str, strlen(str), 0);
	memset(str, 0, 256);
	recv(fd, str, 256, 0);
	strcpy(p->passwd, str);
	memset(str, 0, 256);

	strcpy(str, "请输入昵称");
	send(fd, str, strlen(str), 0);
	memset(str, 0, 256);
	recv(fd, str, 256, 0);
	strcpy(p->name, str);

	p1 = head; 				//尾插
	while (p1->next){
		p1 = p1->next;
	}
	p1->next = p;
	p->fd = -5;
	p->next = NULL;
	memset(str, 0, 256);

	strcpy(str, "注册成功\n");
	send(fd, str, strlen(str), 0);
	count++;                          //账号数量+1
	Save_account();		              //保存一次账号信息在文件  重写链表文件
}

//接收文件
void Recv_File(int fd){
	time_t timep;
	time(&timep);  			//时间函数
	printf("服务器准备接收文件\n");
	//服务器接收客户端文件
	//进入接收，输入quit退出
	char shou[1024];
	memset(shou, 0, 1024);
    recv(fd, shou, 1024, 0);

	if (strcmp(shou, "quit") == 0) return;

	char filname[256];
	memset(filname,0,256);
	strcpy(filname,shou);
	int newFd = open(filname,O_WRONLY|O_CREAT,0666);
	if(-1 == newFd){
		printf("文件创建失败\n");
		return;
	}
	printf("文件创建成功\n");

	//接收文件大小
	memset(shou, 0, 1024);
    recv(fd, shou, 1024, 0);
	int fileSize = atoi(shou);
    printf("接收到的文件大小是%d\n",fileSize);

	// 循环接收数据并写入文件
	int cnt = 0;
	while (cnt < fileSize) {
		memset(shou, 0, 1024);
		int r = recv(fd, shou, 1024, 0);
		if (r > 0) {
			write(newFd, shou, r);
			cnt += r;
		} else if (r <= 0) {
			perror("recv failed");
			break;
		}
	}
	printf("文件接收结束,总共接收 %d 字节\n", cnt);
	close(newFd); 
   
}

//传输文件
void Send_File(int fd, char id[20]){
    time_t timep;
	time(&timep);  			//时间函数
	User* p;
	p = head->next;
	while (p){
		if (strcmp(p->id, id) == 0)
			break;
		p = p->next;
	}
    char shou[1024];
	char fa[1024];
	memset(shou, 0, 1024);
	memset(fa, 0, 1024);
	strcpy(fa, "输入你发送文件的好友id: 输入over退出传输");
	send(fd, fa, strlen(fa), 0);
    recv(fd, shou, 1024, 0);
	User* p1;
	p1 = head->next;
	while (p1){
		if (strcmp(p1->id, shou) == 0)
			break;
		p1 = p1->next;
	}
	if (p1 == NULL){
		memset(fa, 0, 1024);
		strcpy(fa, "此好友不存在");
		send(fd, fa, strlen(fa), 0);
		return;
	}
    //进入传输，输入over退出	
	memset(fa, 0, 1024);
	strcpy(fa, "请输入要传输的文件名:\t");
	send(fd, fa, strlen(fa), 0);
	memset(shou, 0, 1024);
	recv(fd, shou, 1024, 0);

	if (strcmp(shou, "over") == 0) return;

	char filname[256];
	memset(filname,0,256);
	strcpy(filname,shou);
	int cfd = open(filname,O_RDONLY);
	if(-1 == cfd){
		memset(fa, 0, 1024);
		strcpy(fa, "请确认文件是否存在\n");
		send(fd, fa, strlen(fa), 0);
		return;
	}
	//获取文件大小
	int fileSize = lseek(cfd,0,SEEK_END);
    lseek(cfd,0,SEEK_SET);

	//发送文件传输提示
	memset(fa, 0, 1024);
	strcpy(fa,"start:\n");
	send(p1->fd, fa, strlen(fa), 0);

	sleep(2);

	//发送文件大小
	memset(fa, 0, 1024);
	sprintf(fa, "%d", fileSize);
	send(p1->fd, fa, strlen(fa), 0);

	sleep(2);

	//发送文件名给对方
	memset(fa, 0, 1024);
	strcpy(fa,filname);
	send(p1->fd, fa, strlen(fa), 0);

	sleep(3);

	//循环读取并发送文件内容
	char buff[256];
	while(1){
		memset(buff,0,255);
		int r = read(cfd,buff,255);
		if(r <= 0) break;
		memset(fa, 0, 1024);
		strcpy(fa, buff);
		send(p1->fd, fa, strlen(fa), 0);
	}
	memset(fa, 0, 1024);
	strcpy(fa, "finish\n");
	send(fd, fa, strlen(fa), 0);
	close(cfd);     
}


char a[20][20];	//放聊天室人的id
int len = 0;		//聊天室人数
//创建多人聊天室
void Multiplayer_chat(int fd, char id[20]){
	char fa[1024];
	char shou[1024];
	memset(fa, 0, 1024);
	memset(shou, 0, 1024);
	strcpy(fa, "您已进入聊天室,输入exit退出,输入look查看当前人");
	send(fd, fa, strlen(fa), 0);

	strcpy(a[len], id);
	len++;					//每进入一个人，长度加1
	int i;
	User* p;
	time_t timep;
	time(&timep);  			//时间函数
	while (1){			//建立聊天室基本信息
		memset(shou, 0, 1024);
		recv(fd, shou, 1024, 0);

		if (strcmp(shou, "exit") == 0){		//exit退出聊天室
			for (i = 0; i < len; i++){
				if (strcmp(a[i], id) == 0){   //退出的人的id被后一个人覆盖
					while (i < len - 1){
						strcpy(a[i], a[i + 1]);
						i++;
					}
				}
			}
			len--;

			for (i = 0; i < len; i++){//告知客户端有人退出聊天室
				p = head->next;
				while (p){
					if (strcmp(p->id, a[i]) == 0){
						memset(fa, 0, 1024);
						sprintf(fa, "%s退出聊天室", id);
						send(p->fd, fa, strlen(fa), 0);
						break;
					}
					p = p->next;
				}
			}
			return;
		}


		if (strcmp(shou, "look") == 0){         //look查看聊天室有多少人，并显示出他们的昵称以及账号
			memset(fa, 0, 1024);
			sprintf(fa, "当前聊天室有%d人,他们是：", len);
			send(fd, fa, strlen(fa), 0);
			for (i = 0; i < len; i++){
				p = head->next;
				while (p){
					if (strcmp(p->id, a[i]) == 0){
						memset(fa, 0, 1024);
						sprintf(fa, "昵称是%s  账号是%s\n", p->name, p->id);
						send(fd, fa, strlen(fa), 0);
						break;
					}
					p = p->next;
				}
			}
			continue;                         //look之后继续执行程序
		}

		for (i = 0; i < len; i++){        //正常发送的消息对聊天室里所有人发出
			p = head->next;
			while (p)
			{
				if (strcmp(p->id, a[i]) == 0 && strcmp(p->id, id) != 0)
				{
					memset(fa, 0, 1024);
					sprintf(fa, "%s%s : %s", ctime(&timep), id, shou);
					send(p->fd, fa, strlen(fa), 0);
					break;
				}
				p = p->next;
			}
		}
	}
}
 
//私聊函数 
void Direct_Message(int fd, char id[20]){
	time_t timep;
	time(&timep);  			//时间函数
	User* p;
	p = head->next; 		//找到当前的人的id
	while (p){
		if (strcmp(p->id, id) == 0)
			break;
		p = p->next;
	}

	char shou[1024];
	char fa[1024];
	memset(shou, 0, 1024);
	memset(fa, 0, 1024);

	strcpy(fa, "输入你想聊天的好友id: 输入over退出私聊");
	send(fd, fa, strlen(fa), 0);
	recv(fd, shou, 1024, 0);      //得到id

	User* p1;
	p1 = head->next;
	while (p1){ 					//找到id
		if (strcmp(p1->id, shou) == 0)
			break;
		p1 = p1->next;
	}
	if (p1 == NULL){
		memset(fa, 0, 1024);
		strcpy(fa, "此好友不存在");
		send(fd, fa, strlen(fa), 0);
		return;
	}

	while (1){					//进入聊天，输入over退出
		memset(shou, 0, 1024);
		recv(fd, shou, 1024, 0); //接收A说的
		if (strcmp(shou, "over") == 0){
			break;
		}
		memset(fa, 0, 1024);
		sprintf(fa, "%s%s :%s", ctime(&timep), id, shou); //处理后发给B
		send(p1->fd, fa, 1024, 0);
	}
}
 
 
//检查账号是否存在链表中
int Verify_account(char id[20]){
	User* p;
	int leap = 0;
	if (head->next == NULL){
		return 0;
	}
	p = head->next;

	while (p){
		if (strcmp(id, p->id) == 0){
			return 1;
		}
		p = p->next;
	}
	return 0;
}
 
//验证密码和帐号是否匹配
int Authentication_password(char id[20], char passwd[20]){
	User* p;
	int leap = 0;
	if (head->next == NULL){
		return 0;
	}

	p = head->next;
	while (p){
		if (strcmp(id, p->id) == 0 && strcmp(passwd, p->passwd) == 0){
			return 1;
		}
		p = p->next;
	}
	return 0;
}
 
//服务器信息处理线程 
void* handleclient(void* newfd)
{
	int fd = *(int*)newfd;
	char recvbuf[1024];
	char recvbuf1[1024];
	char sendbuf[1024];
	memset(recvbuf, 0, 1024);
	memset(recvbuf1, 0, 1024);
	memset(sendbuf, 0, 1024);
	if (recv(fd, recvbuf, 1024, 0) == -1){
		printf("recv error:%m\n");						//错误提示
		exit(1);
	}else{
		printf("%s\n", recvbuf); 						//输出握手信息
	}
p:
	while (1){
		memset(sendbuf, 0, 1024);
		//登录后界面
		strcpy(sendbuf, "请选择进行的操作：\n1.登录帐号\n2.注册帐号\nsend.传输文件\nend.退出\n");
		send(fd, sendbuf, strlen(sendbuf), 0);
		memset(recvbuf, 0, sizeof(recvbuf));
		recv(fd, recvbuf, 1024, 0);

		if (strcmp(recvbuf, "1") == 0){     //接收到客户端输入为1，请输入登录账号
			memset(sendbuf, 0, 1024);
			strcpy(sendbuf, "请输入登录账号");
			send(fd, sendbuf, strlen(sendbuf), 0);
			memset(recvbuf, 0, sizeof(recvbuf));
			recv(fd, recvbuf, 1024, 0);

			memset(sendbuf, 0, sizeof(sendbuf));
			strcpy(sendbuf, "请输入登录密码");
			send(fd, sendbuf, strlen(sendbuf), 0);
			memset(recvbuf1, 0, sizeof(recvbuf1));
			recv(fd, recvbuf1, 1024, 0);            //recvbuf存入帐号,recvbuf1存入密码，调验证函数验证帐号和密码是否正确 



//********************************************************************************************************************************* */
			if(strcmp(recvbuf,GLY)==0 && strcmp(recvbuf1,MM)==0){
				memset(sendbuf, 0, sizeof(sendbuf));
				strcpy(sendbuf, "管理员登录成功！");
				send(fd, sendbuf, strlen(sendbuf), 0);
				
				//管理员登录后操作
				while(1){
					memset(sendbuf, 0, 1024);
					strcpy(sendbuf, "请选择你要进行的操作：\n1.删除或修改用户信息\n2.查找用户信息\n3.退出\n");
					send(fd, sendbuf, strlen(sendbuf), 0);
					memset(recvbuf, 0, sizeof(recvbuf));
					//1.修改用户信息，账号，密码                             找到其在链表中的值，修改后将链表重新写文件
					if(recv(fd, recvbuf, 1024, 0) == -1){
						printf("recv error:%m\n");
						exit(1);
					}

					if(strcmp(recvbuf, "1") == 0){
						memset(sendbuf, 0, 1024);
						strcpy(sendbuf, "请选择你要进行的操作：\n1.修改用户信息\n2.删除用户信息\n");
						send(fd, sendbuf, strlen(sendbuf), 0);
						recv(fd, recvbuf, 1024, 0);
						
						if(strcmp(recvbuf, "1") == 0){
							//修改用户
							memset(sendbuf, 0, 1024);
							strcpy(sendbuf, "你要修改的用户id是:\n");
							send(fd, sendbuf, strlen(sendbuf), 0);
							recv(fd, recvbuf, 1024, 0);
							if (Verify_account(recvbuf) == 0){
								memset(sendbuf, 0, 1024);
								strcpy(sendbuf, "该id不存在\n");
								send(fd, sendbuf, strlen(sendbuf), 0);
							}else{
								//id存在，在链表中找到该id并将其修改
								User* p;
								p = head;
								while (p->next){
									if (strcmp(p->next->id, recvbuf) == 0){//登录成功后去查找获取其在链表中对应的fd，便于通信
										break;
									}
									p = p->next;
								}
								//此时p是应找到ip的前一个
								memset(sendbuf, 0, 1024);
								strcpy(sendbuf, "你要把其id修改为:\n");
								send(fd, sendbuf, strlen(sendbuf), 0);
								recv(fd, recvbuf, 1024, 0);
								strcpy(p->next->id,recvbuf);

								memset(sendbuf, 0, 1024);
								strcpy(sendbuf, "你要把其密码修改为:\n");
								send(fd, sendbuf, strlen(sendbuf), 0);
								recv(fd, recvbuf, 1024, 0);
								strcpy(p->next->passwd,recvbuf);
								
								memset(sendbuf, 0, 1024);
								strcpy(sendbuf, "你要把其昵称修改为:\n");
								send(fd, sendbuf, strlen(sendbuf), 0);
								recv(fd, recvbuf, 1024, 0);
								strcpy(p->next->name,recvbuf);

								Save_account();
							}

						}else if(strcmp(recvbuf, "2") == 0){
							//删除用户
							memset(sendbuf, 0, 1024);
							strcpy(sendbuf, "你要删除的用户id是:\n");
							send(fd, sendbuf, strlen(sendbuf), 0);
							recv(fd, recvbuf, 1024, 0);
							if(Verify_account(recvbuf) == 0){
								memset(sendbuf, 0, 1024);
								strcpy(sendbuf, "该id不存在\n");
								send(fd, sendbuf, strlen(sendbuf), 0);
							}else{
								//id存在，在链表中找到该id并将其删除
								User* pp;
								pp = head;
								if(strcmp(pp->next->id, recvbuf) == 0){
									//删头
									if(pp->next->next==NULL){//只有一个
										User* temp;
										temp=pp->next;
										free(temp);
										pp->next==NULL;
											
									}else{ 					//删头不只一个
										User* tempp;
										tempp=pp->next;
										pp->next=pp->next->next;
										free(tempp);

									}
								}else{//不是删头
									User* ppp;
									ppp = head;
									while(ppp->next){
									if(strcmp(ppp->next->id, recvbuf) == 0){//登录成功后去查找获取其在链表中对应的fd，便于通信
										break;
									}
									ppp = ppp->next;
									}
									if(ppp->next->next==NULL){//删尾//此时p是应找到ip的前一个
										User* tempp;
										tempp=ppp->next;
										ppp->next=NULL;
										free(tempp);	
									}else{//中间
										User* temppp;
										temppp=ppp->next;
										ppp->next=ppp->next->next;
										free(temppp);
									}
								}			
							}
							Save_account();	
						}

					}else if(strcmp(recvbuf, "2") == 0){
						//2.查找所有用户信息
						User* ppp;
						ppp = head;
						while(ppp->next->next){
							memset(sendbuf, 0, 1024);
							strcpy(sendbuf, "\n用户名:");
							send(fd, sendbuf, strlen(sendbuf), 0);
							memset(sendbuf, 0, 1024);
							strcpy(sendbuf, ppp->next->id);
							send(fd, sendbuf, strlen(sendbuf), 0);

							memset(sendbuf, 0, 1024);
							strcpy(sendbuf, "  密码:");
							send(fd, sendbuf, strlen(sendbuf), 0);
							memset(sendbuf, 0, 1024);
							strcpy(sendbuf, ppp->next->passwd);
							send(fd, sendbuf, strlen(sendbuf), 0);

							memset(sendbuf, 0, 1024);
							strcpy(sendbuf, "  昵称:");
							send(fd, sendbuf, strlen(sendbuf), 0);
							memset(sendbuf, 0, 1024);
							strcpy(sendbuf, ppp->next->name);
							send(fd, sendbuf, strlen(sendbuf), 0);

							ppp=ppp->next;
						}
							memset(sendbuf, 0, 1024);
							strcpy(sendbuf, "\n");
							send(fd, sendbuf, strlen(sendbuf), 0);
						
					}else if(strcmp(recvbuf, "3") == 0){
						//退出,返回到登录界面
						goto p;
					}
					
				}
				

/*************************************************************************************************************************** */

			}else if (Verify_account(recvbuf) == 0 || Authentication_password(recvbuf, recvbuf1) == 0){
				//匹配账号密码是否都存在且正确  第一项为1则在链表中 第二项为1则账号密码匹配
				memset(sendbuf, 0, 1024);
				strcpy(sendbuf, "输入账号或密码不正确");
				send(fd, sendbuf, strlen(sendbuf), 0);
			}else{
				strcpy(recvbuf1, recvbuf);   //把id存入recvbuf1,recvbuf用于接受客户端发来的信息
				memset(sendbuf, 0, 1024);
				strcpy(sendbuf, "登陆成功\n");

				User* p;
				p = head;
				while (p->next){
					if (strcmp(p->next->id, recvbuf1) == 0){//登录成功后去查找获取其在链表中对应的fd，便于通信
						p->next->fd = fd;
						break;
					}
					p = p->next;
				}
				send(fd, sendbuf, strlen(sendbuf), 0);
				//登录之后的操作 
				while (1){
					memset(sendbuf, 0, 1024);
					strcpy(sendbuf, "请选择你要进行的操作：\n1.群聊\n2.私聊\n3.文件传输\n4.退出");
					send(fd, sendbuf, strlen(sendbuf), 0);
					memset(recvbuf, 0, sizeof(recvbuf));
					if (recv(fd, recvbuf, 1024, 0) == -1){
						printf("recv error:%m\n");
						exit(1);
					}

					if (strcmp(recvbuf, "1") == 0){//进入多人聊天室
						Multiplayer_chat(fd, recvbuf1);
					}else if (strcmp(recvbuf, "2") == 0){ //私聊
						Direct_Message(fd, recvbuf1);
					}else if (strcmp(recvbuf, "4") == 0){//退出,返回到登录界面
						goto p;
					}else if (strcmp(recvbuf, "3") == 0){
                        Send_File(fd, recvbuf1);
						sleep(1);
                    }
				}
 
			}
		}
		//主菜单选择
		else if (strcmp(recvbuf, "2") == 0){
			Sign_in(fd); //注册
			goto  p;
		}else if (strcmp(recvbuf, "send") == 0){
			Recv_File(fd);
			goto  p;
		}else if (strcmp(recvbuf, "end") == 0){//退出
			memset(sendbuf, 0, 1024);
			strcpy(sendbuf, "欢迎使用，再见");
			send(fd, sendbuf, strlen(sendbuf), 0);
			break;
		}else{
			continue;             //如果选择不存在的功能键，直接continue继续
		}
	}
	close(fd);
}


int main()
{
	int newfd = 0;    //定义并初始化
	int len = sizeof(struct sockaddr_in);
	struct sockaddr_in myaddr;  	
	struct sockaddr_in otheraddr;
	memset(&myaddr, 0, len);
	memset(&otheraddr, 0, len);
	//实现端口复用，必须在绑定之前，而且只要绑定到同一个端口的所有套接字都得设置复用
	int reuse = 1;
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0){
		printf("setsockopet error%m\n");
		return -1;
	}
	//协议地址族
	myaddr.sin_family = AF_INET;
	myaddr.sin_port = htons(PORT);
	myaddr.sin_addr.s_addr = inet_addr(IP);
	//绑定
	if ((bind(sockfd, (struct sockaddr*)&myaddr, len)) == -1){
		printf("bind error%m\n");
		return -1;
	}else{
		printf("bind success\n");
	}
	//监听
	if ((listen(sockfd, 10)) == -1){
		printf("listen error%m\n");
		return -1;
	}else{
		printf("listen success\n");
	}

	pthread_t id1;
	head = create();         //创建账号信息的头结点 并初始化链表
	while (1){
		if ((newfd = accept(sockfd, (struct sockaddr*)&otheraddr, &len)) == -1){    //连接客户端 
			printf("accept error%m\n");
			return -1;
		}else{		//连接成功输出客户端等信息
			printf("accept success\n");   
			printf("client ip=%s\n", inet_ntoa(otheraddr.sin_addr)); //打印出客户端的ip地址
		}
		//线程客户端
		pthread_create(&id1, NULL, handleclient, &newfd);
	}
	close(sockfd);
	return 0;
}
 
 
