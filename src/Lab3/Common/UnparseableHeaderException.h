#pragma once
class UnparseableHeaderException
{
public:
	char *header;
	UnparseableHeaderException(char *header);
	~UnparseableHeaderException();
};

