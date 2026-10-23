#include"neighbour_table.hpp"
using namespace std;

void NeighbourTable::remake(const vector<Tuple> &linkState) {
    idToNeighbour.clear();
    for(auto& tup:linkState) {
        if (tup.cost == 0) continue;
        Neighbour n{};
        n.id = tup.id;
        n.cost = tup.cost;
        n.addr.sin_family = AF_INET;
        n.addr.sin_addr.s_addr = tup.ip;
        n.addr.sin_port = tup.port;
        idToNeighbour[tup.id] = n;
    }
}