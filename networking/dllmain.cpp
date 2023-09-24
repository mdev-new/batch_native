#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <Windows.h>
#include "Injector.h"
#include "netsock.hpp"

#include <vector>
#include <map>
#include <string>

#define MAXLEN 512

#define ENV SetEnvironmentVariable

std::map<UINT, NetSocket*> sockets;

// todo: move to some kind of cpp utilities file

std::vector<std::string> split(const std::string& str, const char delim)
{
  std::vector<std::string> splitted;
  std::string tmp;
  //MessageBox(NULL, "Ass", "Ass1", 0);
  for (char c : str)
  {
    MessageBox(NULL, "Ass", (LPCSTR)(str.c_str()), 0);
    if (c == delim)
    {
      splitted.push_back(tmp);
      tmp.clear();
    }
    else
    {
      tmp += c;
    }
  }
  //MessageBox(NULL, "Ass", "Ass2", 0);
  splitted.push_back(tmp);
  return splitted;
}

DWORD CALLBACK HandleNetOutPipe(LPVOID data)
{
  HANDLE hPipe;
  DWORD dwRead;

  std::string buffer;
  buffer.reserve(MAXLEN);

  hPipe = CreateNamedPipe((LPCSTR)data,
    PIPE_ACCESS_INBOUND,
    PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, // | FILE_FLAG_FIRST_PIPE_INSTANCE,
    1,
    0,
    MAXLEN,
    NMPWAIT_USE_DEFAULT_WAIT,
    NULL);

  while (hPipe != INVALID_HANDLE_VALUE) {
    while (ConnectNamedPipe(hPipe, NULL) != FALSE) {
      // when a socket receives data, it will write it to the pipe
      // which the client can read

      std::map<UINT, std::string> received_data;

      // collect all the data from the sockets
      for (auto& [id, socket] : sockets)
      {
        char* data = new char[MAXLEN];
        int len = socket->Recv(data, MAXLEN);
        if (len > 0)
        {
          received_data[id] = std::string(data, len);
        }
        delete[] data;
      }

      // write the data to the pipe
      for (auto& [id, data] : received_data)
      {
        buffer = std::to_string(id) + ";" + data;
        WriteFile(hPipe, buffer.data(), buffer.length(), &dwRead, NULL);
      }
    }
    DisconnectNamedPipe(hPipe);
  }
  return 0;
}

DWORD CALLBACK HandleNetPipe(LPVOID data)
{

  HANDLE hPipe;
  DWORD dwRead;

  std::string buffer;

  hPipe = CreateNamedPipe((LPCSTR)data,
    PIPE_ACCESS_INBOUND,
    PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, // | FILE_FLAG_FIRST_PIPE_INSTANCE,
    1,
    0,
    MAXLEN,
    NMPWAIT_USE_DEFAULT_WAIT,
    NULL);

  while (hPipe != INVALID_HANDLE_VALUE) {
    while (ConnectNamedPipe(hPipe, NULL) != FALSE) {
      while (ReadFile(hPipe, buffer.data(), MAXLEN - 1, &dwRead, NULL) != FALSE) {
        buffer[dwRead] = '\0';
        std::vector<std::string> split_command = split(buffer, ';');

        if (split_command.size() < 2) continue;
        int socket_id = atoi(split_command[0].c_str());
        if (sockets.find(socket_id) == sockets.end()) continue;

        // join the rest of the strings
        std::string data = "";
        for (int i = 1; i < split_command.size(); i++)
          data += split_command[i];

        sockets[socket_id]->Send(data.c_str(), data.length());
      }
    }
    DisconnectNamedPipe(hPipe);
  }
  return 0;
}

DWORD CALLBACK HandleNetCmd(LPVOID data)
{
  HANDLE hPipe;
  DWORD dwRead;

  std::string buffer;

  hPipe = CreateNamedPipe((LPCSTR)data,
    PIPE_ACCESS_INBOUND,
    PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
    1,
    0,
    MAXLEN,
    NMPWAIT_USE_DEFAULT_WAIT,
    NULL);


  while (hPipe != INVALID_HANDLE_VALUE) {
    while (ConnectNamedPipe(hPipe, NULL) != FALSE) {
      while (ReadFile(hPipe, buffer.data(), MAXLEN - 1, &dwRead, NULL) != FALSE) {
        MessageBox(NULL, "Made", "it here2", 0);
        buffer[dwRead] = '\0';
        MessageBox(NULL, "nun", (LPCSTR)buffer.c_str(), 0);

        std::vector<std::string> split_command = split(buffer, ';');

        /*if (split_command.size() < 2)
        {
        }*/
        MessageBox(NULL, "Made", "it here25", 0);

        // the protocol is the first 3 characters of the first string
        std::string protocol = split_command[0].substr(0, 3);

        if (protocol == "tcp")
        {
          MessageBox(NULL, "Made", "it here234", 0);
          // the rest is the operation
          std::string operation = split_command[0].substr(3, split_command[0].size() - 1);
          if (operation == "create")
          {
            MessageBox(NULL, "Made", "it kurwy", 0);
            UINT id = atoi(split_command[1].c_str());
            USHORT port = atoi(split_command[2].c_str());
            sockets[id] = new NetSocketTCP(id, port);
            MessageBox(NULL, "Made", "it mac", 0);
          }
          else if (operation == "connect")
          {
            UINT id = atoi(split_command[1].c_str());
            std::string addr = split_command[2];
            USHORT port = atoi(split_command[3].c_str());
            ULONG addr_long = inet_addr(addr.c_str());
            sockets[id] = new NetSocketTCP(id, port, addr_long);
          }
          else if (operation == "destroy")
          {
            UINT id = atoi(split_command[1].c_str());
            sockets[id]->Close();
            delete sockets[id];
            sockets.erase(id);
          }
          else if (operation == "status")
          {
            UINT id = atoi(split_command[1].c_str());
            bool stat = ((NetSocketTCP*)sockets[id])->isConnected();
            ENV("batnetstat", (LPCSTR)(stat ? '1' : '0'));
          }
        }
        if (protocol == "udp")
        {
          // the rest is the operation
          std::string operation = split_command[0].substr(3);
          if (operation == "create")
          {
            UINT id = atoi(split_command[1].c_str());
            USHORT port = atoi(split_command[2].c_str());
            std::string addr = split_command[3];
            ULONG addr_long = inet_addr(addr.c_str());
            sockets[id] = new NetSocketUDP(id, port, addr_long);
          }
          else if (operation == "destroy")
          {
            UINT id = atoi(split_command[1].c_str());
            sockets[id]->Close();
            delete sockets[id];
            sockets.erase(id);
          }
        }
      }
    }
    DisconnectNamedPipe(hPipe);
  }
  return 0;
}

BasicDllMainImpl(HandleNetCmd);