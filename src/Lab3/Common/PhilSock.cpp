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
		sndr_seqno(0),
		sndr_buf(NULL),
		sndr_len(0),
		recv_seqno(0),
		recv_buf(NULL),
		recv_len(0),
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

void net::Socket::data(dgram &d, int seqNo, size_t sz, const char * payload) {
	d.seqno = seqNo;
	d.type = DATA;
	d.size = sz;
	//Copy payload data
	memcpy((void*)d.payload, payload, sz);
}

void net::Socket::wait(size_t &recv, size_t &sent) {
	dgram sent_pkt, recv_pkt;

	// Initialize to 0
	recv = 0;
	sent = 0;

	// Loop until there is no job left.
	while (sndr_len > 0 || recv_len > 0) {

		if (sndr_len > 0) {
			// We have some outstanding bytes to send.
			size_t pl_size = min(MAX_PAYLOAD_SIZE, sndr_len);
			data(sent_pkt, sndr_seqno, pl_size, sndr_buf);
			if (trace) tracefile << "SENDER: Sending packet #" << sent_pkt.seqno << "\n";
			send_dgram(sent_pkt);
		}

		// Wait for a packet.
		int status = recv_dgram(recv_pkt);
		if (status == SOCKET_ERROR) {
			if (WSAGetLastError() == WSAETIMEDOUT) {
				// Timed out, try sending packet again
				continue;
			}
			// This is another type of error.
			traceError(WSAGetLastError(), "SOCKET_ERROR while waiting for packet.");
			throw new SocketException("Could not receive ACK from other endpoint!");
		}

		// We have something; how we react depends on the type of packet received.
		switch (recv_pkt.type) {
		case SYNACK: 
			// Last ACK of connection handshake was lost. Re-ACK it.
			send_ack(recv_pkt);
			break; // Go back to waiting.
		case ACK: 
			// Were we waiting for one of these?
			if (sndr_len > 0) {
				// Yes. Is this the one we were waiting for?
				if (recv_pkt.seqno == sent_pkt.seqno) {
					if (trace) tracefile << "SENDER: Received ACK for packet #" << recv_pkt.seqno << "\n";
					// Yes. Packet was acknowledged. 
					sndr_seqno = nextSeqNo(sndr_seqno);
					sndr_len -= sent_pkt.size;
					sndr_buf += sent_pkt.size;
					sent += sent_pkt.size;
				}
			}
			// In all other cases, simply discard.
			break;
		case DATA:
			// Were we waiting for one of these?
			if (recv_len > 0) {
				// Yes. Is this the one we were waiting for?
				if (recv_pkt.seqno == recv_seqno) {
					if (trace) tracefile << "RECEIVER: Received expected DATA packet #" << recv_pkt.seqno << "\n";
					// Yes. We have received the expected packet.
					size_t safe_size = min(recv_len, recv_pkt.size); // In case the client sent too many bytes.
					memcpy(recv_buf, recv_pkt.payload, safe_size); // Copy payload bytes into buffer
					recv_seqno = nextSeqNo(recv_seqno);
					recv_len -= safe_size;
					recv_buf += safe_size;
					recv += safe_size;
				}
			}
			if (trace) tracefile << "RECEIVER: Acknowledging packet #" << recv_pkt.seqno << "\n";
			send_ack(recv_pkt); // Acknowledge the packet in all cases.
			break;

		default: break; // Default is to discard packet.
		}
	}
}

size_t net::Socket::send(const char * buf, int len) {
	size_t r, s;

	// Setup sender job.
	sndr_buf = buf;
	sndr_len = len;

	// Block until the job is done
	wait(r, s);
	return s;
}

size_t net::Socket::recv(char * buf, int len) {
	size_t r, s;

	// Setup receiver job
	recv_buf = buf;
	recv_len = len;

	// Block until the job is done
	wait(r, s);
	return r;
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

	sndr_seqno = nextSeqNo(_syn.seqno);

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

		recv_seqno = nextSeqNo(_synack.seqno);

		// Step 3: Send ACK
		if (trace) tracefile << "CLIENT: Sending ACK in response to SYNACK\n";
		// Simulate loss of packet!
		send_ack(_synack);

		break;
	}
	if (trace) {
		tracefile << "CLIENT: Connection established!\n";
		tracefile << "Next sequence numbers will be:\n";
		tracefile << "  Client: " << sndr_seqno << "\n";
		tracefile << "  Server: " << recv_seqno << "\n";
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
	recv_seqno = nextSeqNo(_syn.seqno);

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

	sndr_seqno = nextSeqNo(_synack.seqno);

	if (trace) {
		tracefile << "SERVER: Received SYNACK acknowledgement, connection established!\n";
		tracefile << "Next sequence numbers will be:\n";
		tracefile << "  Client: " << recv_seqno << "\n";
		tracefile << "  Server: " << sndr_seqno << "\n";
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
	if (trace) tracefile << msg << ": Error #" << err << ": " << errString << std::endl;
	LocalFree(errString);
}