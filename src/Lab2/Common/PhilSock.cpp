#include "PhilSock.h"
#include <WinSock2.h>

SOCKET net::socket(int af, int protocol) {
	SOCKET winsocket = ::socket(af, SOCK_DGRAM, protocol);
	if (winsocket == INVALID_SOCKET) return winsocket;

	state *ptr = (state*)malloc(sizeof(state));
	ptr->winsocket = winsocket;
	ptr->af = af;
	ptr->protocol = protocol;
	
	return (SOCKET)ptr;
}

int net::connect(SOCKET s, const struct sockaddr * name, int namelen) {
	// First, initialize state
	state *st = (state*)s;
	memcpy(&st->dest, name, sizeof(sockaddr));
	st->dest_len = namelen;

	// Three-way handshake.
	// Step 1: Send the sequence number to the server
	dgram _syn;
	syn(_syn);
	st->this_seqno = _syn.seqno;
	sendto(st->winsocket, (const char*)&_syn, sizeof(_syn), 0, name, namelen);

	// Step 2: Wait for SYNACK
	dgram _synack;
	int recvbytes = recvfrom(st->winsocket, (char*)&_synack, sizeof(_synack), 0, 0, 0);
	if (recvbytes == sizeof(_synack) && _synack.type == SYNACK) {
		st->dest_seqno = _synack.seqno;

		// Step 3: Send ACK
		dgram _ack;
		ack(_ack, _synack);
		sendto(st->winsocket, (const char*)&_ack, sizeof(_ack), 0, name, namelen);
		return 0;
	}
	else {
		return SOCKET_ERROR;
	}
}

int net::bind(SOCKET s, const struct sockaddr * name, int namelen) {
	SOCKET winsocket = getwinsocket(s);
	return ::bind(winsocket, name, namelen);
}

int net::listen(SOCKET s, int backlog) {
	// First, initialize state
	state *st = (state*)s;
	memset(&st->dest, 0, sizeof(st->dest));
	st->dest_len = 0;
	memset(&st->incoming_syn, 0, sizeof(st->incoming_syn));

	// Wait for a SYN
	dgram _syn;
	while (true) {
		int recvbytes = recvfrom(st->winsocket, (char*)&_syn, sizeof(_syn), 0, &st->dest, &st->dest_len);
		if (recvbytes == sizeof(_syn) && _syn.type == SYN) {
			break;
		}
		// Drop the packet if it's not a SYN
	}
	st->incoming_syn = _syn;
	st->dest_seqno = _syn.seqno;
	// Here we return, and let the user accept the connection
	return 0;
}

SOCKET net::accept(SOCKET s, struct sockaddr * addr, int * addrlen) {
	// Complete the connection handshake.
	state *st = (state*)s;

	// Send a SYNACK
	dgram _synack;
	synack(_synack, st->incoming_syn);
	sendto(st->winsocket, (const char*)&_synack, sizeof(_synack), 0, &st->dest, st->dest_len);

	// Wait for ACK
	dgram _ack;
	while (true) {
		int recvbytes = recvfrom(st->winsocket, (char*)&_ack, sizeof(_ack), 0, 0, 0);
		if (recvbytes == sizeof(_ack) && _ack.type == ACK) {
			break;
		}
		// Drop the packet if it's not a ACK
	}

	state *c_st = (state*)malloc(sizeof(state));
	c_st->af = st->af;
	c_st->protocol = st->protocol;
	c_st->dest = st->dest;
	c_st->dest_len = st->dest_len;
	c_st->dest_seqno = st->dest_seqno;
	c_st->winsocket = st->winsocket;
	c_st->this_seqno = _synack.seqno;
	memset(&c_st->incoming_syn, 0, sizeof(c_st->incoming_syn));

	addr = &c_st->dest;
	addrlen = &c_st->dest_len;

	return (SOCKET)c_st;
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

void net::syn(dgram &d) {
	d.seqno = genSeqNo();
	d.type = SYN;
	d.size = 0;
	d.payload = NULL;
}

void net::synack(dgram &d, dgram prevsyn) {
	d.seqno = genSeqNo();
	d.type = SYNACK;
	d.size = 0;
	d.payload = NULL;
}

void net::ack(dgram &d, dgram prev) {
	d.seqno = prev.seqno;
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