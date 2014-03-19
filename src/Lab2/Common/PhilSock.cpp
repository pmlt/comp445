#include "PhilSock.h"
#include <WinSock2.h>
#include <iostream>

net::Socket::Socket(int af, int protocol, bool trace) :
		winsocket(::socket(af, SOCK_DGRAM, protocol)),
		af(af),
		protocol(protocol),
		dest_len(sizeof(dest)),
		this_seqno(0),
		dest_seqno(0),
		trace(trace),
		tracefile(TRACEFILE, std::ios::out | std::ios::app)
{
	if (winsocket == INVALID_SOCKET) throw new SocketException("Could not initialize socket!");
}

net::Socket::~Socket() {
	::closesocket(winsocket);
}

int net::Socket::genSeqNo() {
	return (rand() / (SEQNO_MAX - SEQNO_MIN)) + SEQNO_MIN;
}

int net::Socket::nextSeqNo(int seqNo) {
	return (seqNo + 1) & SEQNO_MASK;
}

void net::Socket::syn(dgram &d) {
	d.seqno = genSeqNo();
	d.type = SYN;
	d.size = 0;
	d.payload = NULL;
}

void net::Socket::synack(dgram &d, dgram prevsyn) {
	d.seqno = genSeqNo();
	d.type = SYNACK;
	d.size = 0;
	d.payload = NULL;
}

void net::Socket::ack(dgram &d, dgram prev) {
	d.seqno = prev.seqno;
	d.type = ACK;
	d.size = 0;
	d.payload = NULL;
}

void net::Socket::data(dgram * d, int seqNo, size_t sz, void * buf) {
	d->seqno = seqNo;
	d->type = DATA;
	d->size = sz;
	d->payload = buf;
}

int net::Socket::send(const char * buf, int len, int flags) {

	if (trace) tracefile << "SENDER: About to send " << len << " bytes in chunks of " << MAX_PAYLOAD_SIZE << "\n";
	int bytes_sent = 0;
	while (len > 0) {
		// Each iteration of this loop equals to a packet being sent
		if (trace) tracefile << "SENDER: Sending packet #" << this_seqno << "\n";

		// First, prepare packet
		size_t pl_size = min(MAX_PAYLOAD_SIZE, len);
		size_t hdr_size = sizeof(dgram);
		size_t pkt_size = hdr_size + pl_size;
		char * packet = (char*)malloc(pkt_size);
		// Initialize headers
		data((dgram*)packet, this_seqno, pl_size, (void*)buf);
		//Copy payload data
		memcpy(packet + hdr_size, buf, pl_size);

		// The packet is ready to be sent.
		while (true) {
			//Each iteration of this loop equals to an attempt to send the packet.

			// Send the packet
			sendto(winsocket, packet, pkt_size, 0, (const sockaddr *)&dest, dest_len);

			// Wait for ACK
			dgram _ack;
			int recvbytes = recvfrom(winsocket, (char*)&_ack, sizeof(_ack), 0, NULL, NULL);
			if (recvbytes == SOCKET_ERROR) {
				if (WSAGetLastError() == WSAETIMEDOUT) continue; // Timed out, try again
				// Some other error occured
				free(packet); // Don't leak
				throw new SocketException("Could not receive ACK from other endpoint!");
			}

			//Check that this is indeed an ACK
			if (_ack.type != ACK) continue;

			//Check that this is the expected sequence number
			if (_ack.seqno != dest_seqno) continue;

			// Packet was acknowledged!
			break;
		}
		// Packet is sent; increment sequence numbers, free memory and move on to next.
		this_seqno = nextSeqNo(this_seqno);
		dest_seqno = nextSeqNo(dest_seqno);
		free(packet);
		buf += pl_size;
		len = max(0, len - pl_size);
		bytes_sent += pl_size;
	}
	// Sent all payload bytes!

	if (trace) tracefile << "SENDER: Completed sending " << bytes_sent << " bytes!\n";
	return bytes_sent;
}

int net::Socket::recv(char * buf, int len, int flags) {

	if (trace) tracefile << "RECEIVER: Expecting " << len << " bytes in chunks of " << MAX_PAYLOAD_SIZE << " bytes\n";

	size_t bytes_received = 0;
	while (len > 0) {
		if (trace) tracefile << "RECEIVER: Expecting packet #" << dest_seqno << "\n";
		// Each iteration of this loop equals to a packet being received
		size_t hdr_size = sizeof(dgram);
		size_t pl_size = min(MAX_PAYLOAD_SIZE, len);
		size_t pkt_size = hdr_size+pl_size;
		char *pkt = (char*)malloc(pkt_size);

		while (true) {

			// Each iteration of this loop equals to an attempt to receive a packet.
			size_t recvbytes = recvfrom(winsocket, pkt, pkt_size, 0, NULL, NULL);
			if (recvbytes == SOCKET_ERROR) {
				free(pkt);
				throw new SocketException("Could not receive DATA message from other endpoint!");
			}
			if (recvbytes < pkt_size) continue; //I don't know WHAT that is...

			dgram *pkt_header = (dgram*)pkt;

			// Check that packet is a DATA packet
			if (pkt_header->type != DATA) continue;

			// Check that it is of the correct sequence number
			if (pkt_header->seqno != dest_seqno) continue;

			// We got the packet; send ACK
			dgram _ack;
			ack(_ack, *pkt_header);
			sendto(winsocket, (const char*)&_ack, sizeof(_ack), 0, (const sockaddr*)&dest, dest_len);

			break;
		}
		// This is our packet! Copy into output buffer, increment sequence numbers, free memory, etc.
		this_seqno = nextSeqNo(this_seqno);
		dest_seqno = nextSeqNo(dest_seqno);
		memcpy(buf, pkt + hdr_size, pl_size);
		buf += pl_size;
		len = max(0, len - pl_size);
		free(pkt);
		bytes_received += pl_size;
	}
	if (trace) tracefile << "RECEIVER: Received " << bytes_received << " bytes!\n";
	return bytes_received;
}

net::ClientSocket::ClientSocket(int af, int protocol, bool trace, const struct sockaddr_in * name, int namelen) :
		Socket(af, protocol, trace) {

	dest = *name;
	dest_len = namelen;

	// Three-way handshake.
	// Step 1: Send the sequence number to the server
	dgram _syn;
	syn(_syn);
	this_seqno = _syn.seqno;
	if (trace) tracefile << "CLIENT: Sending SYN message with Seq No " << _syn.seqno << "\n";
	sendto(winsocket, (const char*)&_syn, sizeof(_syn), 0, (const sockaddr*)name, namelen);

	// Step 2: Wait for SYNACK
	while (true) {
		if (trace) tracefile << "CLIENT: Waiting for SYNACK message\n";

		dgram _synack;
		int recvbytes = recvfrom(winsocket, (char*)&_synack, sizeof(_synack), 0, 0, 0);
		if (recvbytes == SOCKET_ERROR) {
			if (WSAGetLastError() == WSAETIMEDOUT) continue; // Timed out, try again
			// Some other error occured
			throw new SocketException("Could not receive SYNACK message from server!");
		}
		if (recvbytes != sizeof(_synack)) continue; // Throw away unexpected packet
		if (_synack.type != SYNACK) continue; //Throw away unexpected packet

		if (trace) tracefile << "CLIENT: Received SYNACK with Seq No " << _synack.seqno << "\n";

		this_seqno = nextSeqNo(this_seqno);
		dest_seqno = nextSeqNo(_synack.seqno);

		// Step 3: Send ACK
		if (trace) tracefile << "CLIENT: Acknowledging received SYNACK\n";
		dgram _ack;
		ack(_ack, _synack);
		sendto(winsocket, (const char*)&_ack, sizeof(_ack), 0, (const sockaddr*)name, namelen);

		if (trace) tracefile << "CLIENT: Connection established!\n";
		break;
	}
}

net::ServerSocket::ServerSocket(int af, int protocol, bool trace, const struct sockaddr_in * name, int namelen):
		Socket(af, protocol, trace) {
	if (SOCKET_ERROR == ::bind(winsocket, (const sockaddr*)name, namelen)) {
		throw new SocketException("Could not bind server socket!");
	}
}

void net::ServerSocket::listen(int backlog) {
	// Wait for a SYN
	dgram _syn, _synack, _ack;
	while (true) {
		if (trace) tracefile << "SERVER: Waiting for SYN message\n";
		int recvbytes = recvfrom(winsocket, (char*)&_syn, sizeof(_syn), 0, (sockaddr*)&dest, &dest_len);
		if (recvbytes == SOCKET_ERROR) {
			if (WSAGetLastError() == WSAETIMEDOUT) continue; // Timed out, try again
			// Some other error occured
			throw new SocketException("Could not receive SYN from a client!");
		}
		if (recvbytes != sizeof(_syn)) continue; // Throw away unexpected packet
		if (_syn.type != SYN) continue; //Throw away unexpected packet

		break;
	}
	dest_seqno = _syn.seqno;

	if (trace) tracefile << "SERVER: Received SYN with Seq No " << _syn.seqno << "\n";
	
	// Complete the connection handshake.

	while (true) {
		// Send a SYNACK
		synack(_synack, _syn);
		if (trace) tracefile << "SERVER: Sending SYNACK with Seq No " << _synack.seqno << "\n";
		sendto(winsocket, (const char*)&_synack, sizeof(_synack), 0, (const sockaddr*)&dest, dest_len);

		// Wait for ACK
		if (trace) tracefile << "SERVER: Waitin for acknowledgement of SYNACK\n";
		int recvbytes = recvfrom(winsocket, (char*)&_ack, sizeof(_ack), 0, 0, 0);
		if (recvbytes == SOCKET_ERROR) {
			if (WSAGetLastError() == WSAETIMEDOUT) continue; // Timed out, try again
			// Some other error occured
			throw new SocketException("Could not receive SYNACK from client!");
		}
		if (recvbytes != sizeof(_ack)) continue; // Throw away unexpected packet
		if (_ack.type != ACK) continue; //Throw away unexpected packet
		break;
	}

	std::cout << "Accepted connection from " << inet_ntoa(dest.sin_addr) << ":"
		 << std::hex << htons(dest.sin_port) << std::endl;

	this_seqno = nextSeqNo(this_seqno);
	dest_seqno = nextSeqNo(_synack.seqno);

	if (trace) tracefile << "SERVER: Received SYNACK acknowledgement, connection established!\n";
}
