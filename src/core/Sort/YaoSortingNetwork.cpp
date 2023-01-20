#include "YaoSortingNetwork.h"

YaoSortNet::YaoSortNet() {
    gParty.Reset();
    ac = gParty.GetCircuit(S_ARITH);
    bc = gParty.GetCircuit(S_BOOL);
    yc = gParty.GetCircuit(S_YAO);
}

void YaoSortNet::LoadData(std::vector<uint64_t> keys, std::vector<std::vector<uint32_t>> vals) {
    neles = keys.size();
    assert(neles = vals.size());
    nattrs = vals[0].size();
    nsort = (uint32_t) pow(2, ceil(log2(neles)));

    shr_keys.resize(nsort);
    shr_vals.resize(nsort);
    for (auto i=0u; i<neles; ++i) {
        shr_keys[i] = bc->PutSharedINGate(keys[i], 64);
        shr_keys[i] = yc->PutB2YGate(shr_keys[i]);
        shr_vals[i] = bc->PutSharedSIMDINGate(nattrs, (uint32_t*)vals[i].data(), 32);
        shr_vals[i] = yc->PutB2YGate(shr_vals[i]);
    }
    for (auto i=neles; i<nsort; ++i) { // dummy elements
        shr_keys[i] = yc->PutINGate((uint64_t)-1, 64, SERVER);
        shr_vals[i] = yc->PutSIMDINGate(nattrs, (uint32_t*)vals[0].data(), 32, SERVER);
    }
}

void YaoSortNet::BitonicMerge(uint32_t pos, uint32_t len, bool direction) {
    if (len > 1) {
        uint32_t halflen = len / 2;
        for (auto i=pos; i<pos + halflen; ++i) {
            share* selbit = new boolshare(1, bc);
            if (direction) {
                selbit = yc -> PutGTGate(shr_keys[i], shr_keys[i + halflen]);
            } else {
                selbit = yc -> PutGTGate(shr_keys[i + halflen], shr_keys[i]);
            }
            share** swapkeys = yc -> PutCondSwapGate(shr_keys[i], shr_keys[i + halflen], selbit, TRUE);
            shr_keys[i] = swapkeys[0]; shr_keys[i + halflen] = swapkeys[1];
            share** swapvals = yc -> PutCondSwapGate(shr_vals[i], shr_vals[i + halflen], selbit, TRUE);
            shr_vals[i] = swapvals[0]; shr_vals[i + halflen] = swapvals[1];
        }
        BitonicMerge(pos, halflen, direction);
        BitonicMerge(pos + halflen, halflen, direction);
    }
    return;
}

void YaoSortNet::BitonicSort(uint32_t pos, uint32_t len, bool direction) {
    if (len > 1) {
        uint32_t halflen = len / 2;
        BitonicSort(pos, halflen, TRUE);
        BitonicSort(pos + halflen, halflen, FALSE);
        BitonicMerge(pos, len, direction);
    }
}

void YaoSortNet::ExecSortNet(CircSMPContext &context) {
    BitonicSort(0, nsort, TRUE);
    outs.resize(neles);
    for (auto i=0u; i<neles; ++i) {
        shr_vals[i] = bc->PutY2BGate(shr_vals[i]);
        outs[i] = bc->PutSharedOUTGate(shr_vals[i]);
    }
    gParty.ExecCircuit();
    gParty.Update(context);
}

void YaoSortNet::OutputData(std::vector<std::vector<uint32_t>> &res) {
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