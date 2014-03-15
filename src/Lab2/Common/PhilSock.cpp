#include "PhilSock.h"
#include <WinSock2.h>

SOCKET net::socket(int af, int protocol) {
	SOCKET winsocket = ::socket(af, SOCK_DGRAM, protocol);
	if (winsocket == INVALID_SOCKET) return winsocket;

	state *ptr = (state*)malloc(sizeof(state));
	ptr->winsocket = winsocket;
	
	return (SOCKET)ptr;
}

int net::connect(SOCKET s, const struct sockaddr * name, int namelen) {
	// Three-way handshake.
	int seqNo = genSeqNo();
	// Step 1: Send the sequence number to the server
	dgram dinit;
	init(dinit);

	SOCKET winsocket = getwinsocket(s);
	return ::connect(winsocket, name, namelen);
}

int net::bind(SOCKET s, const struct sockaddr * name, int namelen) {
	SOCKET winsocket = getwinsocket(s);
	return ::bind(winsocket, name, namelen);
}

int net::listen(SOCKET s, int backlog) {
	SOCKET winsocket = getwinsocket(s);
	return ::listen(winsocket, backlog);
}

SOCKET net::accept(SOCKET s, struct sockaddr * addr, int * addrlen) {
	SOCKET winsocket = getwinsocket(s);
	SOCKET client = ::accept(winsocket, addr, addrlen);
	if (winsocket == INVALID_SOCKET) return winsocket;
	state *ptr = (state*)malloc(sizeof(state));
	ptr->winsocket = client;
	return (SOCKET)ptr;
}

int net::send(SOCKET s, const char * buf, int len, int flags) {
	SOCKET winsocket = getwinsocket(s);
	return ::send(winsocket, buf, len, flags);
}

int net::recv(SOCKET s, char * buf, int len, int flags) {
	SOCKET winsocket = getwinsocket(s);
	return ::recv(winsocket, buf, len, flags);
}

int net::closesocket(SOCKET s) {
	SOCKET winsocket = getwinsocket(s);
	int ret = ::closesocket(winsocket);
	free((void*)s);
	return ret;
}

SOCKET net::getwinsocket(SOCKET s) {
	state *ptr = (state*)s;
	return ptr->winsocket;
}

int net::genSeqNo() {
	return (rand() / (SEQNO_MAX - SEQNO_MIN)) + SEQNO_MIN;
}

int net::nextSeqNo(int seqNo) {
	return (seqNo + 1) & SEQNO_MASK;
}

void net::init(dgram &d) {
	d.seqno = genSeqNo();
	d.type = INIT;
	d.size = 0;
	d.payload = NULL;
}

void net::syn(dgram &d) {
	d.seqno = genSeqNo();
	d.type = SYN;
	d.size = 0;
	d.payload = NULL;
}

void net::synack(dgram &d, int seqNo) {
	d.seqno = seqNo;
	d.type = SYNACK;
	d.size = 0;
	d.payload = NULL;
}

void net::ack(dgram &d, int seqNo) {
	d.seqno = seqNo;
	d.type = ACK;
	d.size = 0;
	d.payload = NULL;
}

void net::data(dgram &d, int seqNo, size_t sz, void * buf) {
	d.seqno = seqNo;
	d.type = DATA;
	d.size = sz;
	d.payload = buf;
}