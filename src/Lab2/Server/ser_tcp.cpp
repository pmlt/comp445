//    SERVER TCP PROGRAM
// revised and tidied up by
// J.W. Atwood
// 1999 June 30
// There is still some leftover trash in this code.

/* send and receive codes between client and server */
/* This is your basic WINSOCK shell */
#pragma once
#pragma comment( linker, "/defaultlib:ws2_32.lib" )
#include <winsock2.h>
#include "PhilSock.h"
#include <ws2tcpip.h>
#include <process.h>
#include <iostream>
#include <fstream>
#include <windows.h>
#include <conio.h>
#include <direct.h>
#include <time.h>
#include "FTPThread.h"
#include "FileTransferProtocolServer.h"

using namespace std;

//port data types

#define REQUEST_PORT 5001
//#define REQUEST_PORT 0x7070

int port=REQUEST_PORT;

//socket data types
SOCKET s;

SOCKET s1;
SOCKADDR_IN sa;      // filled by bind
SOCKADDR_IN sa1;     // fill with server info, IP, port
union {struct sockaddr generic;
	struct sockaddr_in ca_in;}ca;

	int calen=sizeof(ca); 

	//buffer data types
	char szbuffer[128];

	char *buffer;
	int ibufferlen;
	int ibytesrecv;

	int ibytessent;

	//host data types
	char localhost[11];

	HOSTENT *hp;

	//wait variables
	int nsa1;
	int r,infds=1, outfds=0;
	struct timeval timeout;
	const struct timeval *tp=&timeout;

	fd_set readfds;

	//others
	HANDLE test;

	DWORD dwtest;

	//reference for used structures

	/*  * Host structure

	    struct  hostent {
	    char    FAR * h_name;             official name of host *
	    char    FAR * FAR * h_aliases;    alias list *
	    short   h_addrtype;               host address type *
	    short   h_length;                 length of address *
	    char    FAR * FAR * h_addr_list;  list of addresses *
#define h_addr  h_addr_list[0]            address, for backward compat *
};

	 * Socket address structure

	 struct sockaddr_in {
	 short   sin_family;
	 u_short sin_port;
	 struct  in_addr sin_addr;
	 char    sin_zero[8];
	 }; */

	void waitforenter() {
		_getch();
	}
	int main(void){
		srand(time(NULL));

		WSADATA wsadata;

		try{        		 
			if (WSAStartup(0x0202,&wsadata)!=0){  
				cout<<"Error in starting WSAStartup()\n";
			}else{
				buffer="WSAStartup was suuccessful\n";   
				WriteFile(test,buffer,sizeof(buffer),&dwtest,NULL); 

				/* display the wsadata structure */
				cout<< endl
					<< "wsadata.wVersion "       << wsadata.wVersion       << endl
					<< "wsadata.wHighVersion "   << wsadata.wHighVersion   << endl
					<< "wsadata.szDescription "  << wsadata.szDescription  << endl
					<< "wsadata.szSystemStatus " << wsadata.szSystemStatus << endl
					<< "wsadata.iMaxSockets "    << wsadata.iMaxSockets    << endl
					<< "wsadata.iMaxUdpDg "      << wsadata.iMaxUdpDg      << endl;
			}

			//Display info of local host

			gethostname(localhost,10);
			cout<<"FTP server hostname: "<<localhost<< endl;

			if((hp=gethostbyname(localhost)) == NULL) {
				cout << "gethostbyname() cannot get local host info?"
					<< WSAGetLastError() << endl; 
				exit(1);
			}

			char cwd[255];
			_getcwd(cwd, 255);
			cout << "Serving directory: " << cwd << endl;

			//Create the server socket
			//if((s = net::socket(AF_INET,0))==INVALID_SOCKET) 
			//	throw "can't initialize socket";
			// For UDP protocol replace SOCK_STREAM with SOCK_DGRAM 


			//Fill-in Server Port and Address info.
			sa.sin_family = AF_INET;
			sa.sin_port = htons(port);
			sa.sin_addr.s_addr = htonl(INADDR_ANY);


			//Bind the server port
			FileTransferProtocolServer server(".", &sa, sizeof(sa));
			cout << "Bind was successful" << endl;

			//Successfull bind, now listen for client requests.

			//FD_ZERO(&readfds);

			//wait loop
			//FTPThread *t;
			while(1)
			{
				cout << "Waiting for new client...\n";
				server.waitForClient();

				//FD_SET(net::getwinsocket(s),&readfds);  //always check the listener

				//if(!(outfds=select(infds,&readfds,NULL,NULL,tp))) {}

				//else if (outfds == SOCKET_ERROR) throw "failure in Select";

				//else if (FD_ISSET(net::getwinsocket(s),&readfds))  cout << "got a connection request" << endl; 

				//Found a connection request, try to accept. 

				//if((s1=net::accept(s,&ca.generic,&calen))==INVALID_SOCKET)
				//	throw "Couldn't accept connection\n";

				//Connection request accepted.
				//cout << "accepted connection from " << inet_ntoa(ca.ca_in.sin_addr) << ":"
				//	<< hex << htons(ca.ca_in.sin_port) << endl;

				server.serve();
				//net::closesocket(s1);
				// Spin a new thread to serve this client
				//t = new FTPThread(s1);
				//t->start();
			}//wait loop

		} //try loop

		//Display needed error message.

		catch(char* str) { cerr<<str<<WSAGetLastError()<<endl;}		

		//close server socket
		//net::closesocket(s);

		/* When done uninstall winsock.dll (WSACleanup()) and exit */ 
		WSACleanup();
		waitforenter();
		return 0;
	}




