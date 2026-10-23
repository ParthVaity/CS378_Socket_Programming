#pragma once

#include<cstdint>
#include<netinet/in.h>

int         createTCPSocket();
int         createUDPSocket();
sockaddr_in makeAddr(const char *ip, int port);
void        bindSocket(int fd, const sockaddr_in& addr);
void        connectToServer(int fd, const sockaddr_in& addr);
bool        recvAll(int fd, uint8_t* buf, int n);
bool        sendAll(int fd, const uint8_t* buf, int n);
void        closeSocket(int fd);
static bool sameAddr(const sockaddr_in &a, const sockaddr_in &b);