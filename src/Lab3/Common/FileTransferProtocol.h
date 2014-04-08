#pragma once
#include "PhilSock.h"
#include <string>

#define SUGGESTED_STREAM_CHUNK_SIZE (1024*1024)

using namespace std;

class FileTransferProtocol
{
public:
	FileTransferProtocol();
	~FileTransferProtocol();

	int sendFilename(net::Socket &s, string filename);
	int recvFilename(net::Socket &s, string &filename);

	int sendCommand(net::Socket &s, char command);
	int recvCommand(net::Socket &s, char &command);
	int sendAck(net::Socket &s);
	int sendErr(net::Socket &s);
	int waitForAck(net::Socket &s, string errmsg);

	virtual void onTransferBegin(char direction, string filename) = 0;
	virtual void onTransferProgress(string filename, size_t bytes_received, size_t total_bytes) = 0;
	virtual void onTransferComplete(string filename) = 0;
	virtual void onTransferError(string error) = 0;
};

