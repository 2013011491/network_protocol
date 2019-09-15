#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <WinSock2.h>
#include <process.h>
#include <direct.h>

#pragma warning(disable:4996)
#pragma comment(lib, "ws2_32.lib")

SOCKET listen_sock, clnt_sock[10], data_sock[10];
int control_port = 36007;
int data_port;
char *username = "np2019";
char *password = "np2019";

void recvThread(void *arg);
int LastIndexof(char *str, char c);
void closedatasocket(int idx);

enum {
	NOTLOGGEDIN, 
	NEEDPASSWORD, 
	LOGGEDIN
};

int main() {
	int count = 0;
	WSADATA wsaData;
	HANDLE th_recv[10];
	data_port = control_port;

	struct sockaddr_in addr;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		printf("WSA Start Failed\n");
		return 0;
	}

	// 林家 汲沥 36007
	addr.sin_family = AF_INET;
	addr.sin_port = htons(control_port);
	addr.sin_addr.S_un.S_addr = INADDR_ANY;

	// 家南 积己 IPv4 TCP
	if ((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		printf("Socket Open Failed\n");
		exit(1);
	}

	// 林家 官牢爹
	if (bind(listen_sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		printf("Bind Failed\n");
		exit(2);
	}

	// listen
	if (listen(listen_sock, 5) == -1) {
		printf("Listen Failed\n");
		exit(3);
	}


	while (1) {
		// accept and create client thread
		if ((clnt_sock[count] = accept(listen_sock, NULL, NULL)) == -1) {
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

	closesocket(listen_sock);
	WSACleanup();
}

void recvThread(void *arg) {
	char buff[1024];
	char msg[1024];
	int connected = 1;
	int len;
	int userstatus = NOTLOGGEDIN;
	int transfermode = 0; // 0 ascii , 1 binary

	int idx = *((int*)arg);
	
	char rootDir[1024];
	char curDir[1024];
	_getcwd(rootDir, 1024);
	strcpy(curDir, rootDir);
	
	printf("User #%d connected\n", idx);

	// send 220 message
	memset(msg, 0, sizeof(msg));
	sprintf(msg, "220 Server ready\r\n");
	len = send(clnt_sock[idx], msg, strlen(msg), 0);

	// receive command
	while (connected) {
		memset(buff, 0, sizeof(buff));
		if (recv(clnt_sock[idx], buff, sizeof(buff), 0) > 0) {

			// USER Command
			if (strncmp(buff, "USER", 4) == 0) {
				if (strncmp(username, &buff[5], strlen(&buff[5]) - 2) == 0) {
					userstatus = NEEDPASSWORD;
	
					memset(msg, 0, sizeof(msg));
					sprintf(msg, "331 Password required for username\r\n");
					
				}
				else {
					memset(msg, 0, sizeof(msg));
					sprintf(msg, "530 Login Failed\r\n");
				}
				len = send(clnt_sock[idx], msg, strlen(msg), 0);
				printf("User %d: %s\n", idx, msg);
			}
			// PASS Command
			else if (strncmp(buff, "PASS", 4) == 0) {
				if ((strncmp(password, &buff[5], strlen(&buff[5]) - 2) == 0) && (userstatus == NEEDPASSWORD)) {
					userstatus = LOGGEDIN;

					memset(msg, 0, sizeof(msg));
					sprintf(msg, "230 User Logged in\r\n");
				}
				else {
					memset(msg, 0, sizeof(msg));
					sprintf(msg, "530 Login Failed\r\n");
				}
				len = send(clnt_sock[idx], msg, strlen(msg), 0);
				printf("User %d: %s\n", idx, msg);
			}
			// CWD Command
			else if (strncmp(buff, "CWD", 3) == 0) {
				char tmpDir[1024];
				char tmpDir2[1024];
				int tmpidx = -1;

				memset(tmpDir, 0, sizeof(tmpDir));
				strcpy(tmpDir, curDir);

				if ((strncmp(&buff[4], "..", strlen(&buff[4]) - 2) == 0)) {
					tmpidx = LastIndexof(tmpDir, '\\');

					if (tmpidx > 0) {
						strcpy(tmpDir2, tmpDir);

						memset(tmpDir, 0, sizeof(tmpDir));
						strncpy(tmpDir, tmpDir2, tmpidx + 1);
					}
				}
				else if ((strncmp(&buff[4], ".", strlen(&buff[4]) - 2) != 0) && (strlen(&buff[4]) - 2 > 0)) {
					strcat(tmpDir, "\\\\");
					strncat(tmpDir, &buff[4], strlen(&buff[4]) - 2);
				}

				memset(curDir, 0, sizeof(curDir));
				strcpy(curDir, tmpDir);

				memset(msg, 0, sizeof(msg));
				sprintf(msg, "250 CWD command succesful\r\n");
				len = send(clnt_sock[idx], msg, strlen(msg), 0);
				printf("User %d: %s\n", idx, msg);
			}
			// NLST Command
			else if (strncmp(buff, "NLST", 4) == 0) {
				WIN32_FIND_DATA find_data;
				HANDLE handle;
				char tmpDir[1024];
				strcpy(tmpDir, curDir);
				strcat(tmpDir, "\\\\*");

				handle = FindFirstFile(tmpDir, &find_data);

				if (handle == INVALID_HANDLE_VALUE)
					return 0;

				do {
					memset(msg, 0, sizeof(msg));
					sprintf(msg, "%s\r\n", find_data.cFileName);
					len = send(data_sock[idx], msg, strlen(msg), 0);
				} while (FindNextFile(handle, &find_data));

				FindClose(handle);
				memset(msg, 0, sizeof(msg));
				sprintf(msg, "226 Successfully transferred\r\n");
				len = send(clnt_sock[idx], msg, strlen(msg), 0);
				printf("User %d: %s\n", idx, msg);

				closedatasocket(idx);
			}
			// QUIT Command
			else if (strncmp(buff, "QUIT", 4) == 0) {
				memset(msg, 0, sizeof(msg));
				sprintf(msg, "221 Goodbye\r\n");
				len = send(clnt_sock[idx], msg, strlen(msg), 0);
				printf("User %d: %s\n", idx, msg);
				connected = 0;
			}
			// PASV Command
			else if (strncmp(buff, "PASV", 4) == 0) {

				struct sockaddr_in addr_data;

				data_port = data_port + 1;

				// 林家 汲沥 36007
				addr_data.sin_family = AF_INET;
				addr_data.sin_port = htons(data_port);
				addr_data.sin_addr.S_un.S_addr = INADDR_ANY;

				// 家南 积己 IPv4 TCP
				if ((data_sock[idx] = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
					printf("Socket Open Failed\n");
					exit(1);
				}

				// 林家 官牢爹
				if (bind(data_sock[idx], (struct sockaddr*)&addr_data, sizeof(addr_data)) == -1) {
					printf("Bind Failed\n");
					printf("%d\n", WSAGetLastError());
					exit(2);
				}

				// listen
				if (listen(data_sock[idx], 5) == -1) {
					printf("Listen Failed\n");
					exit(3); 
				}

				memset(msg, 0, sizeof(msg));
				sprintf(msg, "227 Entering Passive Mode (%s,%s,%s,%s,%d,%d)\r\n", "127", "0", "0", "1", data_port/256, data_port%256);
				len = send(clnt_sock[idx], msg, strlen(msg), 0);
				printf("User %d: %s\n", idx, msg);

				if ((data_sock[idx] = accept(data_sock[idx], NULL, NULL)) == -1) {
					printf("Accept Failed\n");
					printf("%d\n", WSAGetLastError());
					exit(4);
				}
				printf("PASV mode - data connection established\n");
			}
			else if (strncmp(buff, "RETR", 4) == 0) {
				char tmpfile[1024];

				memset(tmpfile, 0, sizeof(tmpfile));
				strcpy(tmpfile, curDir);

				strcat(tmpfile, "\\\\");
				strncat(tmpfile, &buff[5], strlen(&buff[5]) - 2);

				memset(msg, 0, sizeof(msg));
				strcpy(msg, "150 Opening data channel\r\n");
				len = send(clnt_sock[idx], msg, strlen(msg), 0);

				

				if (transfermode == 1) {
					int count;
					char buff[1024];

					FILE *fp = fopen(tmpfile, "rb");

					memset(buff, 0, sizeof(buff));

					while ((count = fread(buff, sizeof(char), 1024, fp)) > 0) {
						len = send(data_sock[idx], buff, count, 0);
						memset(buff, 0, sizeof(buff));
					}

					fclose(fp);
				}
				else {
					int count;
					char buff[1024];

					FILE *fp = fopen(tmpfile, "rt");

					memset(buff, 0, sizeof(buff));

					while (fgets(buff, 1024, fp) != NULL) {
						len = send(data_sock[idx], buff, strlen(buff), 0);
						memset(buff, 0, sizeof(buff));
					}

					fclose(fp);
				}

				memset(msg, 0, sizeof(msg));
				sprintf(msg, "226 Successfully transferred\r\n");
				len = send(clnt_sock[idx], msg, strlen(msg), 0);
				printf("User %d: %s\n", idx, msg);

				closedatasocket(idx);
			}
			else if (strncmp(buff, "TYPE", 4) == 0) {
				if (strncmp("A", &buff[5], strlen(&buff[5]) - 2) == 0) {
					
					transfermode = 0;
					memset(msg, 0, sizeof(msg));
					sprintf(msg, "200 Type set to ascii\r\n");
				}
				else if (strncmp("I", &buff[5], strlen(&buff[5]) - 2) == 0) {

					transfermode = 1;
					memset(msg, 0, sizeof(msg));
					sprintf(msg, "200 Type set to binary\r\n");
				}
				len = send(clnt_sock[idx], msg, strlen(msg), 0);
				printf("User %d: %s\n", idx, msg);
			}
			else {
				// send 220 message
				memset(msg, 0, sizeof(msg));
				strcpy(msg, "501 Server cannot accept argument\r\n");
				len = send(clnt_sock[idx], msg, strlen(msg), 0);
			}
		}
	}
	closesocket(clnt_sock[idx]);
}

int LastIndexof(char *str, char c)
{
	int i, idx = -1;
	for (i = 0; i < strlen(str); i++)
	{
		if (str[i] == c)
			idx = i;
	}

	return idx;
}
void closedatasocket(int idx) {
	closesocket(data_sock[idx]);
	data_sock[idx] = NULL;
}
