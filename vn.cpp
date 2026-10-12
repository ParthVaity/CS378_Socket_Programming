#include <iostream>
#include <cstring>
#include <cstdint>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
using namespace std;

void recvAll(int fd, uint8_t *buf, int n) {
    int got = 0;
    while (got < n) {
        int r = recv(fd, buf + got, n - got, 0);
        if (r <= 0) { cerr << "connection closed\n"; exit(1); }
        got += r;
    }
}

int main(int argc, char **argv) {
    if (argc != 4) { cerr << "usage: ./vn <ON_IP> <MY_IP> <MY_UDP_PORT>\n"; return 1; }

    sockaddr_in mine{};
    mine.sin_family = AF_INET;
    mine.sin_port = htons(atoi(argv[3]));
    inet_pton(AF_INET, argv[2], &mine.sin_addr);
    int udp = socket(AF_INET, SOCK_DGRAM, 0);
    if (bind(udp, (sockaddr *)&mine, sizeof mine) < 0) { perror("bind udp"); return 1; }

    sockaddr_in on{};
    on.sin_family = AF_INET;
    on.sin_port = htons(5000);
    inet_pton(AF_INET, argv[1], &on.sin_addr);
    int tcp = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(tcp, (sockaddr *)&on, sizeof on) < 0) { perror("connect"); return 1; }

    uint8_t msg[7];
    msg[0] = 1;
    memcpy(msg + 1, &mine.sin_addr.s_addr, 4);
    memcpy(msg + 5, &mine.sin_port, 2);
    send(tcp, msg, 7, 0);

    uint8_t hdr[2];
    recvAll(tcp, hdr, 2);
    int count = hdr[1];
    for (int k = 0; k < count; k++) {
        uint8_t t[11];
        recvAll(tcp, t, 11);
        uint32_t c;
        memcpy(&c, t + 7, 4);
        cout << (k ? "," : "") << (char)t[0] << "=" << ntohl(c);
    }
    cout << endl;

    close(tcp);
    close(udp);
    return 0;
}