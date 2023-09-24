#define WINDOWS_LEAN_AND_MEAN
#pragma comment(lib, "ws2_32.lib")
#include <Windows.h>
#include <winsock.h>

class NetSocket
{
protected:
  SOCKET sock;
  sockaddr_in addr;
  int addrLen;

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
  NetSocketTCP(UINT _id, USHORT _port, ULONG _addr = INADDR_ANY) : NetSocket(_id)
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

  bool isConnected() {
    return accept(sock, NULL, NULL) != INVALID_SOCKET;
  }

  int Send(const char* data, int len)
  {
    return send(sock, data, len, 0);
  }

  int Recv(char* data, int len)
  {
    return recv(sock, data, len, 0);
  }

  void Close()
  {
    closesocket(sock);
  }
};

class NetSocketUDP : public NetSocket
{
public:
  NetSocketUDP(UINT _id, USHORT _port, ULONG _addr = INADDR_ANY) : NetSocket(_id)
  {
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = _addr;
    addr.sin_port = _port;
    addrLen = sizeof(addr);

    bind(sock, (sockaddr*)&addr, addrLen);
    getsockname(sock, (sockaddr*)&addr, &addrLen);
  }

  int Send(const char* data, int len)
  {
    return sendto(sock, data, len, 0, (sockaddr*)&addr, addrLen);
  }

  int Recv(char* data, int len)
  {
    return recvfrom(sock, data, len, 0, (sockaddr*)&addr, &addrLen);
  }

  void Close()
  {
    closesocket(sock);
  }
};