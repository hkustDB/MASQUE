#include "SortingNetwork.h"

void GenBitonicMerger(std::vector<share*> &keys, std::vector<share*> &vals, uint32_t pos, uint32_t len, bool direction, Circuit* circ) {
    if (len > 1) {
        uint32_t halflen = len / 2;
        for (auto i=pos; i<pos + halflen; ++i) {
            share* selbit = new boolshare(1, circ);
            if (direction) {
                selbit = circ -> PutGTGate(keys[i], keys[i + halflen]);
            } else {
                selbit = circ -> PutGTGate(keys[i + halflen], keys[i]);
            }
            share** swapkeys = circ -> PutCondSwapGate(keys[i], keys[i + halflen], selbit, TRUE);
            keys[i] = swapkeys[0]; keys[i + halflen] = swapkeys[1];
            share** swapvals = circ -> PutCondSwapGate(vals[i], vals[i + halflen], selbit, TRUE);
            vals[i] = swapvals[0]; vals[i + halflen] = swapvals[1];
        }
        GenBitonicMerger(keys, vals, pos, halflen, direction, circ);
        GenBitonicMerger(keys, vals, pos + halflen, halflen, direction, circ);
    }
    return;
}

void GenBitonicSort(std::vector<share*> &keys, std::vector<share*> &vals, uint32_t pos, uint32_t len, bool direction, Circuit* circ) {
    if (len > 1) {
        uint32_t halflen = len / 2;
        GenBitonicSort(keys, vals, pos, halflen, TRUE, circ);
        GenBitonicSort(keys, vals, pos + halflen, halflen, FALSE, circ);
        GenBitonicMerger(keys, vals, pos, len, direction, circ);
    }
    return;
}

void SortingNetwork(std::vector<uint64_t> keys, std::vector<std::vector<uint32_t>> &vals, e_role role, bool direction, e_sharing keytype, e_sharing intype, e_sharing outtype) {
    uint32_t neles = vals.size();
    assert(neles == keys.size());
    uint32_t nattrs = vals[0].size();

    uint32_t oldneles = neles;
    // extend to power of 2
    neles = (uint32_t)pow(2, ceil(log2(neles)));
    keys.resize(neles);
    if (role == SERVER) {
        for (auto i=oldneles; i<neles; ++i) {
            keys[i] = (uint64_t)-1;
        }
    }
    vals.resize(neles);
    for (auto i=oldneles; i<neles; ++i) {
        vals[i].resize(nattrs);
    }

    std::vector<share*> sharekeys(neles), sharevals(neles), outvals(neles);

    Circuit* ac = gParty.GetCircuit(S_ARITH);
    Circuit* bc = gParty.GetCircuit(S_BOOL);
    Circuit* yc = gParty.GetCircuit(S_YAO);

    if (keytype == S_BOOL) {
        for (auto i=0U; i<neles; ++i) {
            sharekeys[i] = bc -> PutSharedINGate(keys[i], 64);
        }
    } else {
        for (auto i=0U; i<neles; ++i) {
            sharekeys[i] = ac -> PutSharedINGate(keys[i], 32);
            sharekeys[i] = bc -> PutA2BGate(sharekeys[i], yc);
        }
    }

    if (intype == S_BOOL) {
        for (auto i=0U; i<neles; ++i) {
            sharevals[i] = bc -> PutSharedSIMDINGate(nattrs, (uint32_t*)vals[i].data(), 32);
        }
    } else {
        for (auto i=0U; i<neles; ++i) {
            sharevals[i] = ac -> PutSharedSIMDINGate(nattrs, (uint32_t*)vals[i].data(), 32);
            sharevals[i] = bc -> PutA2BGate(sharevals[i], yc);
        }
    }

    GenBitonicSort(sharekeys, sharevals, 0U, neles, direction, bc);

    if (outtype == S_BOOL) {
        for (auto i=0U; i<neles; ++i) {
            outvals[i] = bc -> PutOUTGate(sharevals[i], ALL);
        }
    } else {
        for (auto i=0U; i<neles; ++i) {
            outvals[i] = ac -> PutB2AGate(sharevals[i]);
            outvals[i] = ac -> PutOUTGate(outvals[i], ALL);
        }
    }

    gParty.ExecCircuit();
    
    uint32_t nvals, bitlen, *tuples;
    for (auto i=0; i<oldneles; ++i) {
        outvals[i] -> get_clear_value_vec(&tuples, &bitlen, &nvals);
        assert(bitlen == 32);
        assert(nvals == nattrs);
        for (auto j=0; j<nattrs; ++j) {
            vals[i][j] = tuples[j];
        }
    }
    vals.resize(oldneles);
    return;
}