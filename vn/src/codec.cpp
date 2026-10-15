#include"codec.hpp"
#include"socket_util.hpp"
#include<cstring>
#include<arpa/inet.h>
using namespace std;

static void encodeTuple(vector<uint8_t>& buf, const Tuple& tup) {
    uint32_t c = htonl(tup.cost);
    buf.push_back(tup.id);
    buf.insert(buf.end(), (const uint8_t*)&tup.ip, (const uint8_t*)&tup.ip + 4);
    buf.insert(buf.end(), (const uint8_t*)&tup.port, (const uint8_t*)&tup.port + 2);
    buf.insert(buf.end(), (const uint8_t*)&c, (const uint8_t*)&c + 4);
}

static bool decodeTuple(const uint8_t* buf, Tuple& tup) {
    if (buf[0] < 'A' || buf[0] > 'Z') return false;
    uint32_t c;
    tup.id = buf[0];
    memcpy(&tup.ip, buf+1, 4);
    memcpy(&tup.port, buf+5, 2);
    memcpy(&c, buf+7, 4);
    tup.cost = ntohl(c);
    return true;
}

vector<uint8_t> encodeConnect(const sockaddr_in& mine) {
    vector<uint8_t> buf(6);
    memcpy(buf.data(), &mine.sin_addr.s_addr, 4);
    memcpy(buf.data()+4, &mine.sin_port, 2);
    return buf;
}

vector<uint8_t> encodeLSP(const Lsp& lsp) {
    vector<uint8_t> buf;
    uint32_t seq = htonl(lsp.seq);
    buf.push_back('A' + lsp.origin);
    buf.insert(buf.end(), (uint8_t*)&seq, (uint8_t*)&seq + 4);
    buf.push_back((uint8_t)lsp.tuples.size());
    for (auto& tup:lsp.tuples) encodeTuple(buf, tup);
    return buf;
}

bool decodeLSP(const uint8_t* buf, int len, Lsp& lsp) {
    if (len < HDR) return false;
    int cnt = buf[5];
    if (buf[0] < 'A' || buf[0] > 'Z') return false;
    if (cnt > MAXN || len < HDR + cnt * ENTRY_SZ) return false;
    uint32_t seq;
    memcpy(&seq, buf+1, 4);
    lsp.origin = buf[0] - 'A';
    lsp.seq = ntohl(seq);
    lsp.tuples.clear();
    for (int i = 0; i < cnt; i++) {
        Tuple tup;
        if (!decodeTuple(buf + HDR + i * ENTRY_SZ, tup)) return false;
        lsp.tuples.push_back(tup);
    }
    return true;
}

bool recvLinkState(int tcp, std::vector<Tuple>& out) {
    uint8_t cnt;
    uint8_t buf[MAXN * ENTRY_SZ];
    if (!recvAll(tcp, &cnt, 1)) return false;
    if (cnt > MAXN) return false;
    if (!recvAll(tcp, buf, cnt * ENTRY_SZ)) return false;
    out.clear();
    for (int i = 0; i < cnt; i++) {
        Tuple tup;
        if (!decodeTuple(buf + i * ENTRY_SZ, tup)) return false;
        out.push_back(tup);
    }
    return true;
}