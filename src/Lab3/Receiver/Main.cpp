#include "PhilSock.h"
#include <WinSock2.h>
#include <iostream>
#include <sys/stat.h>
#include <conio.h>

#define TRACE true
#define FILE  "sample.pdf"

//#define LOCAL_PORT 5001
#define LOCAL_PORT 0x7070

using namespace std;

int main() {

	WSADATA wsadata;
	if (WSAStartup(0x0202, &wsadata) != 0) {
		cout << "Error in starting WSAStartup()" << endl;
		_getch();
		return 1;
	}

	SOCKADDR_IN sa;
	sa.sin_family = AF_INET;
	sa.sin_port = htons(LOCAL_PORT);
	sa.sin_addr.s_addr = htonl(INADDR_ANY);

	// Read WHOLE file into buffer.

	ofstream file(FILE, ios::binary | ios::trunc);
	if (!file) {
		cout << "Could not open file for writing.\n";
		_getch();
		return 1;
	}

	net::ServerSocket socket(AF_INET, 0, TRACE, &sa, sizeof(sa));

	cout << "Listening for connections." << endl;
	socket.listen(10);

	cout << "Received connection, waiting for file size." << endl;
	size_t filesize;
	socket.recv((char*)&filesize, sizeof(filesize), 0);

	char * buf = (char*)malloc(filesize);
	if (buf == NULL) {
		cout << "Could not allocate enough memory to hold whole file " << filesize << " bytes." << endl;
		_getch();
		return 1;
	}

	cout << "About to receive " << filesize << " bytes." << endl;

	int bytes = socket.recv(buf, filesize, 0);

	cout << "Received " << bytes << " bytes, writing to file " << FILE << endl;
	file.write(buf, filesize);
	

	free(buf);
	WSACleanup();
	cout << "Transfer successful. Press enter to close." << endl;
	_getch();
	return 0;
}