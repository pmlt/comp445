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
	d.type = SYN;
	d.size = 0;
}

void net::Socket::synack(dgram &d, dgram acked) {
	d.seqno = genSeqNo();
	d.type = SYNACK;
	d.size = 0;
}

void net::Socket::ack(dgram &d, dgram acked) {
	d.seqno = acked.seqno;
	d.type = ACK;
	d.size = 0;
}

void net::Socket::fin(dgram &d, dgram ack) {
	d.seqno = ack.seqno;
	d.type = FIN;
	d.size = 0;
}

void net::Socket::data(dgram &d, int seqNo, size_t sz, const char * payload) {
	d.seqno = seqNo;
	d.type = DATA;
	d.size = sz;
	//Copy payload data
	memcpy((void*)d.payload, payload, sz);
}

int net::Socket::send(const char * buf, int len, int flags) {

	dgram packet, _ack;
	if (trace) tracefile << "SENDER: About to send " << len << " bytes in chunks of " << MAX_PAYLOAD_SIZE << "\n";
	int bytes_sent = 0;
	while (len > 0) {
		// Each iteration of this loop equals to a packet being sent
		if (trace) tracefile << "SENDER: Sending packet #" << this_seqno << "\n";

		// First, prepare packet
		size_t pl_size = min(MAX_PAYLOAD_SIZE, len);
		size_t pkt_size = sizeof(dgram);
		// Initialize headers
		data(packet, this_seqno, pl_size, buf);

		// The packet is ready to be sent.
		// Send the packet
		send_dgram(packet);

		while (true) {

			// Wait for ACK
			int recvbytes = recv_dgram(_ack);
			if (recvbytes == SOCKET_ERROR) {
				if (WSAGetLastError() == WSAETIMEDOUT) {
					// Timed out, try sending packet again
					send_dgram(packet);
					continue;
				}
				// Some other error occured
				traceError(WSAGetLastError(), "SOCKET_ERROR while recv");
				throw new SocketException("Could not receive ACK from other endpoint!");
			}

			//Check that this is indeed an ACK
			if (_ack.type != ACK) {
				if (_ack.type == SYNACK) {
					// This is an old packet that should be re-acked.
					send_ack(_ack);
				}
				continue;
			}

			//Check that this ACK is ACKing our DATA packet specifically
			if (_ack.seqno != packet.seqno) {
				// This is an old ACK that was lost in the last transfer.
				// Re-send FIN to finalize that transfer
				if (trace) tracefile << "SENDER: Re-sending FIN to finalize previous transfer.\n";
				dgram _fin;
				fin(_fin, packet);
				send_dgram(_fin);
				continue;
			}

			// Packet was acknowledged!
			break;
		}
		// Packet is sent; increment sequence numbers, free memory and move on to next.
		this_seqno = nextSeqNo(this_seqno);
		buf += pl_size;
		len = max(0, len - pl_size);
		bytes_sent += pl_size;
	}

	// Send FIN to signal end of transfer.
	if (trace) tracefile << "SENDER: Sending FIN to finalize transfer.\n";
	dgram _fin;
	fin(_fin, _ack);
	send_dgram(_fin);
	
	// Sent all payload bytes!
	if (trace) tracefile << "SENDER: Completed sending " << bytes_sent << " bytes!\n";
	return bytes_sent;
}

int net::Socket::recv(char * buf, int len, int flags) {

	if (trace) tracefile << "RECEIVER: Expecting " << len << " bytes in chunks of " << MAX_PAYLOAD_SIZE << " bytes\n";

	dgram pkt, _fin;
	size_t bytes_received = 0;
	while (len > 0) {
		if (trace) tracefile << "RECEIVER: Expecting packet #" << dest_seqno << "\n";
		// Each iteration of this loop equals to a packet being received
		size_t hdr_size = sizeof(dgram);

		while (true) {

			// Each iteration of this loop equals to an attempt to receive a packet.
			int recvbytes = recv_dgram(pkt);
			if (recvbytes == SOCKET_ERROR) {
				if (WSAGetLastError() == WSAETIMEDOUT) continue; // Timed out, try again
				traceError(WSAGetLastError(), "SOCKET_ERROR while recv");
				throw new SocketException("Could not receive DATA message from other endpoint!");
			}
			if (recvbytes < hdr_size) continue; //I don't know WHAT that is...

			// Check that packet is a DATA packet
			if (pkt.type != DATA) {
				if (pkt.type == SYNACK) {
					// This is an old SYNACK packet that must be re-acked.
					send_ack(pkt);
				}
				else if (pkt.type == ACK) {
					// This is an old ACK that was lost in the last transfer.
					// Re-send FIN to finalize that transfer
					if (trace) tracefile << "RECEIVER: Re-sending FIN to finalize previous transfer.\n";
					fin(_fin, pkt);
					send_dgram(_fin);
				}
				continue;
			}

			// Check that it is of the correct sequence number
			if (pkt.seqno != dest_seqno) {
				// This is an old DATA packet that must be re-acked
				if (trace) tracefile << "RECEIVER: Re-acking previous DATA packet.\n";
				send_ack(pkt);
				continue;
			}

			// We got the packet; send ACK
			send_ack(pkt);

			break;
		}
		// This is our packet! Increment sequence numbers, move buffer pointer, etc.
		dest_seqno = nextSeqNo(dest_seqno);
		memcpy(buf, pkt.payload, pkt.size); // Copy payload bytes into buffer
		buf += pkt.size;
		len = max(0, len - pkt.size);
		bytes_received += pkt.size;
	}

	// Wait for the FIN.
	if (trace) tracefile << "RECEIVER: Waiting for FIN.\n";
	while (true) {
		int rbytes = recv_dgram(_fin);
		if (rbytes == SOCKET_ERROR) {
			if (WSAGetLastError() == WSAETIMEDOUT) {
				send_ack(pkt); // Re-send last ACK, it got lost
				continue;
			}
			traceError(WSAGetLastError(), "SOCKET_ERROR while waiting for FIN");
			throw new SocketException("Could not receive FIN message from other endpoint!");
		}
		if (_fin.type == FIN) break;
		// Re-send last ACK, it got lost
		send_ack(pkt);
	}
	if (trace) tracefile << "RECEIVER: Received " << bytes_received << " bytes!\n";
	return bytes_received;
}

int net::Socket::recv_dgram(dgram &pkt, bool acceptDest) {
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

	size_t pkt_size = sizeof(dgram);
	return recvfrom(winsocket, (char*)&pkt, pkt_size, 0, dst, dst_l);
}

int net::Socket::send_ack(dgram acked) {
	dgram _ack;
	ack(_ack, acked);
	return send_dgram(_ack);
}

int net::Socket::send_dgram(const dgram &d) {
	return sendto(winsocket, (const char*)&d, sizeof(dgram), 0, (const sockaddr*)&dest, dest_len);
}

net::ClientSocket::ClientSocket(int af, int protocol, bool trace, int local_port, const struct sockaddr_in * name, int namelen) :
		Socket(af, protocol, trace) {

	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(local_port);
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	// This is dumb, but the router requires that the client socket binds to a static port.
	if (SOCKET_ERROR == ::bind(winsocket, (const sockaddr*)&local_addr, sizeof(local_addr))) {
		traceError(WSAGetLastError(), "SOCKET_ERROR while binding");
		throw new SocketException("Could not bind client socket!");
	}

	dest = *name;
	dest_len = namelen;

	// Three-way handshake.
	// Step 1: Send the sequence number to the server
	dgram _syn;
	syn(_syn);
	if (trace) tracefile << "CLIENT: Sending SYN message with Seq No " << _syn.seqno << "\n";
	int sentbytes = send_dgram(_syn);
	if (sentbytes == SOCKET_ERROR) {
		traceError(WSAGetLastError(), "SOCKET_ERROR while sendto");
		throw new SocketException("Could not send SYN");
	}

	this_seqno = nextSeqNo(_syn.seqno);

	// Step 2: Wait for SYNACK
	if (trace) tracefile << "CLIENT: Waiting for SYNACK message\n";
	while (true) {

		dgram _synack;
		int recvbytes = recv_dgram(_synack);
		if (recvbytes == SOCKET_ERROR) {
			if (WSAGetLastError() == WSAETIMEDOUT) {
				// Timed out, re-send syn
				send_dgram(_syn);
				continue; // Timed out, try again
			}
			// Some other error occured
			traceError(WSAGetLastError(), "SOCKET_ERROR while recv");
			throw new SocketException("Could not receive SYNACK message from server!");
		}
		if (recvbytes < sizeof(dgram)) continue; // Throw away unexpected packet
		if (_synack.type != SYNACK) continue; //Throw away unexpected packet

		if (trace) tracefile << "CLIENT: Received SYNACK with Seq No " << _synack.seqno << "\n";

		dest_seqno = nextSeqNo(_synack.seqno);

		// Step 3: Send ACK
		if (trace) tracefile << "CLIENT: Sending ACK in response to SYNACK\n";
		// Simulate loss of packet!
		send_ack(_synack);

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
		traceError(WSAGetLastError(), "SOCKET_ERROR while binding");
		throw new SocketException("Could not bind server socket!");
	}
}

void net::ServerSocket::listen(int backlog) {
	// Wait for a SYN
	dgram _syn, _synack, _ack;
	if (trace) tracefile << "SERVER: Waiting for SYN message\n";
	while (true) {
		int recvbytes = recv_dgram(_syn, true);
		if (recvbytes == SOCKET_ERROR) {
			if (WSAGetLastError() == WSAETIMEDOUT) continue; // Timed out, try again
			// Some other error occured
			traceError(WSAGetLastError(), "SOCKET_ERROR while recv");
			throw new SocketException("Could not receive SYN from a client!");
		}
		if (recvbytes < sizeof(dgram)) continue; // Throw away unexpected packet
		if (_syn.type != SYN) continue; //Throw away unexpected packet

		break;
	}
	dest_seqno = nextSeqNo(_syn.seqno);

	if (trace) tracefile << "SERVER: Received SYN with Seq No " << _syn.seqno << "\n";
	
	// Complete the connection handshake.
		// Send a SYNACK
	synack(_synack, _syn);
	if (trace) tracefile << "SERVER: Sending SYNACK with Seq No " << _synack.seqno << "\n";
	send_dgram(_synack);

	if (trace) tracefile << "SERVER: Waiting for acknowledgement of SYNACK\n";
	while (true) {

		// Wait for ACK
		int recvbytes = recv_dgram(_ack);
		if (recvbytes == SOCKET_ERROR) {
			if (WSAGetLastError() == WSAETIMEDOUT) {
				// Timed out, re-send packet
				if (trace) tracefile << "SERVER: Timed-out, re-sending SYNACK\n";
				send_dgram(_synack);
				continue;
			}
			// Some other error occured
			traceError(WSAGetLastError(), "SOCKET_ERROR while recv");
			throw new SocketException("Could not receive SYNACK from client!");
		}
		if (recvbytes < sizeof(dgram)) continue; // Throw away unexpected packet
		if (_ack.type != ACK) continue; //Throw away unexpected packet
		if (_ack.seqno != _synack.seqno) {
			continue; // This ACK is not for our SYNACK!
		}

		if (trace) tracefile << "SERVER: Received ACK with Seq No " << _ack.seqno << "\n";
		break;
	}

	std::cout << "Accepted connection from " << inet_ntoa(dest.sin_addr) << ":"
		 << std::hex << htons(dest.sin_port) << std::dec << std::endl;

	this_seqno = nextSeqNo(_synack.seqno);

	if (trace) {
		tracefile << "SERVER: Received SYNACK acknowledgement, connection established!\n";
		tracefile << "Next sequence numbers will be:\n";
		tracefile << "  Client: " << dest_seqno << "\n";
		tracefile << "  Server: " << this_seqno << "\n";
	}
}

void net::Socket::traceError(int err, char *msg) {
	LPSTR errString;
	int errStringSize = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		0,
		err,
		0,
		(LPSTR)&errString,
		0,
		0);
	if (trace) tracefile << msg << ": " << errString << std::endl;
	LocalFree(errString);
}