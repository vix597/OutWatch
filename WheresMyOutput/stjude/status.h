#pragma once

struct Status{
	Status() :
	success(false),
	errorString(),
	errorCode(0)
	{}
	Status(bool s, std::string es, int ec) :
		success(s),
		errorString(es),
		errorCode(ec)
	{}
	bool success;
	std::string errorString;
	int errorCode;
};