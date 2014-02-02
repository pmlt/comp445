#pragma once
#include "Thread.h"
#include <WinSock2.h>


class FTPThread :
	public Thread
{
private:
	SOCKET client;
public:
	FTPThread(SOCKET s);
	~FTPThread();

	virtual void run();
};

