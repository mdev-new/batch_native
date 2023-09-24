#define WIN32_LEAN_AND_MEAN
#pragma comment(lib, "ws2_32.lib")
#include <Windows.h>
#include <winsock2.h>

#include "netsock.hpp"

NetSocketTCP::NetSocketTCP(UINT _id, USHORT _port, ULONG _addr = INADDR_ANY) : NetSocket(_id)
{
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = _addr;
    addr.sin_port = _port;
    addrLen = sizeof(addr);

    bind(sock, (sockaddr*)&addr, addrLen);
    getsockname(sock, (sockaddr*)&addr, &addrLen);
    listen(sock, 1);
}

bool NetSocketTCP::isConnected() {
    return accept(sock, NULL, NULL) != INVALID_SOCKET;
}

int NetSocketTCP::Send(const char* data, int len)
{
    return send(sock, data, len, 0);
}

int NetSocketTCP::Recv(char* data, int len)
{
    return recv(sock, data, len, 0);
}

void NetSocketTCP::Close()
{
    closesocket(sock);
}

NetSocketUDP::NetSocketUDP(UINT _id, USHORT _port, ULONG _addr = INADDR_ANY) : NetSocket(_id)
{
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = _addr;
    addr.sin_port = _port;
    addrLen = sizeof(addr);

    bind(sock, (sockaddr*)&addr, addrLen);
    getsockname(sock, (sockaddr*)&addr, &addrLen);
}

int NetSocketUDP::Send(const char* data, int len)
{
    return sendto(sock, data, len, 0, (sockaddr*)&addr, addrLen);
}

int NetSocketUDP::Recv(char* data, int len)
{
    return recvfrom(sock, data, len, 0, (sockaddr*)&addr, &addrLen);
}

void NetSocketUDP::Close()
{
    closesocket(sock);
}
