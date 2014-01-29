// CLIENT TCP PROGRAM
// Revised and tidied up by
// J.W. Atwood
// 1999 June 30



char* getmessage(char *);



/* send and receive codes between client and server */
/* This is your basic WINSOCK shell */
#pragma comment( linker, "/defaultlib:ws2_32.lib" )
#include <winsock2.h>
#include <ws2tcpip.h>

#include <winsock.h>
#include <stdio.h>
#include <iostream>

#include <string.h>

#include <windows.h>

#include <conio.h>
#include <fstream>
#include <sys/stat.h>
#include <direct.h>
#include "StreamSocketSender.h"

using namespace std;

//user defined port number
#define REQUEST_PORT 0x7070;

int port=REQUEST_PORT;



//socket data types
SOCKET s;
SOCKADDR_IN sa;         // filled by bind
SOCKADDR_IN sa_in;      // fill with server info, IP, port



//buffer data types
char szbuffer[128];

char *buffer;

int ibufferlen=0;

int ibytessent;
int ibytesrecv=0;



//host data types
HOSTENT *hp;
HOSTENT *rp;

char localhost[11],
     remotehost[11];


//other

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
	//int ch;
	//while (((ch = getchar()) != '\n') && (ch != EOF)) /* void */;
}
int main(void){

	WSADATA wsadata;

	try {

		if (WSAStartup(0x0202,&wsadata)!=0){  
			cout<<"Error in starting WSAStartup()" << endl;
		} else {
			buffer="WSAStartup was successful\n";   
			WriteFile(test,buffer,sizeof(buffer),&dwtest,NULL); 

			/* Display the wsadata structure */
			cout<< endl
				<< "wsadata.wVersion "       << wsadata.wVersion       << endl
				<< "wsadata.wHighVersion "   << wsadata.wHighVersion   << endl
				<< "wsadata.szDescription "  << wsadata.szDescription  << endl
				<< "wsadata.szSystemStatus " << wsadata.szSystemStatus << endl
				<< "wsadata.iMaxSockets "    << wsadata.iMaxSockets    << endl
				<< "wsadata.iMaxUdpDg "      << wsadata.iMaxUdpDg      << endl;
		}  


		//Display name of local host.

		gethostname(localhost,10);
		cout<<"Local host name is \"" << localhost << "\"" << endl;

		if((hp=gethostbyname(localhost)) == NULL) 
			throw "gethostbyname failed\n";

		//Ask name of file to upload

		char cwd[255];
		_getcwd(cwd, 255);
		cout << "Current directory: " << cwd << endl;
		cout << "Please enter the name of the file to upload :" << flush;
		char filename[80];
		cin >> filename;
		struct stat stats;
		if (stat(filename, &stats) != 0) {
			cout << "File does not exist or cannot be stat!" << endl;
			waitforenter();
			exit(1);
		}
		ifstream theFile(filename, ios::in | ios::binary);
		if (!theFile) {
			cout << "Could not open " << filename << " for reading!" << endl;
			waitforenter();
			exit(1);
		}
		cout << "About to transfer file " << filename << " (" << stats.st_size << " bytes)" << endl;

		//Ask for name of remote server

		cout << "please enter your remote server name :" << flush ;   
		cin >> remotehost ;
		cout << "Remote host name is: \"" << remotehost << "\"" << endl;

		if((rp=gethostbyname(remotehost)) == NULL)
			throw "remote gethostbyname failed\n";

		//Create the socket
		if((s = socket(AF_INET,SOCK_STREAM,0))==INVALID_SOCKET) 
			throw "Socket failed\n";
		/* For UDP protocol replace SOCK_STREAM with SOCK_DGRAM */

		//Specify server address for client to connect to server.
		memset(&sa_in,0,sizeof(sa_in));
		memcpy(&sa_in.sin_addr,rp->h_addr,rp->h_length);
		sa_in.sin_family = rp->h_addrtype;   
		sa_in.sin_port = htons(port);

		//Display the host machine internet address

		cout << "Connecting to remote host:";
		cout << inet_ntoa(sa_in.sin_addr) << endl;

		//Connect Client to the server
		if (connect(s,(LPSOCKADDR)&sa_in,sizeof(sa_in)) == SOCKET_ERROR)
			throw "connect failed\n";

		// We are ready to send data.
		StreamSocketSender sender(128); //Send chunks of 128 bytes
		sender.send(s, theFile, stats.st_size);
		theFile.close();
		cout << "File sent; waiting for acknowledgement from server..." << endl;

		//wait for reception of server response.
		ibytesrecv=0; 
		if((ibytesrecv = recv(s,szbuffer,128,0)) == SOCKET_ERROR)
			throw "Receive failed\n";
		else
			cout << "hip hip hoorah!: Successful message replied from server: " << szbuffer;      

	} // try loop

	//Display any needed error response.

	catch (char *str) { cerr<<str<<":"<<dec<<WSAGetLastError()<<endl;}

	//close the client socket
	closesocket(s);

	/* When done uninstall winsock.dll (WSACleanup()) and exit */ 
	WSACleanup();
	waitforenter();
	return 0;
}




