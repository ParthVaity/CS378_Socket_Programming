#include"socket_util.hpp"
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
using namespace std;

int createTCPSocket() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); exit(1); }
    return fd;
}

int createUDPSocket() {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) { perror("socket"); exit(1); }
    return fd;
}

sockaddr_in makeAddr(const char *ip, int port) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1) {
        cerr << "Bad IP " << ip << "\n";
        exit(1);
    }
    return addr;
}

void bindSocket(int fd, const sockaddr_in& addr) {
    if (bind(fd, (const sockaddr*)&addr , sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }
}

void connectToServer(int fd, const sockaddr_in& addr) {
    if (connect(fd, (const sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        exit(1);
    }
}

bool recvAll(int fd, uint8_t* buf, int n) {
    int got = 0;
    while (got < n) {
        int r = recv(fd, buf+got, n-got, 0);
        if (r <= 0) return false;
        got += r;
    }
    return true;
}

bool sendAll(int fd, const uint8_t* buf, int n) {
    int sent = 0;
    while (sent < n) {
        int s = send(fd, buf+sent, n-sent, MSG_NOSIGNAL);
        if (s <= 0) return false;
        sent += s;
    }
    return true;
}

void closeSocket(int fd) {
    close(fd);
}