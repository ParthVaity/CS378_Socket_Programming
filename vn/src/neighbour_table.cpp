#include"neighbour_table.hpp"
using namespace std;

void NeighbourTable::remake(const vector<Tuple> &linkState) {
    idToNeighbour.clear();
    for(auto& tup:linkState) {
        Neighbour n{};
        n.id = tup.id;
        n.cost = tup.cost;
        n.addr.sin_family = AF_INET;
        n.addr.sin_addr.s_addr = tup.ip;
        n.addr.sin_port = tup.port;
        idToNeighbour[tup.id] = n;
    }
}

bool NeighbourTable::sameAddr(const sockaddr_in &a, const sockaddr_in &b) {
    return a.sin_addr.s_addr == b.sin_addr.s_addr && a.sin_port == b.sin_port;
}