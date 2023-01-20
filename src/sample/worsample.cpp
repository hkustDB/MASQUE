#include "worsample.h"

#include <random>
#include <cmath>

void PutMODGate(share* a, share* b, share* &c, Circuit* circ) {
    std::vector<share*> pow2b(32);
    pow2b[0] = b;
    share* zero = circ->PutCONSGate((uint64_t)0, 32);
    share* tag = circ->PutCONSGate((uint64_t)0, 1);
    for (auto i=1u; i<32; ++i) {
        pow2b[i] = circ->PutADDGate(pow2b[i-1], pow2b[i-1]);
        pow2b[i] = circ->PutMUXGate(zero, pow2b[i], tag);
        tag = circ->PutMUXGate(tag, pow2b[i]->get_wire_ids_as_share(31), tag);
    }
    c = a;
    for (int i=31; i>=0; --i) {
        share* selbit = circ->PutGTGate(pow2b[i], c);
        share* sub = circ->PutMUXGate(zero, pow2b[i], selbit);
        c = circ->PutSUBGate(c, sub);
    }
    // circ->PutPrintValueGate(a, "value a");
    // circ->PutPrintValueGate(b, "value b");
    // circ->PutPrintValueGate(c, "value c");
    return;
}

void GenRandomIDs(std::vector<uint32_t> &IDs, uint32_t s, uint32_t d, CircSMPContext &context) {
    auto start_time = clock();
    gParty.Init(context.address, context.port, context.role);
    Circuit* bc = gParty.GetCircuit(S_BOOL);
    Circuit* yc = gParty.GetCircuit(S_YAO);
    share* degree = bc->PutSharedINGate(d, 32);
    degree = yc->PutB2YGate(degree);
    std::vector<share*> shrids(s);
    std::random_device rd;
    std::mt19937 sd(rd());
    std::uniform_int_distribution<uint32_t> unifgen(0, (uint32_t)-1);

    share* one = yc->PutCONSGate((uint64_t)1, 32);
    share* modd = degree;
    for (auto i=0u; i<s; ++i) {
        share* val = bc->PutSharedINGate(unifgen(sd), 32);
        val = yc->PutB2YGate(val);
        PutMODGate(val, modd, shrids[i], yc);
        modd = yc->PutSUBGate(modd, one);
    }
    for (auto i=0u; i<s; ++i) {
        shrids[i] = bc->PutY2BGate(shrids[i]);
        shrids[i] = bc->PutSharedOUTGate(shrids[i]);
    }
    gParty.ExecCircuit();
    gParty.Update(context);

    IDs.resize(s);
    for (auto i=0; i<s; ++i) {
        IDs[i] = shrids[i] -> get_clear_value<uint32_t>();
    }
    gParty.Reset();
    auto end_time = clock();
    context.time_cost = 1.0 * (end_time - start_time) / CLOCKS_PER_SEC;
}

void FKJoin(std::vector<std::vector<uint32_t>> R, std::vector<std::vector<uint32_t>> S, std::vector<std::vector<uint32_t>> &T, CircSMPContext &context) {
    uint32_t reles = R.size(), seles = S.size();
    T.resize(reles);
    
    std::vector<uint64_t> sortkeys(reles + seles);
    std::vector<std::vector<uint32_t>> sorttuples(reles + seles);

    for (auto i=0U; i<seles; ++i) {
        sortkeys[i] = (((uint64_t)S[i][0]) << 32);
        if (context.role == SERVER) {
            sortkeys[i] += i;
        }
        sorttuples[i] = {S[i][0], 0, S[i][1]};
    }
    for (auto i=0U; i<reles; ++i) {
        sortkeys[seles + i] = (((uint64_t)R[i][0]) << 32 );
        if (context.role == SERVER) {
            sortkeys[seles + i] += seles + i;
        }
        sorttuples[seles + i] = {R[i][0], R[i][1], 0};
    }
    ShuffleQSort(sortkeys, sorttuples, context);
    printf("sort result\n");
    RevealResult(sorttuples, context);

    gParty.Init(context.address, context.port, context.role);
    Circuit *bc = gParty.GetCircuit(S_BOOL);
    Circuit *yc = gParty.GetCircuit(S_YAO);
    uint32_t neles = sorttuples.size(), nlevels = (uint32_t)ceil(log2(neles));
    // Prefix sum circuit
    // for simplicity, we use linear scan circuit due to GC
    std::vector<share*> shrin1(neles), shrin2(neles), shrin3(neles), shrtags(neles);
    share* zero = yc->PutINGate(0U, 32, SERVER);

    for (auto i=0U; i<neles; ++i) {
        shrin1[i] = bc->PutSharedINGate(sorttuples[i][0], 32);
        shrin1[i] = yc->PutB2YGate(shrin1[i]);
        shrin2[i] = bc->PutSharedINGate(sorttuples[i][1], 32);
        shrin2[i] = yc->PutB2YGate(shrin2[i]);
        shrin3[i] = bc->PutSharedINGate(sorttuples[i][2], 32);
        shrin3[i] = yc->PutB2YGate(shrin3[i]);

        shrtags[i] = yc->PutEQGate(shrin2[i], zero);
        if (i >= 1) {
            share* selbit = yc->PutEQGate(shrin1[i], shrin1[i-1]);
            shrin3[i] = yc->PutMUXGate(shrin3[i-1], shrin3[i], selbit);
        }
    }

    // Compaction circuit
    std::vector<share*> move(neles);
    move[0] = zero;
    for (auto i=1U; i<neles; ++i) {
        move[i] = yc->PutADDGate(move[i-1], shrtags[i-1]);
    }

    std::vector<std::vector<share*>> compc1(nlevels+1), compc2(nlevels+1);
    compc1[0] = move;
    compc2[0].resize(neles);
    for (auto i=0U; i<neles; ++i) {
        compc2[0][i] = yc->PutCombinerGate(shrin1[i], shrin2[i]);
        compc2[0][i] = yc->PutCombinerGate(compc2[0][i], shrin3[i]);
    }

    for (auto lid=0U; lid<nlevels; ++lid) {
        uint32_t jump = (1U << (lid));
        compc1[lid+1].resize(neles);
        compc2[lid+1].resize(neles);
        for (auto i=0; i<neles; ++i) {
            if (i + jump < neles) {
                share* selbit = compc1[lid][i+jump]->get_wire_ids_as_share(lid);
                share* selbits = yc->PutRepeaterGate(compc2[lid][i]->get_nvals(), selbit);
                compc1[lid+1][i] = yc->PutMUXGate(compc1[lid][i+jump], compc1[lid][i], selbit);
                compc2[lid+1][i] = yc->PutMUXGate(compc2[lid][i+jump], compc2[lid][i], selbits);
            } else {
                compc1[lid+1][i] = compc1[lid][i];
                compc2[lid+1][i] = compc2[lid][i];
            }
        }
    }

    std::vector<share*> shrout(reles);
    for (auto i=0U; i<reles; ++i) {
        shrout[i] = bc->PutY2BGate(compc2[nlevels][i]);
        shrout[i] = bc->PutSharedOUTGate(shrout[i]);
        // yc->PutPrintValueGate(compc2[nlevels][i], "vec");
    }

    // for (auto i=0U; i<neles; ++i) {
    //     shrtags[i] = bc->PutY2BGate(shrtags[i]);
    //     shrtags[i] = bc->PutSharedOUTGate(shrtags[i]);
    //     shrin3[i] = bc->PutY2BGate(shrin3[i]);
    //     shrin3[i] = bc->PutSharedOUTGate(shrin3[i]);
    // }

    gParty.ExecCircuit();

    std::vector<std::vector<uint32_t>> tuples(reles);
    for (auto i=0; i<reles; ++i) {
        uint32_t *vec, bitlen, nvals;
        shrout[i]->get_clear_value_vec(&vec, &bitlen, &nvals);
        // std::cerr << bitlen << ' ' << nvals << std::endl;
        tuples[i].resize(3);
        for (auto j=0U; j<3; ++j) {
            tuples[i][j] = 0;
            for (auto k=0; k<32; ++k) {
                tuples[i][j] += vec[32*j+k] << k;
            }
        }
    }

    RevealResult(tuples, context);

    gParty.Update(context);
    gParty.Reset();
    return;
}

void PointerJumping(std::vector<std::vector<uint32_t>> &L, CircSMPContext &context) {
    std::vector<std::vector<uint32_t>> R = L, S = L, T;
    for (auto i=0u; i<S.size(); ++i) {
        std::swap(S[i][0], S[i][1]);
    }

    uint32_t reles = R.size(), seles = S.size();
    T.resize(reles);
    
    std::vector<uint64_t> sortkeys(reles + seles);
    std::vector<std::vector<uint32_t>> sorttuples(reles + seles);

    for (auto i=0U; i<seles; ++i) {
        sortkeys[i] = (((uint64_t)S[i][0]) << 32);
        if (context.role == SERVER) {
            sortkeys[i] += i;
        }
        sorttuples[i] = {S[i][0], 0, S[i][1]};
    }
    for (auto i=0U; i<reles; ++i) {
        sortkeys[seles + i] = (((uint64_t)R[i][0]) << 32 );
        if (context.role == SERVER) {
            sortkeys[seles + i] += seles + i;
        }
        sorttuples[seles + i] = {R[i][0], R[i][1], 0};
    }
    ShuffleQSort(sortkeys, sorttuples, context);
    // printf("sort result\n");
    // RevealResult(sorttuples, context);

    gParty.Init(context.address, context.port, context.role);
    Circuit *bc = gParty.GetCircuit(S_BOOL);
    Circuit *yc = gParty.GetCircuit(S_YAO);
    uint32_t neles = sorttuples.size(), nlevels = (uint32_t)ceil(log2(neles));
    // Prefix sum circuit
    // for simplicity, we use linear scan circuit due to GC
    std::vector<share*> shrin1(neles), shrin2(neles), shrin3(neles), shrtags(neles);
    share* zero = yc->PutINGate(0U, 32, SERVER);

    for (auto i=0U; i<neles; ++i) {
        shrin1[i] = bc->PutSharedINGate(sorttuples[i][0], 32);
        shrin1[i] = yc->PutB2YGate(shrin1[i]);
        shrin2[i] = bc->PutSharedINGate(sorttuples[i][1], 32);
        shrin2[i] = yc->PutB2YGate(shrin2[i]);
        shrin3[i] = bc->PutSharedINGate(sorttuples[i][2], 32);
        shrin3[i] = yc->PutB2YGate(shrin3[i]);

        shrtags[i] = yc->PutEQGate(shrin2[i], zero);
        if (i >= 1) {
            share* selbit = yc->PutEQGate(shrin1[i], shrin1[i-1]);
            shrin3[i] = yc->PutMUXGate(shrin3[i-1], shrin3[i], selbit);
        }
    }

    // Compaction circuit
    std::vector<share*> move(neles);
    move[0] = zero;
    for (auto i=1U; i<neles; ++i) {
        move[i] = yc->PutADDGate(move[i-1], shrtags[i-1]);
    }

    std::vector<std::vector<share*>> compc1(nlevels+1), compc2(nlevels+1);
    compc1[0] = move;
    compc2[0].resize(neles);
    for (auto i=0U; i<neles; ++i) {
        share* selbits = yc->PutRepeaterGate(32*2, yc->PutEQGate(shrin3[i], zero));
        compc2[0][i] = yc->PutMUXGate(yc->PutCombinerGate(shrin1[i], shrin2[i]), yc->PutCombinerGate(shrin3[i], shrin2[i]), selbits);
    }

    for (auto lid=0U; lid<nlevels; ++lid) {
        uint32_t jump = (1U << (lid));
        compc1[lid+1].resize(neles);
        compc2[lid+1].resize(neles);
        for (auto i=0; i<neles; ++i) {
            if (i + jump < neles) {
                share* selbit = compc1[lid][i+jump]->get_wire_ids_as_share(lid);
                share* selbits = yc->PutRepeaterGate(compc2[lid][i]->get_nvals(), selbit);
                compc1[lid+1][i] = yc->PutMUXGate(compc1[lid][i+jump], compc1[lid][i], selbit);
                compc2[lid+1][i] = yc->PutMUXGate(compc2[lid][i+jump], compc2[lid][i], selbits);
            } else {
                compc1[lid+1][i] = compc1[lid][i];
                compc2[lid+1][i] = compc2[lid][i];
            }
        }
    }

    std::vector<share*> shrout(reles);
    for (auto i=0U; i<reles; ++i) {
        shrout[i] = bc->PutY2BGate(compc2[nlevels][i]);
        shrout[i] = bc->PutSharedOUTGate(shrout[i]);
        // yc->PutPrintValueGate(compc2[nlevels][i], "vec");
    }

    // for (auto i=0U; i<neles; ++i) {
    //     shrtags[i] = bc->PutY2BGate(shrtags[i]);
    //     shrtags[i] = bc->PutSharedOUTGate(shrtags[i]);
    //     shrin3[i] = bc->PutY2BGate(shrin3[i]);
    //     shrin3[i] = bc->PutSharedOUTGate(shrin3[i]);
    // }

    gParty.ExecCircuit();

    for (auto i=0; i<reles; ++i) {
        uint32_t *vec, bitlen, nvals;
        shrout[i]->get_clear_value_vec(&vec, &bitlen, &nvals);
        // std::cerr << bitlen << ' ' << nvals << std::endl;
        L[i].resize(2);
        for (auto j=0U; j<2; ++j) {
            L[i][j] = 0;
            for (auto k=0; k<32; ++k) {
                L[i][j] += vec[32*j+k] << k;
            }
        }
    }

    // RevealResult(L, context);

    gParty.Update(context);
    gParty.Reset();
    return;
}

std::vector<uint32_t> GenWoRIndices(uint32_t n, uint32_t m, uint32_t batch_size, CircSMPContext &context) {
    // auto start_time = clock();
    gParty.Init(context.address, context.port, context.role);

    uint32_t samples = m * batch_size;

    auto start_time = clock();

    std::random_device rd;
    std::mt19937 sd(rd());
    std::vector<uint32_t> indices(samples);
    for (auto i=0U; i<samples; ++i) {
        std::uniform_int_distribution<int> unifgen(0, n - 1 - i % batch_size);
        indices[i] = unifgen(sd);
    }

    Circuit* ac = gParty.GetCircuit(S_ARITH);
    Circuit* bc = gParty.GetCircuit(S_BOOL);
    Circuit* yc = gParty.GetCircuit(S_YAO);

    std::vector<share*> shrid(samples), bound(batch_size);
    share* zero = yc->PutINGate(0u, 32, SERVER);
    for (auto i=0u; i<batch_size; ++i) {
        bound[i] = yc->PutINGate(n-1-i, 32, SERVER);
    }
    share* add = yc->PutINGate(1u, 32, SERVER);
    share* shrn = yc->PutINGate(n, 32, SERVER);
    for (auto i=0U; i<samples; ++i) {

        if (i > 0 && i % batch_size == 0) {
            add = yc->PutADDGate(add, shrn);
        }

        shrid[i] = ac->PutSharedINGate(indices[i], 32);
        shrid[i] = yc->PutA2YGate(shrid[i]);
        share* selbit = yc->PutGTGate(shrid[i], bound[i % batch_size]);
        share* minus = yc->PutMUXGate(bound[i % batch_size], zero, selbit);
        shrid[i] = yc->PutSUBGate(shrid[i], minus);
        shrid[i] = yc->PutADDGate(shrid[i], add);

        // yc->PutPrintValueGate(shrid[i], "value");

        shrid[i] = bc->PutY2BGate(shrid[i]);
        shrid[i] = bc->PutSharedOUTGate(shrid[i]);

    }

    gParty.ExecCircuit();

    std::vector<uint64_t> keys (samples);
    std::vector<std::vector<uint32_t>> tuples (samples);

    for (auto i=0u; i<samples; ++i) {
        keys[i] = (((uint64_t)shrid[i] -> get_clear_value<uint32_t>()) << 32) + i;
        tuples[i] = {shrid[i] -> get_clear_value<uint32_t>(), 0};
        if (context.role == SERVER) {
            tuples[i][1] = n- (i % batch_size) + n * (i / batch_size);
        }
    }

    // gParty.Update(context);
    auto end_time = clock();
    context.time_cost -= 1.0 * (end_time - start_time) / CLOCKS_PER_SEC;

    gParty.Reset();

    ShuffleQSort(keys, tuples, context);

    // RevealResult(tuples, context);

    gParty.Init(context.address, context.port, context.role);
    bc = gParty.GetCircuit(S_BOOL);

    std::vector<share*> remdummy(samples), tmp(samples), out(samples);

    for (auto i=0u; i<samples; ++i) {
        remdummy[i] = bc -> PutSharedINGate(tuples[i][0], 32);
        tmp[i] = bc -> PutSharedINGate(tuples[i][1], 32);
        if (i >= 1) {
            share* selbit = bc -> PutEQGate(remdummy[i], remdummy[i-1]);
            out[i] = bc -> PutMUXGate(tmp[i], remdummy[i], selbit);
        } else {
            out[i] = remdummy[i];
        }
        out[i] = bc -> PutSharedOUTGate(out[i]);
    }

    gParty.ExecCircuit();

    std::vector<std::vector<uint32_t>> L(samples);

    for (auto i=0u; i<samples; ++i) {
        L[i].resize(2);
        L[i][0] = out[i] -> get_clear_value<uint32_t>();
        L[i][1] = tuples[i][1];
    }

    gParty.Update(context);
    gParty.Reset();

    // RevealResult(L, context);

    uint32_t jumps = (uint32_t) ceil(log2(log2(n))) + 1;
    
    for (auto i=0U; i<jumps; ++i) {
        std::cerr << "  pointer jumping " << i+1 << '/' << jumps << std::endl;
        PointerJumping(L, context);
    }

    // RevealResult(L, context);

    keys.resize(L.size());
    for (auto i=0u; i<samples; ++i) {
        keys[i] = (((uint64_t)L[i][0]) << 32) + L[i][1];
    }
    ShuffleQSort(keys, L, context);

    // RevealResult(L, context);

    bc = gParty.GetCircuit(S_BOOL);

    share* minus = bc->PutINGate(1u, 32, SERVER);
    share* bond = bc->PutINGate(n - batch_size, 32, SERVER);
    std::vector<share*> a(samples), b(samples);
    shrn = bc->PutINGate(n, 32, SERVER);
    for (auto i=0u; i<samples; ++i) {
        if (i > 0 && i % batch_size == 0) {
            minus = bc->PutADDGate(minus, shrn);
            bond = bc->PutADDGate(bond, shrn);
        }
        a[i] = bc->PutSharedINGate(L[i][0], 32);
        b[i] = bc->PutSharedINGate(L[i][1], 32);
        share *c1, *c2;
        if (i == 0) {
            c1 = bc->PutINGate(1u, 1, SERVER);
        } else {
            c1 = bc->PutEQGate(a[i], a[i-1]);
            c1 = ((BooleanCircuit*)bc)->PutINVGate(c1);
        }
        c2 = bc->PutGTGate(a[i], bond);
        c2 = ((BooleanCircuit*)bc)->PutINVGate(c2);
        share* selbit = bc->PutANDGate(c1, c2);
        out[i] = bc->PutMUXGate(a[i], b[i], selbit);
        out[i] = bc->PutSUBGate(out[i], minus);
        // bc->PutPrintValueGate(out[i], "outs");
        out[i] = bc->PutSharedOUTGate(out[i]);
    }

    gParty.ExecCircuit();
    std::vector<uint32_t> ids(samples);
    for (auto i=0u; i<samples; ++i) {
        ids[i] = out[i] -> get_clear_value<uint32_t>();
        // std::cerr << ids[i] << ' ' ;
    }
    gParty.Update(context);
    return ids;
}

void GetSamples(std::vector<uint32_t> indices, std::vector<std::vector<uint32_t>> tuples, std::vector<std::vector<uint32_t>> &samples, CircSMPContext &context) {
    uint32_t neles = tuples.size(), nattr = tuples[0].size(), meles = indices.size();
    uint32_t nsort = neles + meles;

    std::vector<uint64_t> sortkeys (nsort, 0);
    std::vector<std::vector<uint32_t>> sorttuples (nsort);
    for (auto i=0u; i<neles; ++i) {
        if (context.role == SERVER) {
            sortkeys[i] += (((uint64_t)i) << 32) + 1 + i;
        }
        sorttuples[i] = tuples[i];
        if (context.role == SERVER) {// a large dummy number to put it back in second sorting
            sorttuples[i].push_back(-1); 
        } else {
            sorttuples[i].push_back(0);
        }
        
    }
    std::vector<uint32_t> temptuple(nattr + 1, 0);
    for (auto i=0u; i<meles; ++i) {
        sortkeys[neles + i] = ((uint64_t)indices[i]) << 32;
        if (context.role == SERVER) {
            sortkeys[neles + i] += 1 + neles + i;
        }
        sorttuples[neles + i] = temptuple;
        if (context.role == SERVER) {
            sorttuples[neles + i][nattr] = i + 1;
        }
    }

    // RevealResult(sortkeys, context);
    // RevealResult(sorttuples, context);

    ShuffleQSort(sortkeys, sorttuples, context);

    // RevealResult(sortkeys, context);
    // RevealResult(sorttuples, context);
    for (auto i=0u; i<nsort; ++i) {
        sortkeys[i] = sorttuples[i][nattr];
        sorttuples[i].pop_back();
    }
    gParty.Init(context.address, context.port, context.role);
    Circuit* bc = gParty.GetCircuit(S_BOOL);
    Circuit* yc = gParty.GetCircuit(S_YAO);
    std::vector<share*> shrkey (nsort), shrval (nsort);
    share* dummy = bc->PutINGate((uint32_t)-1, 32, SERVER);
    for (auto i=0u; i<nsort; ++i) {
        shrkey[i] = bc->PutSharedINGate(((uint32_t)sortkeys[i]), 32);
        shrkey[i] = bc->PutEQGate(shrkey[i], dummy);
        shrkey[i] = yc->PutB2YGate(shrkey[i]);
        shrval[i] = bc->PutSharedSIMDINGate(nattr, (uint32_t*)sorttuples[i].data(), 32);
        shrval[i] = yc->PutB2YGate(shrval[i]);
    }
    for (auto i=1u; i<nsort; ++i) {
        shrval[i] = yc->PutMUXGate(shrval[i], shrval[i-1], yc->PutRepeaterGate(nattr, shrkey[i]));
    }

    for (auto i=0u; i<nsort; ++i) {
        shrval[i] = bc->PutY2BGate(shrval[i]);
        shrval[i] = bc->PutSharedOUTGate(shrval[i]);
    }
    gParty.ExecCircuit();
    uint32_t nvals, bitlen, *revtuple;
    for (auto i=0u; i<nsort; ++i) {
        shrval[i] -> get_clear_value_vec(&revtuple, &bitlen, &nvals);
        for (auto j=0u; j<nvals; ++j) {
            sorttuples[i][j] = revtuple[j];
        }
    }

    gParty.Update(context);
    gParty.Reset();
    // RevealResult(sorttuples, context);
    
    for (auto i=0u; i<nsort; ++i) {
        sortkeys[i] = (sortkeys[i] << 32);
        if (context.role == SERVER) {
            sortkeys[i] += i;
        }
    }
    // RevealResult(sortkeys, context);
    ShuffleQSort(sortkeys, sorttuples, context);
    // RevealResult(sorttuples, context);

    samples.resize(meles);
    for (auto i=0u; i<meles; ++i) {
        samples[i] = sorttuples[i];
    }
}

void WoRSample(std::vector<std::vector<uint32_t>> tuples, std::vector<std::vector<uint32_t>> &samples, uint32_t batch_cnt, uint32_t batch_size, CircSMPContext &context) {
    auto start_time = clock();
    std::vector<uint32_t> indices = GenWoRIndices(tuples.size(), batch_cnt, batch_size, context);
    std::cerr << "finish indices generation" << std::endl;
    // GetSamples(indices, tuples, samples, context);
    std::cerr << "ignore samples generation" << std::endl;
    auto end_time = clock();
    context.time_cost = 1.0 * (end_time - start_time) / CLOCKS_PER_SEC;
}

void UniformWoRSample(std::vector<std::vector<uint32_t>> tuples, std::vector<std::vector<uint32_t>> &samples, uint32_t sample_size, CircSMPContext &context) {
    auto start_time = clock();

    uint32_t neles = tuples.size();
    uint32_t nattrs = tuples[0].size();

    std::random_device rd;
    std::mt19937 sd(rd());
    std::uniform_int_distribution<int> unifgen(0, neles - 1);
    std::vector<uint32_t> indices = GenWoRIndices(tuples.size(), 1, sample_size, context);

    gParty.Reset();
    gParty.Init(context.address, context.port, context.role);
    Circuit* ac = gParty.GetCircuit(S_ARITH);
    Circuit* bc = gParty.GetCircuit(S_BOOL);
    Circuit* yc = gParty.GetCircuit(S_YAO);

    share* one = bc->PutINGate(1u, 32, SERVER);
    share* zero = bc->PutINGate(0u, 32, SERVER);

    std::cerr << "initial gates " << gParty.GetGates() << std::endl;

    std::vector<share*> sample_ids (sample_size);
    // for (auto i=0u; i<sample_size; ++i) {
    //     std::cerr << indices[i] << ' ';
    // }
    // Convert share from modular `neles` to modular `2^32`
    for (auto i=0U; i<sample_size; ++i) {
        sample_ids[i] = bc->PutSharedINGate(indices[i], 32);
    }

    // Generate sorting tuples
    // keys - sorting key
    // values - [ data value, original ID in sample ]
    uint32_t nsort = sample_size + neles;
    std::vector<share*> keys (nsort), values (nsort);
    std::vector<uint32_t> empty (nattrs + 1, 0), tuple;
        // sample index
    for (auto i=0U; i<sample_size; ++i) {
        keys[i] = bc->PutADDGate(sample_ids[i], sample_ids[i]);
        keys[i] = bc->PutADDGate(keys[i], one);
        if (context.role == SERVER) {
            empty[nattrs] = i+1; // ID from 1 to sample_size, NOT START FROM 0 !!!
        }
        values[i] = ac->PutSharedSIMDINGate(nattrs + 1, (uint32_t*)empty.data(), 32);
        values[i] = bc->PutA2BGate(values[i], yc);
    }
        // data set
    for (auto i=sample_size; i<nsort; ++i) {
        tuple = tuples[i - sample_size];
        tuple.push_back(0);
        keys[i] = bc->PutINGate((i - sample_size) << 1, 32, SERVER);
        values[i] = ac->PutSharedSIMDINGate(nattrs + 1, (uint32_t*)tuple.data(), 32);
        values[i] = bc->PutA2BGate(values[i], yc);
    }

    uint32_t pow2neles = (uint32_t)pow(2, ceil(log2(nsort)));
    keys.resize(pow2neles);
    values.resize(pow2neles);
    for (auto i=nsort; i<pow2neles; ++i) {
        keys[i] = bc->PutINGate((uint32_t)-1, 32, SERVER);
        values[i] = ac->PutSIMDINGate(nattrs + 1, (uint32_t*)empty.data(), 32, SERVER);
    }

    std::cerr << "prepare sorting gates " << gParty.GetGates() << std::endl;

    // for (auto i=0U; i<nsort; ++i) {
    //     bc->PutPrintValueGate(keys[i], " before sort key");
    //     bc->PutPrintValueGate(values[i], " before sort value");
    // }

    // for (auto i=0u; i<pow2neles; ++i) {
    //     bc->PutPrintValueGate(keys[i], "sort keys");
    // }

    GenBitonicSort(keys, values, 0, pow2neles, TRUE, bc);

    std::cerr << "after sorting gates " << gParty.GetGates() << std::endl;

    // for (auto i=0U; i<nsort; ++i) {
    //     bc->PutPrintValueGate(keys[i], "key");
    //     bc->PutPrintValueGate(values[i], "value");
    // }

    std::vector<share*> tags (nsort);
    uint32_t posids[1] = {nattrs};
    for (auto i=0U; i<nsort; ++i) {
        // bc->PutPrintValueGate(keys[i], "old key");
        keys[i] = bc->PutSubsetGate(values[i], posids, 1);
        // bc->PutPrintValueGate(values[i], "values");
        // bc->PutPrintValueGate(keys[i], "new key");
        tags[i] = bc->PutEQGate(keys[i], zero);
    }
    for (auto i=nsort; i<pow2neles; ++i); // keys = UINT_MAX

    // Generate Prefix Sum Gates
    uint32_t logn = (uint32_t) floor(log2(nsort));
    std::vector< std::vector< share* >> level_tags(2 * logn + 3), level_vals(2 * logn + 3);
    
    uint32_t l = 0;
    level_tags[0] = tags;
    level_vals[0] = values;

    // for (auto i=0U; i<nsort; ++i) {
    //     bc->PutPrintValueGate(level_tags[0][i], "=== tags === ");
    //     bc->PutPrintValueGate(level_vals[0][i], "=== values === ");
    // }

    // for (auto i=0; i<=logn; ++i) {
    //     level_tags[i+1].resize(nsort);
    //     level_vals[i+1].resize(nsort);
    //     uint32_t jump = 1 << i;
    //     for (auto j=0; j<nsort; ++j) {
    //         if (j < jump) {
    //             level_tags[i+1][j] = level_tags[i][j];
    //             level_vals[i+1][j] = level_vals[i][j];
    //         } else {
    //             share* selbit = level_tags[i][j];
    //             share* selbits = bc->PutRepeaterGate(nattrs + 1, selbit);
    //             level_tags[i+1][j] = bc->PutMUXGate(level_tags[i][j], level_tags[i][j-jump], selbit);
    //             level_vals[i+1][j] = bc->PutMUXGate(level_vals[i][j], level_vals[i][j-jump], selbits);
    //         }
    //     }
    // }

    for (auto i=0U; i<=logn; ++i) {
        l += 1;
        level_tags[l].resize(nsort);
        level_vals[l].resize(nsort);
        uint32_t jump = 1 << i;
        for (auto j=0U; j<nsort; ++j) {
            if ((j+1) % (2 * jump) == 0) {
                // std::cerr << j-jump << ' ' << j << std::endl;
                share* selbit = level_tags[l-1][j];
                share* selbits = bc->PutRepeaterGate(nattrs + 1, selbit);
                level_tags[l][j] = bc->PutMUXGate(level_tags[l-1][j], level_tags[l-1][j-jump], selbit);
                level_vals[l][j] = bc->PutMUXGate(level_vals[l-1][j], level_vals[l-1][j-jump], selbits);
            } else {
                level_tags[l][j] = level_tags[l-1][j];
                level_vals[l][j] = level_vals[l-1][j];
            }
        }
    }
    for (int i=logn; i>=0; --i) {
        l += 1;
        level_tags[l].resize(nsort);
        level_vals[l].resize(nsort);
        uint32_t jump = 1 << i;
        for (auto j=0U; j<nsort; ++j) {
            if (j >= jump && (j+1 - jump) % (2 * jump) == 0) {
                // std::cerr << j-jump << ' ' << j << std::endl;
                share* selbit = level_tags[l-1][j];
                share* selbits = bc->PutRepeaterGate(nattrs + 1, selbit);
                level_tags[l][j] = bc->PutMUXGate(level_tags[l-1][j], level_tags[l-1][j-jump], selbit);
                level_vals[l][j] = bc->PutMUXGate(level_vals[l-1][j], level_vals[l-1][j-jump], selbits);
            } else {
                level_tags[l][j] = level_tags[l-1][j];
                level_vals[l][j] = level_vals[l-1][j];
            }
        }
    }

    for (auto i=0U; i<nsort; ++i) {
        values[i] = level_vals[l][i];
    }

    std::cerr << "prefix sum gates " << gParty.GetGates() << std::endl;

    // for (auto i=0U; i<nsort; ++i) {
    //     bc->PutPrintValueGate(keys[i], " second round sort keys === ");
    //     bc->PutPrintValueGate(values[i], " second round sort values === ");
    // }

    GenBitonicSort(keys, values, 0, pow2neles, TRUE, bc);


    std::cerr << "Output sorting gates " << gParty.GetGates() << std::endl;

    // for (auto i=0U; i<nsort; ++i) {
    //     bc->PutPrintValueGate(keys[i], " after second round sort keys === ");
    //     bc->PutPrintValueGate(values[i], " after second round sort values === ");
    // }

    for (auto i=neles; i<nsort; ++i) {
        values[i] = bc -> PutSharedOUTGate(values[i]);
    }

    std::cerr << "Final gates " << gParty.GetGates() << std::endl;

    std::cerr << "execute circuit now ..." << std::endl;
    gParty.ExecCircuit();
    gParty.Update(context);

    uint32_t nvals, bitlen, *revtuple;
    samples.resize(sample_size);
    for (auto i=0U; i<sample_size; ++i) {
        values[neles + i] -> get_clear_value_vec(&revtuple, &bitlen, &nvals);
        assert(nvals == nattrs + 1);
        assert(bitlen == 32);
        samples[i].resize(nattrs);
        for (auto j=0U; j<nattrs; ++j) {
            samples[i][j] = revtuple[j];
        }
    }

    auto end_time = clock();
    context.time_cost += 1.0 * (end_time - start_time) / CLOCKS_PER_SEC;
    return;
}

/*
void FKJoin(std::vector<std::vector<uint32_t>> R, std::vector<std::vector<uint32_t>> S, std::vector<std::vector<uint32_t>> &T, CircSMPContext &context) {
    uint32_t reles = R.size(), seles = S.size();
    T.resize(reles);
    
    std::vector<uint64_t> sortkeys(reles + seles);
    std::vector<std::vector<uint32_t>> sorttuples(reles + seles);

    for (auto i=0U; i<seles; ++i) {
        sortkeys[i] = (((uint64_t)S[i][0]) << 32);
        if (context.role == SERVER) {
            sortkeys[i] += i;
        }
        sorttuples[i] = {S[i][0], 0, S[i][1]};
    }
    for (auto i=0U; i<reles; ++i) {
        sortkeys[seles + i] = (((uint64_t)R[i][0]) << 32 );
        if (context.role == SERVER) {
            sortkeys[seles + i] += seles + i;
        }
        sorttuples[seles + i] = {R[i][0], R[i][1], 0};
    }
    ShuffleQSort(sortkeys, sorttuples, context);
    printf("sort result\n");
    RevealResult(sorttuples, context);

    gParty.Init(context.address, context.port, context.role);
    Circuit *bc = gParty.GetCircuit(S_BOOL);
    uint32_t neles = sorttuples.size(), nlevels = (uint32_t)ceil(log2(neles));
    // Prefix sum circuit
    std::vector<share*> shrin1(neles), shrin2(neles), shrtags(neles), shrout(neles);
    std::vector<std::vector<share*>> ps(nlevels + 1);
    share* zero = bc->PutINGate(0U, 32, SERVER);
    ps[0].resize(neles);

    for (auto i=0U; i<neles; ++i) {
        shrin1[i] = bc->PutSharedINGate(sorttuples[i][0], 32);
        shrin2[i] = bc->PutSharedINGate(sorttuples[i][1], 32);
        ps[0][i] = bc->PutSharedINGate(sorttuples[i][2], 32);
        shrtags[i] = bc->PutEQGate(shrin2[i], zero);
    }
    for (auto leveli=1; leveli <= nlevels; ++leveli) {
        ps[leveli].resize(neles);
        uint32_t bound = (uint32_t)pow(2, leveli - 1);
        for (auto i=0; i<neles; ++i) {
            if (i < bound) {
                ps[leveli][i] = ps[leveli-1][i];
            } else {
                share* selbit = bc->PutEQGate(shrin1[i], shrin1[i-bound]);
                ps[leveli][i] = bc->PutMUXGate(ps[leveli-1][i-bound], ps[leveli-1][i], selbit);
            }
        }
    }

    for (auto i=0U; i<neles; ++i) {
        shrtags[i] = bc->PutSharedOUTGate(shrtags[i]);
        shrout[i] = bc->PutSharedOUTGate(ps[nlevels][i]);
    }

    gParty.ExecCircuit();

    std::vector<std::vector<uint32_t>> tuples(neles);
    std::vector<bool> tags(neles);
    for (auto i=0; i<neles; ++i) {
        tags[i] = shrtags[i] -> get_clear_value<bool>();
        tuples[i] = {sorttuples[i][0], (shrout[i] -> get_clear_value<uint32_t>())};
    }

    RevealResult(tuples, context);

    gParty.Update(context);
    gParty.Reset();
    return;
}

*/