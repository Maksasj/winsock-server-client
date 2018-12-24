// client using winsock2 

#define _WIN32_WINNT 0x501
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <string>
#include <iostream>

#define DEFAULT_PORT 2000
#define DEFAULT_BUFLEN 512

// for inet_pton - https://stackoverflow.com/questions/15660203/inet-pton-identifier-not-found/15660299
// https://stackoverflow.com/questions/1561469/is-there-an-alternative-to-inet-ntop-inetntop-in-windows-xp

// need 1 extra thread for sending messages. 
// https://stackoverflow.com/questions/27610316/winsock-single-client-server-send-and-receive-simultaneously
HANDLE receiveThread;

// to be executed in new thread 
DWORD WINAPI receiveMessagesProc(LPVOID lpParam){
	
	// pass in the socket for this client to receive on 
	// dereference lpParam as a casted SOCKET pointer 
	SOCKET clientSock = *(SOCKET *)lpParam;
	char recvbuf[DEFAULT_BUFLEN] = {0};
	int recvbuflen = DEFAULT_BUFLEN;
	int rtnVal;
	
	while(1){
		// clear the recv buffer 
		ZeroMemory(recvbuf, recvbuflen);
		rtnVal = recv(clientSock, recvbuf, recvbuflen, 0);
		if(rtnVal > 0){
			//printf("bytes received: %d\n", iResult);
			printf("%s\n", recvbuf);
		}else if(rtnVal == 0){
			printf("connection closed.\n");
			return 1;
		}else{
			printf("recv failed: %d\n", WSAGetLastError());
			return 1;
		}
	}
	
	return 0;
}

// have a protocol:
// when sending messages, append the user's name to the front of the message

int main(int argc, char** argv){
	
	// validate args!
	if(argc != 3){
		return 1;
	}
	
	// argv[1] = ip address to connect with 
	// argv[2] = user name 
	
	WSADATA wsaData;
	
	int iResult;
	
	//std::string test = "this is a test";
	//const char *sendbuf = test.c_str();
	char* username = argv[2];
	
	// intialize winsock 
	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if(iResult != 0){
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}
	
	
	// start socket stuff
	SOCKET connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(connectSocket == INVALID_SOCKET){
		printf("socket() failed\n");
		WSACleanup();
		return 1;
	}
	
	struct sockaddr_in servAddr;
	ZeroMemory(&servAddr, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	
	int size = sizeof(servAddr);
	iResult = WSAStringToAddressA(argv[1], AF_INET, NULL, (struct sockaddr *)&servAddr, &size);
	if(iResult != 0){
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	servAddr.sin_port = htons(DEFAULT_PORT);
	
	// CONNECT TO THE SERVER
	iResult = connect(connectSocket, (struct sockaddr *)&servAddr, sizeof(servAddr));
	if(iResult == SOCKET_ERROR){
		closesocket(connectSocket);
		connectSocket = INVALID_SOCKET;
		return 1;
	}else{
		printf("was able to connect to server!\n");
	}
	
	// create thread to receive messages 
	receiveThread = CreateThread(NULL, 0, receiveMessagesProc, &connectSocket, 0, 0);
	
	// receive data/send data loop 
	do{
		std::string newInputHead = username + std::string(": ");
		std::string newInput;
		std::getline(std::cin, newInput);
		newInput = newInputHead + newInput; 
		const char *msg = (const char *)newInput.c_str();
		iResult = send(connectSocket, msg, (int)strlen(msg), 0);
	}while(iResult > 0);
	
	// at this point, connection is closed so disconnect
	iResult = shutdown(connectSocket, SD_SEND);
	if(iResult == SOCKET_ERROR){
		printf("shutdown failed: %d\n", WSAGetLastError());
		closesocket(connectSocket);
		WSACleanup();
		return 1;
	}
	
	closesocket(connectSocket);
	WSACleanup();
	
	return 0;
}