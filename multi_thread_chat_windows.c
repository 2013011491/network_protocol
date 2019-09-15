// Windows
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <WinSock2.h>
#include <process.h>

#define MAX_BUFF 100
#define MAX_MESSAGE_LEN 256

#pragma warning(disable:4996)
#pragma comment(lib, "ws2_32.lib")

typedef struct Message {
	int user_id;
	char str[MAX_MESSAGE_LEN];
}Message;

void sendThread();
void recvThread(void *data);
void sendThreadClient();
int isFull();
int isEmpty();
int enqueue(Message item);
Message* dequeue();

SOCKET sock_main, sock_client[10];
Message *msgbuff;
HANDLE mutex;

int front = -1;
int rear = -1;

int main()
{
	int mode;
	WSADATA wsaData;
	int count = 0;
	Message buff;
	HANDLE th_send;

	struct sockaddr_in addr;
	
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		printf("WSA Start Failed\n");
		return 0;
	}

	printf("1. Server, 2. Client: ");
	scanf("%d", &mode);
	getchar();
	
	if (mode == 1) {
		HANDLE th_recv[10];

		msgbuff = (Message *)malloc(sizeof(Message) * MAX_BUFF);

		// Create Send Thread
		th_send = (HANDLE)_beginthread(sendThread, 0, NULL);

		// �ּ� ���� 36007
		addr.sin_family = AF_INET;
		addr.sin_port = htons(36007);
		addr.sin_addr.S_un.S_addr = INADDR_ANY;

		// ���� ���� IPv4 TCP
		if ((sock_main = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
			printf("Socket Open Failed\n");
			exit(1);
		}

		// �ּ� ���ε�
		if (bind(sock_main, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
			printf("Bind Failed\n");
			printf("%d\n", WSAGetLastError());
			exit(2);
		}

		// listen
		if (listen(sock_main, 5) == -1) {
			printf("Listen Failed\n");
			exit(3);
		}

		while (1) {
			// accept and create client thread
			if ((sock_client[count] = accept(sock_main, NULL, NULL)) == -1) {
				printf("Accept Failed\n");
				continue;
			}
			else {
				if (count < 10) {
					int idx = count;
					th_recv[count] = (HANDLE)_beginthread(recvThread, 0, &idx);
					count++;
				}
			}
		}
	}
	else {
		// �ּ� ���� 127.0.0.1 36007
		addr.sin_family = AF_INET;
		addr.sin_port = htons(36007);
		addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");

		// ���� ���� IPv4 TCP
		if ((sock_main = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
			printf("Socket Open Failed\n");
			exit(1);
		}

		// Connect
		if (connect(sock_main, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
			printf("Connect Failed\n");
			exit(4);
		}

		// Client Send Thread
		th_send = (HANDLE)_beginthread(sendThreadClient, 0, NULL);

		while (1) {

			// �޽��� ���� �� ���?
			memset(&buff, 0, sizeof(buff));

			if (recv(sock_main, (char*)&buff, sizeof(buff), 0) > 0) {
				printf("User %d: %s\n", buff.user_id, buff.str);
			}
			else {
				printf("Disconnected\n");
				closesocket(sock_main);
				exit(5);
			}
		}
	}

	WSACleanup();
	return 0;
}


void sendThread() {
	Message *tmp;
	
	printf("Send Thread Start\n");

	while (1) {
		// ť���� ������ �޽����� ������ dequeue �� ������ �������� ����
		if ((tmp = dequeue()) != NULL) {
			for (int i = 0; i < 10; i++) {
				if (i != tmp->user_id) {
					send(sock_client[i], (char*)tmp, sizeof(Message), 0);
				}
			}
		}
		_sleep(1);
	}
}

void recvThread(void *data) {
	Message buff;
	int thread_id = *((int*)data);

	printf("Receive Thread %d Start\n", thread_id);
	

	// ������ �̽����� ť�� enqueue
	memset(&buff, 0, sizeof(Message));

	while (recv(sock_client[thread_id], (char*)&buff, sizeof(buff), 0) > 0) {
		buff.user_id = thread_id;
		if (enqueue(buff) == -1) {
			printf("Message Buffer Full\n");
		}
	}
}

void sendThreadClient() {
	Message tmp;
	int count;

	while (1) {
		// �޽����� �Է� ���� �� ����
		memset(&tmp, 0, sizeof(tmp));
		fgets(tmp.str, MAX_MESSAGE_LEN, stdin);
		tmp.str[strlen(tmp.str) - 1] = '\0';
		tmp.user_id = -1;

		count = send(sock_main, (char *)&tmp, sizeof(Message), 0);
	}
}

int isFull() {
	if ((front == rear + 1) || (front == 0 && rear == MAX_BUFF - 1)) {
		return 1;
	}
	return 0;
}

int isEmpty() {
	if (front == -1) {
		return 1;
	}
	return 0;
}

int enqueue(Message item) {

	
	if (isFull()) {
		return -1;
	}
	else {
		WaitForSingleObject(mutex, INFINITE);
		if (front == -1) {
			front = 0;
		}
		rear = (rear + 1) % MAX_BUFF;
		msgbuff[rear].user_id = item.user_id;
		strcpy(msgbuff[rear].str, item.str);
		ReleaseMutex(mutex);
	}
	return 0;
}

Message* dequeue() {
	Message *item;

	if (isEmpty()) {
		return NULL;
	}
	else {
		WaitForSingleObject(mutex, INFINITE);
		item = &msgbuff[front];

		if (front == rear) {
			front = -1;
			rear = -1;
		}
		else {
			front = (front + 1) % MAX_BUFF;
		}	
		ReleaseMutex(mutex);
		return item;
	}
	
}
