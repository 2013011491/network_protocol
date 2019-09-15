#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include <WinSock2.h>
#include <process.h>

#define PUB 0
#define SUB 1
#define MAX_STR_LEN 100
#define NO_PERI 0
#define PERIODIC 1
#define MAX_PUB_NUM 10
#define MAX_SUB_NUM 10
#define MAX_ADMIN 10

#pragma warning(disable:4996) //for scanf
#pragma comment(lib, "ws2_32.lib")

typedef struct Message
{
    char filename[MAX_STR_LEN];
	int filesize;
	char value;
}Message;

typedef struct Topic
{
    int type;  //Pub or Sub
    char name[MAX_STR_LEN];
    int period; //PERIOD or NO_PERI
    struct sockaddr_in address;
    Message body;
}Topic;

typedef struct BrokerTopic
{
    Topic base;
    SOCKET sock_address;
}BTopic;

Topic uPubStorage[MAX_PUB_NUM];
Topic uSubStorage[MAX_SUB_NUM];
BTopic pubStorage[MAX_PUB_NUM];
BTopic subStorage[MAX_SUB_NUM];
int pcount=0;
int scount=0;
SOCKET client[MAX_ADMIN];
SOCKET sock_pub,sock_sub;
HANDLE mutex;

void recvThread(void *data);
void sendThread();
void PubServer();
void recvThreadUser();

int main()
{
    int mode;
    WSADATA wsaData;
    struct sockaddr_in addr,addrMiddle;
    

    //start socket
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		printf("WSA Start Failed\n");
		return 0;
	}

    //body
    printf("0.Broker, 1. User: ");
	scanf("%d", &mode);
	getchar();

    if(mode==0) //Broker mode
    {
        SOCKET sock_broker;
        int adminCount=0;

		addr.sin_family = AF_INET;
		addr.sin_port = htons(36007);
		addr.sin_addr.S_un.S_addr = INADDR_ANY;

        _beginthread(sendThread, 0, NULL);

		//  IPv4 TCP
		if ((sock_broker = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
			printf("Socket Open Failed\n");
			exit(1);
		}

		// binding
		if (bind(sock_broker, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
			printf("Bind Failed\n");
			printf("%d\n", WSAGetLastError());
			exit(2);
		}

		// listen
		if (listen(sock_broker, 5) == -1) {
			printf("Listen Failed\n");
			exit(3);
		}

		while(1)
        {
			// accept and create client thread
			if ((client[adminCount] = accept(sock_broker, NULL, NULL))==-1)
            {
				printf("Accept Failed\n");
				continue;
			}
			else
            {
                if(adminCount<MAX_ADMIN)
                {
					int idx = adminCount;
					_beginthread(recvThread, 0, &idx);
					adminCount++;
				}
	        }
        }
    }else if(mode==1)//User
    {
        int check;
        int i=1;
        char respon[30];
        Message exam={"example", 10, 'a'};

    //for pub server
		addr.sin_family = AF_INET;
		addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    
		//  IPv4 TCP
		if ((sock_pub = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
			printf("Socket Open Failed\n");
			exit(1);
		}

		// binding
        for(i=1; i<MAX_ADMIN; i++)
        {
            int temp=36007+i;
            addr.sin_port = htons(temp);
            if(bind(sock_pub, (struct sockaddr*)&addr, sizeof(addr))!=-1) break;
        }
        if(i==MAX_ADMIN)
        {
            printf("Bind Failed\n");
			printf("%d\n", WSAGetLastError());
			exit(2);
        }

		// listen
		if (listen(sock_pub, 5) == -1)
        {
			printf("Listen Failed\n");
			exit(3);
		}

        _beginthread(PubServer, 0, NULL);

    //for sub client
        addrMiddle.sin_family = AF_INET;
		addrMiddle.sin_port = htons(36007);
		addrMiddle.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");

        if ((sock_sub = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
			printf("Socket Sub Open Failed\n");
			exit(1);
		}

		// Connect
		if (connect(sock_sub, (struct sockaddr*)&addrMiddle, sizeof(addrMiddle)) == -1) {
			printf("Connect Failed\n");
			exit(4);
		}

        _beginthread(recvThreadUser,0,NULL);
        
        while(1)
        {
            
            printf("0.Topic Create : ");
            scanf("%d",&check);
            if(check==0)//topic create
            {
                printf("Topic type(0:Pub, 1:Sub): ");
                scanf("%d",&i);
                if(i==0) //for pub
                {
                    if(pcount<MAX_PUB_NUM)
                    {
                        uPubStorage[pcount].type=PUB;
                        printf("Topic name: ");
                        scanf("%s",uPubStorage[pcount].name);
                        printf("Data period: ");
                        scanf("%d",&uPubStorage[pcount].period);
                        printf("Contents: Assuming already created\n");
                        uPubStorage[pcount].address=addr;
                        uPubStorage[pcount].body=exam;
                        pcount++;
                        send(sock_sub, (char*)&uPubStorage[pcount-1], sizeof(Topic), 0);

                    }else printf("Pub Storage is pull\n");

                }else //for sub
                {
                    if(scount<MAX_SUB_NUM)
                    {
                        uSubStorage[scount].type=SUB;
                        printf("Topic name: ");
                        scanf("%s",uSubStorage[scount].name);
                        scount++;
                        send(sock_sub, (char*) &uSubStorage[scount-1], sizeof(Topic), 0);
                    }else printf("Sub Storage is pull\n");                    
                }
				_sleep(1000);

            }else //check msg
            {
                                
            }
        }
    }else
    {
        printf("Exit");
    }

	WSACleanup();
    return 0;
}

void recvThread(void *data)
{
    int thread_id = *((int*)data);
    Topic msg;
    while (recv(client[thread_id], (char*)&msg, sizeof(msg), 0) > 0)
                {
                    WaitForSingleObject(mutex, INFINITE);
                    if(msg.type==PUB)
                    {
                        if(pcount!=MAX_PUB_NUM)
                        {
                            strcpy(pubStorage[pcount].base.name,msg.name);
                            pubStorage[pcount].base.period=msg.period;
                            pubStorage[pcount].base.address=msg.address;
                            pubStorage[pcount].sock_address=client[thread_id];
                            pcount++;
                            printf("Store Pub\n");
                            printf("name: %s\n",pubStorage[pcount-1].base.name);
                            printf("period: %d\n",pubStorage[pcount-1].base.period);
                            printf("\n");
                            send(client[thread_id], (char*)"OK", sizeof("OK"), 0);
                        }else
                        {
                            send(client[thread_id], (char*)"Server Pull", sizeof("Server Pull"), 0);
                        }
                    }else
                    {
                        if(scount!=MAX_SUB_NUM)
                        {
                            strcpy(subStorage[scount].base.name,msg.name);
                            subStorage[scount].sock_address=client[thread_id];
                            scount++;
                            printf("Store Sub\n");
                            printf("name: %s\n",subStorage[scount-1].base.name);
                            printf("\n");
                            send(client[thread_id], (char*)"OK", sizeof("OK"), 0);
                        }else
                        {
                            send(client[thread_id], (char*)"Server Pull", sizeof("Server Pull"), 0);
                        }
                    }
                    ReleaseMutex(mutex);
		        }
}

void sendThread()
{
	Topic *tmp;
    int oldPcount=0;
    int oldScount=0;
    int prePcount;
    int preScount;
	
	printf("Send Thread Start\n");

	while(1)
    {
        _sleep(1000);
        WaitForSingleObject(mutex, INFINITE);
        prePcount=pcount;
        preScount=scount;
        ReleaseMutex(mutex);

        if(prePcount>oldPcount)
        {   
            for(int i=0; i<preScount; i++)
            {
                if(!strcmp(subStorage[i].base.name,pubStorage[prePcount-1].base.name))
                {
                    send(subStorage[i].sock_address, (char*)&pubStorage[prePcount-1], sizeof(BTopic), 0);
                }
            }
            oldPcount=prePcount;
        }
        if(preScount>oldScount)
        {
            
            for(int i=0; i<prePcount; i++)
            {
                if(!strcmp(subStorage[preScount-1].base.name,pubStorage[i].base.name))
                {
                    send(subStorage[preScount-1].sock_address, (char*)&pubStorage[i], sizeof(BTopic), 0);
                }
            }
            oldScount=preScount;
        }
		
	}
}



void PubServer()
{
    SOCKET sock_rec;
    BTopic reqMsg;
    
    while(1)
    {    
        if ((sock_rec = accept(sock_pub, NULL, NULL))==-1)
        {
			printf("Accept Failed\n");
			continue;
		}else 
        {
            recv(sock_rec, (char*)&reqMsg, sizeof(BTopic), 0);
            send(sock_rec, (char*)"OK", sizeof("OK"), 0);
            for(int i=0; i<=pcount; i++)
            {
                if(!strcmp(uPubStorage[i].name,reqMsg.base.name))
                {
                    printf("%s sending ok\n",uPubStorage[i].name);
                    send(sock_rec, (char*)&uPubStorage[i], sizeof(Topic), 0);
                    break;
                }
            }	
		}
    }
}

void recvThreadUser()
{
    BTopic match;
    char respon[30];

    while(1)
    {
        if(recv(sock_sub, (char*)&match, sizeof(BTopic),0)>0)
        {
            if(!strcmp((char*)&match,"OK"))
            {
                printf("From Middle : OK\n");
                continue;
            }

            SOCKET sock_data;

            printf("From middle: matched pub found ");
            printf("name: %s\n", &match.base.name);


            if ((sock_data = socket(AF_INET, SOCK_STREAM, 0))==-1)
            {
                printf("Socket data Open Failed\n");
                exit(1);
            }

            // Connect
            if (connect(sock_data, (struct sockaddr*) &match.base.address, sizeof(match.base.address)) == -1)
            {
                printf("Connect Failed111111111\n");
                exit(4);
            }

            send(sock_data,(char*)&match,sizeof(BTopic),0);
            printf("request to sender\n");
            if(recv(sock_data, (char*)&respon, sizeof(respon), 0) > 0)
            {printf("From Sender : %s\n", respon);}
            for(int i=0; i<=scount; i++)
            {
                if(!strcmp(uSubStorage[i].name,match.base.name))
                {
                    memset(&uSubStorage[i],0,sizeof(Topic));
                    recv(sock_data, (char*)&uSubStorage[i], sizeof(Topic), 0);
                    printf("New matched msg\n");
                    printf("%s\n",(char*)&uSubStorage[i]);
                    printf("name: %s\n period: %d\n msg filename: %s\n msg size: %d\n msg value: %c\n",uSubStorage[i].name,uSubStorage[i].period,uSubStorage[i].body.filename,uSubStorage[i].body.filesize,uSubStorage[i].body.value);
                    break;
                }
            }
            closesocket(sock_data);
        }
    }
}