#include "OptSortingNetwork.h"

SortNet::SortNet() {
    gParty.Reset();
    ac = gParty.GetCircuit(S_ARITH);
    bc = gParty.GetCircuit(S_BOOL);
    yc = gParty.GetCircuit(S_YAO);
}

void SortNet::LoadData(std::vector<uint64_t> keys, std::vector<std::vector<uint32_t>> vals) {
    neles = keys.size();
    assert(neles = vals.size());
    nattrs = vals[0].size();
    nsort = (uint32_t) pow(2, ceil(log2(neles)));

    shr_keys.resize(nsort);
    shr_vals.resize(nsort);
    for (auto i=0u; i<neles; ++i) {
        shr_keys[i] = bc->PutSharedINGate(keys[i], 64);
        shr_vals[i] = bc->PutSharedSIMDINGate(nattrs, (uint32_t*)vals[i].data(), 32);
    }
    for (auto i=neles; i<nsort; ++i) { // dummy elements
        shr_keys[i] = bc->PutINGate((uint64_t)-1, 64, SERVER);
        shr_vals[i] = bc->PutSIMDINGate(nattrs, (uint32_t*)vals[0].data(), 32, SERVER);
    }
}

void SortNet::BitonicMerge(uint32_t pos, uint32_t len, bool direction) {
    if (len > 1) {
        uint32_t halflen = len / 2;
        for (auto i=pos; i<pos + halflen; ++i) {
            share* selbit = new boolshare(1, bc);
            if (direction) {
                selbit = bc -> PutGTGate(shr_keys[i], shr_keys[i + halflen]);
            } else {
                selbit = bc -> PutGTGate(shr_keys[i + halflen], shr_keys[i]);
            }
            share** swapkeys = bc -> PutCondSwapGate(shr_keys[i], shr_keys[i + halflen], selbit, TRUE);
            shr_keys[i] = swapkeys[0]; shr_keys[i + halflen] = swapkeys[1];
            share** swapvals = bc -> PutCondSwapGate(shr_vals[i], shr_vals[i + halflen], selbit, TRUE);
            shr_vals[i] = swapvals[0]; shr_vals[i + halflen] = swapvals[1];
        }
        BitonicMerge(pos, halflen, direction);
        BitonicMerge(pos + halflen, halflen, direction);
    }
    return;
}

void SortNet::BitonicSort(uint32_t pos, uint32_t len, bool direction) {
    if (len > 1) {
        uint32_t halflen = len / 2;
        BitonicSort(pos, halflen, TRUE);
        BitonicSort(pos + halflen, halflen, FALSE);
        BitonicMerge(pos, len, direction);
    }
}

void SortNet::ExecSortNet(CircSMPContext &context) {
    BitonicSort(0, nsort, TRUE);
    outs.resize(neles);
    for (auto i=0u; i<neles; ++i) {
        outs[i] = bc->PutSharedOUTGate(shr_vals[i]);
    }
    gParty.ExecCircuit();
    gParty.Update(context);
}

void SortNet::OutputData(std::vector<std::vector<uint32_t>> &res) {
    uint32_t nvals, bitlen, *tuples;
    res.resize(neles);
    for (auto i=0u; i<neles; ++i) {
        outs[i] -> get_clear_value_vec(&tuples, &bitlen, &nvals);
        assert(bitlen == 32);
        assert(nvals == nattrs);
        res[i].resize(nattrs);
        for (auto j=0; j<nattrs; ++j) {
            res[i][j] = tuples[j];
        }
    }
    return;
}