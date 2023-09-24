#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winsock2.h>

class NetSocket
{
protected:
    SOCKET sock;
    sockaddr_in addr;
    int addrLen;
    HANDLE hPipe;

public:
    UINT16 id;

    NetSocket(UINT _id) : id(_id) {}

    virtual int Send(const char* data, int len) = 0;
    virtual int Recv(char* data, int len) = 0;
    virtual void Close() = 0;
};

class NetSocketTCP : public NetSocket
{
public:
    NetSocketTCP(UINT _id, USHORT _port, ULONG _addr = INADDR_ANY);

    bool isConnected();

    int Send(const char* data, int len);

    int Recv(char* data, int len);

    void Close();
};

class NetSocketUDP : public NetSocket
{
public:
    NetSocketUDP(UINT _id, USHORT _port, ULONG _addr = INADDR_ANY);

    int Send(const char* data, int len);

    int Recv(char* data, int len);

    void Close();
};