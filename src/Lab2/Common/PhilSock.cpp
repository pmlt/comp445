#define _CRT_RAND_S

#include "PhilSock.h"
#include <WinSock2.h>
#include <iostream>
#include <stdlib.h>

net::Socket::Socket(int af, int protocol, bool trace) :
		winsocket(::socket(af, SOCK_DGRAM, protocol)),
		af(af),
		protocol(protocol),
		dest_len(sizeof(dest)),
		this_seqno(0),
		dest_seqno(0),
		trace(trace),
		tracefile(TRACEFILE, std::ios::out | std::ios::trunc)
{
	unsigned long nonblocking = 1;
	if (winsocket == INVALID_SOCKET) throw new SocketException("Could not initialize socket!");
	if (0 != ioctlsocket(winsocket, FIONBIO, &nonblocking)) {
		throw new SocketException("Could not set socket to non-blocking mode!");
	}
}

net::Socket::~Socket() {
	::closesocket(winsocket);
}

int net::Socket::genSeqNo() {
	unsigned int rnd, range(SEQNO_MAX - SEQNO_MIN), offset(SEQNO_MIN);
	rand_s(&rnd);
	rnd = (rnd % range) + offset;
	return rnd;
}

int net::Socket::nextSeqNo(int seqNo) {
	return (seqNo + 1) & SEQNO_MASK;
}

void net::Socket::syn(dgram &d) {
	d.seqno = genSeqNo();
	d.ack_seqno = 0;
	d.type = SYN;
	d.size = 0;
}

void net::Socket::synack(dgram &d, dgram acked) {
	d.seqno = genSeqNo();
	d.ack_seqno = acked.seqno;
	d.type = SYNACK;
	d.size = 0;
}

void net::Socket::ack(dgram &d, int seqNo, dgram acked) {
	d.seqno = seqNo;
	d.ack_seqno = acked.seqno;
	d.type = ACK;
	d.size = 0;
}

void net::Socket::data(dgram * d, int seqNo, size_t sz) {
	d->seqno = seqNo;
	d->ack_seqno = 0;
	d->type = DATA;
	d->size = sz;
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
		data((dgram*)packet, this_seqno, pl_size);
		//Copy payload data
		memcpy(packet + hdr_size, buf, pl_size);

		// The packet is ready to be sent.
		while (true) {
			//Each iteration of this loop equals to an attempt to send the packet.

			// Send the packet
			sendto(winsocket, packet, pkt_size, 0, (const sockaddr *)&dest, dest_len);

			// Wait for ACK
			dgram _ack;
			int recvbytes = recv_dgram(_ack, 0, NULL);
			if (recvbytes == SOCKET_ERROR) {
				if (WSAGetLastError() == WSAETIMEDOUT) continue; // Timed out, try again
				// Some other error occured
				free(packet); // Don't leak
				throw new SocketException("Could not receive ACK from other endpoint!");
			}

			//Check that this is indeed an ACK
			if (_ack.type != ACK) {
				if (_ack.type == DATA || _ack.type == SYNACK) {
					// This is an old packet that should be re-acked.
					send_ack(lastAck.seqno, _ack);
				}
				continue;
			}

			//Check that this is the expected sequence number
			if (_ack.seqno != dest_seqno) continue;

			//Check that this ACK is ACKing our DATA packet specifically
			if (_ack.ack_seqno != ((dgram*)packet)->seqno) continue;

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
		dgram pkt_header;

		while (true) {

			// Each iteration of this loop equals to an attempt to receive a packet.
			int recvbytes = recv_dgram(pkt_header, pl_size, buf);
			if (recvbytes == SOCKET_ERROR) {
				if (WSAGetLastError() == WSAETIMEDOUT) continue; // Timed out, try again
				throw new SocketException("Could not receive DATA message from other endpoint!");
			}
			if (recvbytes < hdr_size) continue; //I don't know WHAT that is...

			// Check that packet is a DATA packet
			if (pkt_header.type != DATA) {
				if (pkt_header.type == SYNACK) {
					// This is an old SYNACK packet that must be re-acked.
					send_ack(lastAck.seqno, pkt_header);
				}
				continue;
			}

			// Check that it is of the correct sequence number
			if (pkt_header.seqno != dest_seqno) {
				// This is an old DATA packet that must be re-acked
				send_ack(lastAck.seqno, pkt_header);
				continue;
			}

			// We got the packet; send ACK
			send_ack(this_seqno, pkt_header);

			break;
		}
		// This is our packet! Increment sequence numbers, move buffer pointer, etc.
		this_seqno = nextSeqNo(this_seqno);
		dest_seqno = nextSeqNo(dest_seqno);
		buf += pl_size;
		len = max(0, len - pl_size);
		bytes_received += pl_size;
	}
	if (trace) tracefile << "RECEIVER: Received " << bytes_received << " bytes!\n";
	return bytes_received;
}

int net::Socket::recv_dgram(dgram &header, size_t pl_size, void *payload, bool acceptDest) {
	fd_set fds;
	int n;
	struct timeval tv;
	tv.tv_sec = NET_TIMEOUT / 1000;
	tv.tv_usec = NET_TIMEOUT * 1000;

	FD_ZERO(&fds);
	FD_SET(winsocket, &fds);

	// Wait till we get a signal from select()
	n = select(winsocket, &fds, NULL, NULL, &tv);
	if (n == 0) {
		// Timed out!
		WSASetLastError(WSAETIMEDOUT);
		return SOCKET_ERROR;
	}
	else if (n == -1) {
		// Some other error... hopefully WSA error is already set
		return SOCKET_ERROR;
	}

	sockaddr *dst(NULL);
	int *dst_l(NULL);

	if (acceptDest) {
		dst = (sockaddr*)&dest;
		dst_l = &dest_len;
	}

	size_t hdr_size = sizeof(dgram);
	if (pl_size == 0) {
		// No payload expected, no need to allocate memory
		return recvfrom(winsocket, (char*)&header, hdr_size, 0, dst, dst_l);
		// XXX Check sender!!
	}
	size_t pkt_size = hdr_size + pl_size;
	char * pkt = (char*)malloc(pkt_size);
	size_t recvbytes = recvfrom(winsocket, pkt, pkt_size, 0, dst, dst_l);
	// XXX Check sender!!
	if (recvbytes >= hdr_size) {
		header = *(dgram*)pkt; // Copy header bytes into dgram
	}
	if (recvbytes == pkt_size) {
		memcpy(payload, pkt + hdr_size, pl_size); // Copy payload bytes into buffer
	}
	free(pkt);
	return recvbytes;
}

int net::Socket::send_ack(int seqNo, dgram acked) {
	ack(lastAck, seqNo, acked);
	return send_dgram(lastAck);
}

int net::Socket::send_dgram(const dgram &d) {
	return sendto(winsocket, (const char*)&d, sizeof(dgram), 0, (const sockaddr*)&dest, dest_len);
}

net::ClientSocket::ClientSocket(int af, int protocol, bool trace, const struct sockaddr_in * name, int namelen) :
		Socket(af, protocol, trace) {

	dest = *name;
	dest_len = namelen;

	// Three-way handshake.
	// Step 1: Send the sequence number to the server
	dgram _syn;
	syn(_syn);
	if (trace) tracefile << "CLIENT: Sending SYN message with Seq No " << _syn.seqno << "\n";
	sendto(winsocket, (const char*)&_syn, sizeof(dgram), 0, (const sockaddr*)name, namelen);

	// Step 2: Wait for SYNACK
	while (true) {
		if (trace) tracefile << "CLIENT: Waiting for SYNACK message\n";

		dgram _synack;
		int recvbytes = recv_dgram(_synack, 0, NULL);
		if (recvbytes == SOCKET_ERROR) {
			if (WSAGetLastError() == WSAETIMEDOUT) continue; // Timed out, try again
			// Some other error occured
			throw new SocketException("Could not receive SYNACK message from server!");
		}
		if (recvbytes < sizeof(dgram)) continue; // Throw away unexpected packet
		if (_synack.type != SYNACK) continue; //Throw away unexpected packet

		if (trace) tracefile << "CLIENT: Received SYNACK with Seq No " << _synack.seqno << "\n";

		// Step 3: Send ACK
		int ack_seqno = nextSeqNo(_syn.seqno);
		if (trace) tracefile << "CLIENT: Sending ACK message with Seq No " << ack_seqno << "\n";
		send_ack(nextSeqNo(_syn.seqno), _synack);

		this_seqno = nextSeqNo(ack_seqno);
		dest_seqno = nextSeqNo(_synack.seqno);

		break;
	}
	if (trace) {
		tracefile << "CLIENT: Connection established!\n";
		tracefile << "Next sequence numbers will be:\n";
		tracefile << "  Client: " << this_seqno << "\n";
		tracefile << "  Server: " << dest_seqno << "\n";
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
		int recvbytes = recv_dgram(_syn, 0, NULL, true);
		if (recvbytes == SOCKET_ERROR) {
			if (WSAGetLastError() == WSAETIMEDOUT) continue; // Timed out, try again
			// Some other error occured
			throw new SocketException("Could not receive SYN from a client!");
		}
		if (recvbytes < sizeof(dgram)) continue; // Throw away unexpected packet
		if (_syn.type != SYN) continue; //Throw away unexpected packet

		break;
	}
	dest_seqno = nextSeqNo(_syn.seqno);

	if (trace) tracefile << "SERVER: Received SYN with Seq No " << _syn.seqno << "\n";
	
	// Complete the connection handshake.

	while (true) {
		// Send a SYNACK
		synack(_synack, _syn);
		if (trace) tracefile << "SERVER: Sending SYNACK with Seq No " << _synack.seqno << "\n";
		sendto(winsocket, (const char*)&_synack, sizeof(dgram), 0, (const sockaddr*)&dest, dest_len);

		// Wait for ACK
		if (trace) tracefile << "SERVER: Waiting for acknowledgement of SYNACK\n";
		int recvbytes = recv_dgram(_ack, 0, NULL);
		if (recvbytes == SOCKET_ERROR) {
			if (WSAGetLastError() == WSAETIMEDOUT) continue; // Timed out, try again
			// Some other error occured
			throw new SocketException("Could not receive SYNACK from client!");
		}
		if (recvbytes < sizeof(dgram)) continue; // Throw away unexpected packet
		if (_ack.type != ACK) {
			if (_ack.type == SYN) {
				// Client never got our SYNACK, must re-send it.
				sendto(winsocket, (const char*)&_synack, sizeof(dgram), 0, (const sockaddr*)&dest, dest_len);
			}
			continue; //Throw away unexpected packet
		}
		if (_ack.seqno != dest_seqno) continue; // Throw away out-of-sync packet
		if (_ack.ack_seqno != _synack.seqno) continue; // This ACK is not for our SYNACK!

		if (trace) tracefile << "SERVER: Received ACK with Seq No " << _ack.seqno << "\n";
		break;
	}

	tracefile << "Accepted connection from " << inet_ntoa(dest.sin_addr) << ":"
		 << std::hex << htons(dest.sin_port) << std::endl;

	this_seqno = nextSeqNo(_synack.seqno);
	dest_seqno = nextSeqNo(_ack.seqno);

	if (trace) {
		tracefile << "SERVER: Received SYNACK acknowledgement, connection established!\n";
		tracefile << "Next sequence numbers will be:\n";
		tracefile << "  Client: " << dest_seqno << "\n";
		tracefile << "  Server: " << this_seqno << "\n";
	}
}
