#pragma once
class IncompleteSendException
{
public:
	size_t bytes_sent;
	size_t expected_bytes_sent;
	IncompleteSendException(size_t bytes_sent, size_t expected_bytes_sent);
	~IncompleteSendException();
};

