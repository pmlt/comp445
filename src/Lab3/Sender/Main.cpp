#include "PhilSock.h"
#include <WinSock2.h>
#include <iostream>
#include <sys/stat.h>
#include <conio.h>

#define TRACE true
#define FILE  "sample.pdf"

#define REMOTE_HOST "localhost"
//#define REMOTE_PORT 7000
//#define LOCAL_PORT  5000
#define REMOTE_PORT 0x7070
#define LOCAL_PORT  0x7071

using namespace std;

int main() {

	WSADATA wsadata;
	if (WSAStartup(0x0202, &wsadata) != 0) {
		cout << "Error in starting WSAStartup()" << endl;
		_getch();
		return 1;
	}

	HOSTENT *rp;
	if ((rp = gethostbyname(REMOTE_HOST)) == NULL) {
		cout << "remote gethostbyname failed\n";
		_getch();
		return 1;
	}
	SOCKADDR_IN sa_in;
	memset(&sa_in, 0, sizeof(sa_in));
	memcpy(&sa_in.sin_addr, rp->h_addr, rp->h_length);
	sa_in.sin_family = rp->h_addrtype;
	sa_in.sin_port = htons(REMOTE_PORT);


	struct stat stats;
	if (0 != stat(FILE, &stats)) {
		cout << "Could not stat file " << FILE << endl;
		_getch();
		return 1;
	}
	// Read WHOLE file into buffer.
	char * buf = (char*)malloc(stats.st_size);
	if (buf == NULL) {
		cout << "Could not allocate enough memory to hold whole file " << stats.st_size << " bytes." << endl;
		_getch();
		return 1;
	}

	ifstream file(FILE, ios::binary);
	if (!file) {
		cout << "Could not open file for reading.\n";
		_getch();
		return 1;
	}
	file.read(buf, stats.st_size);

	cout << "Press enter to send file " << FILE << " (" << stats.st_size << " bytes)." << endl;
	_getch();

	net::ClientSocket socket(AF_INET, 0, TRACE, LOCAL_PORT, &sa_in, sizeof(sa_in));

	// First send the size, then send the file
	socket.send((const char*)&(stats.st_size), sizeof(stats.st_size), 0);
	int bsent = socket.send(buf, stats.st_size, 0);
	cout << "Sent " << bsent << " bytes." << endl;
	
	free(buf);

	WSACleanup();
	cout << "Transfer successful. Press enter to close." << endl;
	_getch();
	return 0;
}