#include "../core/Utils.h"
#include "../core/Party.h"
#include "../core/PermutationNetwork.h"
#include "../core/Sort/SortingNetwork.h"
#include "../core/Sort/OptSortingNetwork.h"

#include "abycore/sharing/sharing.h"
#include "abycore/circuit/booleancircuits.h"
#include "abycore/circuit/booleancircuits.h"
#include "abycore/circuit/arithmeticcircuits.h"
#include "abycore/circuit/circuit.h"
#include "abycore/aby/abyparty.h"

#include <vector>

using namespace std;

void TestMultiWires(CircSMPContext context) {
    gParty.Init(context.address, context.port, context.role);

    std::vector<uint32_t> values = {1};
    auto circ = gParty.GetCircuit(S_BOOL);
    
    share* share_a = circ -> PutSIMDINGate(values.size(), (uint32_t*) values.data(), 32, SERVER);
    share* share_b = circ -> PutSIMDCONSGate(values.size(), (uint32_t)3, 32);
    share* share_comp = circ -> PutGTGate(share_a, share_b);
    // share* share_max = circ -> PutMUXGate(share_a, share_b, share_comp);
    // share* outwires = circ -> PutOUTGate(share_max, ALL);
    share** share_swap = circ -> PutCondSwapGate(share_a, share_b, share_comp, TRUE);
    // share* out_a = circ -> PutOUTGate(share_swap[0], ALL);
    // share* out_b = circ -> PutOUTGate(share_swap[1], ALL);

    // share* temp1 = circ -> PutCombineAtPosGate(share_a, 0);
    // share* temp2 = circ -> PutSplitterGate(temp1);
    // share* tempout = circ -> PutOUTGate(temp2, ALL);

    gParty.ExecCircuit();

    // uint32_t outsize, outbitlen, *outs;
    // out_b->get_clear_value_vec(&outs, &outbitlen, &outsize);

    // for (auto i=0U; i<outsize; ++i) {
    //     std::cout << outs[i] << ' ';
    // }
    // std::cout << std::endl;

    // std::cout << tempout -> get_clear_value<uint32_t>() << std::endl;

    return;
}

std::vector<bool> selbits;
uint32_t pos;
void PermNet(std::vector<uint32_t> &vals) {
    uint32_t neles = vals.size();
    if (neles <= 1) return;
    uint32_t upper = neles / 2;
    std::vector<uint32_t> uppervals, lowervals;

    for (auto i=0U; i<upper; ++i) {
        if (selbits[pos+i] == 0) {
            uppervals.push_back(vals[i+i]);
            lowervals.push_back(vals[i+i+1]);
        } else {
            uppervals.push_back(vals[i+i+1]);
            lowervals.push_back(vals[i+i]);
        }
    }
    if (neles & 1) {
        lowervals.push_back(vals[neles-1]);
    }
    pos += upper;
    PermNet(uppervals);
    PermNet(lowervals);
    for (auto i=0U; i<(neles - 1)/2; ++i) {
        if (selbits[pos + i] == 0) {
            vals[i+i] = uppervals[i];
            vals[i+i+1] = lowervals[i];
        } else {
            vals[i+i+1] = uppervals[i];
            vals[i+i] = lowervals[i];
        }
    }
    pos += (neles - 1) / 2;
    if (neles & 1) {
        vals[neles - 1] = lowervals[upper];
    } else {
        vals[neles - 2] = uppervals[upper - 1];
        vals[neles - 1] = lowervals[upper - 1];
    }
    return;
}

void TestPermNetSB() {
    std::vector<uint32_t> indices = {7,6,5,8,0,3,2,1,4};

    GetSelectionBits(indices, selbits);
    for (auto i:selbits) {
        std::cout << i << ' ';
    }
    std::cout << std::endl;

    std::vector<uint32_t> vals = {0,1,2,3,4,5,6,7,8};
    pos = 0;
    PermNet(vals);
    for (auto i:vals) {
        std::cout << i << ' ';
    }
    std::cout << std::endl;
}

void TestPermNet(CircSMPContext context) {
    gParty.Init(context.address, context.port, context.role);
    std::vector<uint32_t> values = {1,2,3,4,5,6,7,8,9};
    if (context.role == CLIENT) {
        values = {0,0,0,0,0,0,0,0,0};
    }
    std::vector<uint32_t> indices = {7,6,5,8,0,3,2,1,4};
    PermutationNetwork(values, indices, SERVER, context.role);
    for (auto i=0U; i<values.size(); ++i) {
        cout << values[i] << ' ';
    }
    cout << endl;
}

void TestPermNet2(CircSMPContext context) {
    gParty.Init(context.address, context.port, context.role);
    uint32_t neles = 9;
    std::vector<std::vector<uint32_t>> tuples (neles);
    for (auto i=0U; i<neles; ++i) {
        uint32_t j = i;
        if (context.role == CLIENT) j=1;
        tuples[i] = {j, j*11, j*111, j*1111, j*11111, j*111111, j*1111111, j*11111111};
    }
    std::vector<uint32_t> indices = {7,6,5,8,0,3,2,1,4};
    PermutationNetwork(tuples, indices, SERVER, context.role, S_ARITH, S_ARITH);
    for (auto i=0U; i<tuples.size(); ++i) {
        for (auto j:tuples[i]) {
            cout << j << ',' ;
        }
        cout << endl;
    }
    cout << endl;
}

void TestOptSortNet(CircSMPContext context) {
    gParty.Init(context.address, context.port, context.role);
    uint32_t neles = 4000;
    std::vector<uint64_t> keys (neles);
    if (context.role == SERVER) {
        for (auto i=0U; i<neles; ++i) {
            keys[i] = neles - i;
        }
    }
    std::vector<std::vector<uint32_t>> vals (neles), outs;
    for (auto i=0U; i<neles; ++i) {
        uint32_t j = keys[i];
        keys[i] = keys[i] * 10000000000000ULL;
        vals[i] = {j, j*11, j*111, j*1111, j*11111, j*111111, j*1111111, j*11111111};
    }
    SortNet sn;
    sn.LoadData(keys, vals);
    sn.ExecSortNet(context);
    sn.OutputData(outs);
    RevealResult(outs, context);
    PrintInfo(context);
}

void TestSortNet(CircSMPContext context) {
    gParty.Init(context.address, context.port, context.role);
    uint32_t neles = 4000;
    std::vector<uint64_t> keys (neles);
    if (context.role == SERVER) {
        for (auto i=0U; i<neles; ++i) {
            keys[i] = neles - i;
        }
    }
    std::vector<std::vector<uint32_t>> vals (neles);
    for (auto i=0U; i<neles; ++i) {
        uint32_t j = keys[i];
        keys[i] = keys[i] * 10000000000000ULL;
        vals[i] = {j, j*11, j*111, j*1111, j*11111, j*111111, j*1111111, j*11111111};
    }
    SortingNetwork(keys, vals, context.role);
    // for (auto i=0U; i<vals.size(); ++i) {
    //     for (auto j:vals[i]) {
    //         cout << j << ',' ;
    //     }
    //     cout << endl;
    // }
    // cout << endl;
    PrintInfo(context);
}

int main(int argc, char ** argv) {
    auto context = read_options(argc, argv);
    // std::cerr << context.address << ' ' << context.port << ' ' << context.role << std::endl;

    TestMultiWires(context);

    // TestPermNetSB();
    // TestPermNet(context);
    // TestPermNet2(context);

    // TestSortNet(context);
    // TestOptSortNet(context);

    return 0;
}