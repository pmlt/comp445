#include "FTPThread.h"
#include <iostream>
#include "FileTransferProtocolServer.h"

FTPThread::FTPThread(SOCKET s)
{
	this->client = s;
}


FTPThread::~FTPThread()
{
}

void FTPThread::run()
{
	FileTransferProtocolServer server(".");
	server.serve(this->client);
	closesocket(this->client);
}