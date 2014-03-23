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
#include <time.h>
#include "FileTransferProtocolClient.h"

//user defined port number
#define REQUEST_PORT 0x7070;

using namespace std;

//socket data types
SOCKET s;

char localhost[11],
     remotehost[11],
	 direction[11];

//host data types
HOSTENT *hp;
HOSTENT *rp;

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

	srand(time(NULL));

	WSADATA wsadata;

	try {

		if (WSAStartup(0x0202,&wsadata)!=0){  
			cout<<"Error in starting WSAStartup()" << endl;
		} else {
			//buffer="WSAStartup was successful\n";   
			//WriteFile(test,buffer,sizeof(buffer),&dwtest,NULL); 

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

		char cwd[255];
		_getcwd(cwd, 255);
		cout << "Current directory: " << cwd << endl;

		while (1) {

			//Ask for name of remote server
			cout << "Type name of FTP server (type \"quit\" to exit): " << flush;
			cin >> remotehost;

			if (strcmp("quit", remotehost) == 0) {
				cout << "Quitting client..." << endl;
				break;
			}
			cout << "Remote host name is: \"" << remotehost << "\"" << endl;

			if ((rp = gethostbyname(remotehost)) == NULL)
				throw "remote gethostbyname failed\n";

			//Specify server address for client to connect to server.
			SOCKADDR_IN sa_in;      // fill with server info, IP, port
			int port = REQUEST_PORT;
			memset(&sa_in, 0, sizeof(sa_in));
			memcpy(&sa_in.sin_addr, rp->h_addr, rp->h_length);
			sa_in.sin_family = rp->h_addrtype;
			sa_in.sin_port = htons(port);

			//Display the host machine internet address
			cout << "Connecting to remote host:";
			cout << inet_ntoa(sa_in.sin_addr) << endl;

			FileTransferProtocolClient client(&sa_in, sizeof(sa_in));

			cout << "Files available on this server: " << endl;
			client.list();

			//Ask name of file to upload
			cout << "Type name of file to be transferred: " << flush;
			char filename[80];
			cin >> filename;

			//Ask direction
			cout << "Type direction of transfer (put/get): " << flush;
			cin >> direction;

			if (strcmp("get", direction) == 0) {
				//Check that we can open the file for writing
				ofstream theFile(filename, ios::out | ios::binary | ios::trunc);
				if (!theFile) {
					cout << "Could not open " << filename << " for writing!" << endl;
					continue;
				}

				//We are ready to receive data.
				client.receive(filename, theFile);
				theFile.close();
			}
			else if (strcmp("put", direction) == 0) {
				//Check if file exists
				struct stat stats;
				if (stat(filename, &stats) != 0) {
					cout << "File does not exist or cannot be stat!" << endl;
					continue;
				}
				ifstream theFile(filename, ios::in | ios::binary);
				if (!theFile) {
					cout << "Could not open " << filename << " for reading!" << endl;
					continue;
				}
				cout << "About to transfer file " << filename << " (" << stats.st_size << " bytes)" << endl;

				// We are ready to send data.
				client.send(filename, theFile, stats.st_size);
				theFile.close();
			}
			else {
				cout << "Invalid direction (either put or get)" << endl;
			}
			client.quit();
		}
	} // try loop

	//Display any needed error response.
	catch (char *str) { cerr<<str<<":"<<dec<<WSAGetLastError()<<endl;}

	/* When done uninstall winsock.dll (WSACleanup()) and exit */ 
	WSACleanup();
	waitforenter();
	return 0;
}





