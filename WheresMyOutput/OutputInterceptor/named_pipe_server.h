#pragma once

#include <Windows.h>
#include <thread>
#include <mutex>
#include <stdint.h>
#include <iostream>

#define PIPE_SERVER_NAME "\\\\.\\pipe\\StJudeOutput"
#define BUFFER_SIZE 32768

class NamedPipeServer{
private:
	std::thread serverThread;
	HANDLE hServerStopEvent;
	HANDLE hCurrentConnection;
private:
	void DoWork();
public:
	NamedPipeServer();
	~NamedPipeServer();
	void SendData(uint64_t length, BYTE * data);
};