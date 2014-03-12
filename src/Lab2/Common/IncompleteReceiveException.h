#pragma once
class IncompleteReceiveException
{
public:
	size_t bytes_sent;
	size_t expected_bytes_sent;
	IncompleteReceiveException(size_t bytes_sent, size_t expected_bytes_sent);
	~IncompleteReceiveException();
};

