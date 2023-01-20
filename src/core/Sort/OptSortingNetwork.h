#pragma once

#include "../Party.h"
#include "../Utils.h"

#include "sharing/sharing.h"
#include "circuit/booleancircuits.h"
#include "circuit/booleancircuits.h"
#include "circuit/arithmeticcircuits.h"
#include "circuit/circuit.h"
#include "aby/abyparty.h"

class SortNet {
public:
    SortNet();
    void LoadData(std::vector<uint64_t> keys, std::vector<std::vector<uint32_t>> vals);
    void BitonicMerge(uint32_t pos, uint32_t len, bool direction);
    void BitonicSort(uint32_t pos, uint32_t len, bool direction);
    void ExecSortNet(CircSMPContext &context);
    void OutputData(std::vector<std::vector<uint32_t>> &res);
private:
    Circuit *ac, *bc, *yc;
    uint32_t nsort, neles, nattrs;
    std::vector<share*> shr_keys, shr_vals, outs;
};