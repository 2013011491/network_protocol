// Windows
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <WinSock2.h>
#include <process.h>

#pragma warning(disable:4996)
#pragma comment(lib, "ws2_32.lib")

void ConnectToServer();
void recvDataThread();

int port, mode;
WSADATA wsaData;
char buff[1024];
char ip_address[16];
char local_ip_address[16];
char username[31];
char password[31];
char command[100];

FILE *fp;

int FileDownMode = 0;

SOCKET sock_main;

struct sockaddr_in addr, addr_data;

int main() {
	char tmp[40] = { 0 };

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		printf("WSA Start Failed\n");
		return 0;
	}

	char hostname[100];
	IN_ADDR myaddr = { 0, };
	gethostname(hostname, sizeof(hostname));
	
	HOSTENT *ptr = gethostbyname(hostname);
	
	while (ptr && ptr->h_name) {
		if (ptr->h_addrtype == AF_INET) {
			memcpy(&myaddr, ptr->h_addr_list[0], ptr->h_length);
			break;
		}
		ptr++;
	}
	strcpy(local_ip_address, inet_ntoa(myaddr));

	for (int i = 0; i < 16; i++) {
		if (local_ip_address[i] == '.') local_ip_address[i] = ',';
	}

	printf("1. Server, 2. Client: ");
	scanf("%d", &mode);
	getchar();

	if (mode == 1) {

	}
	else if (mode == 2) {
		printf("立加 林家: ");
		scanf("%s", ip_address);
		printf("器飘: ");
		scanf("%d", &port);
		printf("User name: ");
		scanf("%s", username);
		printf("password: ");
		scanf("%s", password);
		getchar();

		ConnectToServer();

		while (1) {
			printf("ftp> ");
			memset(command, 0, sizeof(command));
			fgets(command, 1000, stdin);
			command[strlen(command) - 1] = '\0';
			memset(tmp, 0, sizeof(tmp));

			if (strcmp(command, "quit") == 0) {
				sprintf(tmp, "QUIT \r\n");
			}
			else if (strcmp(command, "ls") == 0) {
				strcpy(tmp, "NLST\r\n");
			}
			else if (strncmp(command, "cd ", 3) == 0) {
				char *tmp2 = strtok(command, " ");
				tmp2 = strtok(NULL, " ");
				sprintf(tmp, "CWD %s\r\n", tmp2);
			}
			else if (strncmp(command, "get ", 4) == 0) {
				char *tmp2 = strtok(command, " ");
				tmp2 = strtok(NULL, " ");
				FileDownMode = 1;
				fp = fopen(tmp2, "wb");
				sprintf(tmp, "RETR %s\r\n", tmp2);
			}
			else if (strcmp(command, "bin") == 0) {
				strcpy(tmp, "TYPE I \r\n");
			}
			else if (strcmp(command, "ascii") == 0) {
				strcpy(tmp, "TYPE A \r\n");
			}
			else if (strcmp(command, "feature") == 0) {
				strcpy(tmp, "FEAT \r\n");
			}
			else {
				printf("Commands: ls, cd, get..\n");
				continue;
			}

			send(sock_main, tmp, strlen(tmp), 0);

			memset(buff, 0, sizeof(buff));

			while (recv(sock_main, (char*)buff, sizeof(buff), 0) > 0) {
				printf("%s", buff);

				if ((strncmp(buff, "226", 3) == 0) || (strncmp(buff, "250", 3) == 0) || (strncmp(buff, "425", 3) == 0) || (strncmp(buff, "211", 3) == 0)) {
					port++;
					_beginthread(recvDataThread, 0, 0);

					char tmp3[40] = { 0 };

					sprintf(tmp3, "PORT %s,%d,%d\r\n", local_ip_address, (unsigned char)(port >> 8), (unsigned char)(port & 0xFF));
					printf("%s", tmp3);
					send(sock_main, tmp3, strlen(tmp3), 0);
				}
				else if (strncmp(buff, "200", 3) == 0) {
					break;
				}
				else if (strncmp(buff, "221", 3) == 0) {
					closesocket(sock_main);
					WSACleanup();
					exit(10);
				}
				memset(buff, 0, sizeof(buff));
			}
		}
	}
	else {
		printf("绢捞\n");
	}

	return 0;
}

void ConnectToServer() {
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.S_un.S_addr = inet_addr(ip_address);

	if ((sock_main = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		printf("Socket Open Failed\n");
		exit(1);
	}

	if (connect(sock_main, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		printf("Connect Failed\n");
		exit(4);
	}

	while (1) {
		memset(buff, 0, sizeof(buff));

		if (recv(sock_main, (char*)buff, sizeof(buff), 0) > 0) {
			printf("%s", buff);

			if (strncmp(buff, "220", 3) == 0) {
				char tmp[40] = { 0 };
				sprintf(tmp, "USER %s\r\n", username);
				send(sock_main, tmp, strlen(tmp), 0);
			}
			else if (strncmp(buff, "331", 3) == 0) {
				char tmp[40] = { 0 };
				sprintf(tmp, "PASS %s\r\n", password);
				send(sock_main, tmp, strlen(tmp), 0);
			}
			else if (strncmp(buff, "230", 3) == 0) {
				port++;
				_beginthread(recvDataThread, 0, 0);

				char tmp[40] = { 0 };

				sprintf(tmp, "PORT %s,%d,%d\r\n", local_ip_address, (unsigned char)(port >> 8), (unsigned char)(port & 0xFF));
				printf("%s", tmp);
				send(sock_main, tmp, strlen(tmp), 0);
			}
			else if (strncmp(buff, "200", 3) == 0) {
				return;
			}
		}
		else {
			printf("Disconnected\n");
			closesocket(sock_main);
			WSACleanup();
			exit(5);
		}
	}
}

void recvDataThread() {
	SOCKET sock_data;	
	addr_data.sin_family = AF_INET;
	addr_data.sin_port = htons(port);
	addr_data.sin_addr.S_un.S_addr = INADDR_ANY;

	if ((sock_data = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		printf("Socket Open Failed\n");
		exit(1);
	}
	
	if (bind(sock_data, (struct sockaddr*)&addr_data, sizeof(addr_data)) == -1) {
		printf("Bind Failed\n");
		printf("%d\n", WSAGetLastError());
		exit(2);
	}

	if (listen(sock_data, 5) == -1) {
		printf("Listen Failed\n");
		exit(3);
	}

	sock_data = accept(sock_data, NULL, 0);

	int flags = 1;
	setsockopt(sock_data, SOL_SOCKET, SO_KEEPALIVE, (void*)&flags, sizeof(flags));

	char tmpbuff[1460];

	memset(tmpbuff, 0, sizeof(tmpbuff));

	int count = 0;
	int count2 = 0;

	while ((count = recv(sock_data, (char*)tmpbuff, sizeof(tmpbuff), 0)) > 0) {
		count2 += count;
		if (FileDownMode == 1) {
			fwrite(tmpbuff, sizeof(char), count, fp);
		}
		else {
			printf("%s", tmpbuff);
		}
		memset(tmpbuff, 0, sizeof(tmpbuff));
	}

	if (FileDownMode == 1) {
		FileDownMode = 0;
		fclose(fp);
	}
	closesocket(sock_data);
	printf("Count2: %d\n", count2);
}