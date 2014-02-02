#pragma once
#include <WinSock2.h>
#include <string>

using namespace std;

class FileTransferProtocol
{
public:
	FileTransferProtocol();
	~FileTransferProtocol();

	int sendFilename(SOCKET s, string filename);
	int recvFilename(SOCKET s, string &filename);

	int sendCommand(SOCKET s, char command);
	int recvCommand(SOCKET s, char &command);
	int sendAck(SOCKET s);
	int sendErr(SOCKET s);
	int waitForAck(SOCKET s, string errmsg);

	virtual void onTransferBegin(char direction, string filename) = 0;
	virtual void onTransferProgress(string filename, size_t bytes_received, size_t total_bytes) = 0;
	virtual void onTransferComplete(string filename) = 0;
	virtual void onTransferError(string error) = 0;
};

