#include "named_pipe_server.h"

using namespace std;

NamedPipeServer::NamedPipeServer()
{
	hServerStopEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
	if (!hServerStopEvent)
		throw runtime_error("Could not create worker thread stop event");
	serverThread = thread(&NamedPipeServer::DoWork, this);
}

NamedPipeServer::~NamedPipeServer()
{
	SetEvent(hServerStopEvent);

	CancelSynchronousIo((HANDLE)serverThread.native_handle());

	if (serverThread.joinable())
		serverThread.join();

	CloseHandle(hServerStopEvent);
}

void NamedPipeServer::DoWork()
{
	HANDLE currentPipe = NULL;
	BOOL connected = FALSE;
	DWORD error = 0;

	while (WaitForSingleObject(hServerStopEvent, 0) != WAIT_OBJECT_0)
	{
		if (!currentPipe){
			currentPipe = CreateNamedPipeA(
				PIPE_SERVER_NAME,
				PIPE_ACCESS_OUTBOUND,
				PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE,
				1,
				BUFFER_SIZE,
				BUFFER_SIZE,
				0,
				NULL
			);
			if (currentPipe == INVALID_HANDLE_VALUE){
				OutputDebugStringA("Create named pipe failed\n");
				return;
			}
		}

		OutputDebugStringA("Waiting for connection\n");
		connected = ConnectNamedPipe(currentPipe, NULL);
		error = GetLastError();
	
		if (connected || error == ERROR_PIPE_CONNECTED){
			OutputDebugStringA("Connected\n");
			
			connectionMtx.lock();
			hCurrentConnection = currentPipe;
			connectionMtx.unlock();

			WaitNamedPipeA(PIPE_SERVER_NAME, NMPWAIT_WAIT_FOREVER);

			connectionMtx.lock();
			FlushFileBuffers(hCurrentConnection);
			DisconnectNamedPipe(hCurrentConnection);
			CloseHandle(hCurrentConnection);
			hCurrentConnection = NULL;
			currentPipe = NULL;
			connectionMtx.unlock();
		}
		else if (error == ERROR_NO_DATA){
			OutputDebugStringA("Client disconnect\n");
		}
		else{
			OutputDebugStringA("Client failed to connect\n");
		}
	}
}

void NamedPipeServer::SendData(uint64_t length, BYTE * data)
{
	DWORD bytesSent = 0;
	uint64_t bytesLeft = 0;
	uint64_t bytesToSend = 0;
	uint64_t totalBytesSent = 0;
	
	lock_guard<mutex> lock(connectionMtx);

	if (!hCurrentConnection || hCurrentConnection == INVALID_HANDLE_VALUE)
		return;

	while (totalBytesSent < length){
		bytesLeft = length - totalBytesSent;
		bytesToSend = (bytesLeft > BUFFER_SIZE) ? BUFFER_SIZE : bytesLeft;

		if (!WriteFile(hCurrentConnection, &data[totalBytesSent], (DWORD)bytesToSend, &bytesSent, NULL)){
			return;
		}
		totalBytesSent += bytesSent;
	}
}