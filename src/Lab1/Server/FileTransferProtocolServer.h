#pragma once
#include <string>
#include <WinSock2.h>

using namespace std;

class FileTransferProtocolServer
{
private:
	string docroot;

public:
	FileTransferProtocolServer(string docroot);
	~FileTransferProtocolServer();

	int serve(SOCKET s);

	int recvDirection(SOCKET s, char &direction);
	int recvFilename(SOCKET s, string &filename);
	int sendAck(SOCKET s);
	int sendErr(SOCKET s);

	virtual void onTransferBegin(char direction, string filename);
	virtual void onTransferProgress(string filename, size_t bytes_received, size_t total_bytes);
	virtual void onTransferComplete(string filename);
	virtual void onTransferError(string error);
};

