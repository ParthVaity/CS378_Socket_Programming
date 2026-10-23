#pragma once

#include<map>
#include<vector>
#include"common.hpp"

class NeighbourTable {
    std::map<char, Neighbour>       idToNeighbour;

public:
    void                            remake(const std::vector<Tuple> &linkState);
    const std::map<char, Neighbour> &allNeighbours() const {return idToNeighbour;}
};