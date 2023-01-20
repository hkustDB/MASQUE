#include "stratifiedsample.h"

void GenCompactionCircuit(uint32_t g, std::vector<share*> &attrs, std::vector<share*> tags) { // Notice: compact tag=1 tuples
    Circuit* yc = gParty.GetCircuit(S_YAO);

    uint32_t neles = attrs.size();
    uint32_t logn = floor(log2(neles));

    std::vector<share*> movesteps(neles);
    movesteps[0] = tags[0];
    for (auto i=1u; i<neles; ++i) {
        movesteps[i] = yc->PutADDGate(movesteps[i-1], tags[i]);
    }
    std::vector<std::vector<share*>> levelattrs (logn + 1), levelmoves (logn + 1);
    levelattrs[0] = attrs;
    levelmoves[0] = movesteps;
    for (auto i=0u; i<logn; ++i) {
        uint32_t jump = 1u << i;
        levelattrs[i+1].resize(neles);
        levelmoves[i+1].resize(neles);
        for (auto j=0u; j<neles; ++j) {
            if (j + jump < neles) {
                share* selbit = levelmoves[i][j+jump]->get_wire_ids_as_share(i);
                levelattrs[i+1][j] = yc->PutMUXGate(levelattrs[i][j+jump], levelattrs[i][j], selbit);
                levelmoves[i+1][j] = yc->PutMUXGate(levelmoves[i][j+jump], levelmoves[i][j], selbit);
            } else {
                levelattrs[i+1][j] = levelattrs[i][j];
                levelmoves[i+1][j] = levelmoves[i][j];
            }
        }
    }
    attrs = levelattrs[logn];
    attrs.resize(g);
}

void DegreeClassification(uint32_t g, std::vector<std::vector<uint32_t>> &tuples, std::vector<uint32_t> &degtable, CircSMPContext &context) {
    uint32_t neles = tuples.size(), nattrs = tuples[0].size();
    std::vector<uint64_t> sortkeys (neles);
    for (auto i=0u; i<neles; ++i) {
        sortkeys[i] = (((uint64_t)tuples[i][0]) << 32);
        if (context.role == SERVER) {
            sortkeys[i] += i;
        }
    }
    ShuffleQSort(sortkeys, tuples, context);
    // RevealResult(tuples, context);

    gParty.Init(context.address, context.port, context.role);
    Circuit* bc = gParty.GetCircuit(S_BOOL);
    Circuit* yc = gParty.GetCircuit(S_YAO);
    share* one = yc->PutINGate(1u, 32, SERVER);
    share* zero = yc->PutINGate(0u, 32, SERVER);
    std::vector<share*> attr (neles), cont (neles);

    for (auto i=0u; i<neles; ++i) {
        attr[i] = bc->PutSharedINGate(tuples[i][0], 32);
        attr[i] = yc->PutB2YGate(attr[i]);
        cont[i] = one;
        if (i > 0) {
            share* selbit = yc->PutEQGate(attr[i], attr[i-1]);
            cont[i] = yc->PutADDGate(cont[i], yc->PutMUXGate(cont[i-1], zero, selbit));
        }
    }
    for (int i=neles - 2; i>=0; --i) {  
        share* selbit = yc->PutEQGate(attr[i], attr[i+1]);
        cont[i] = yc->PutMUXGate(cont[i+1], cont[i], selbit);
    }
    for (auto i=0u; i<neles; ++i) {
        cont[i] = bc->PutY2BGate(cont[i]);
        cont[i] = bc->PutSharedOUTGate(cont[i]);
    }
    gParty.ExecCircuit();
    gParty.Update(context);
    for (auto i=0u; i<neles; ++i) {
        tuples[i].push_back(cont[i]->get_clear_value<uint32_t>());
        sortkeys[i] = (((uint64_t)tuples[i][nattrs]) << 32); // sort by degree then group-by key
        if (context.role == SERVER) {
            sortkeys[i] += i;
        }
    }
    gParty.Reset();
    // RevealResult(sortkeys, context);
    ShuffleQSort(sortkeys, tuples, context);
    // RevealResult(tuples, context);
    
    /*
    Compaction to compute degree information
    */
    gParty.Init(context.address, context.port, context.role);
    bc = gParty.GetCircuit(S_BOOL);
    yc = gParty.GetCircuit(S_YAO);
    zero = yc->PutINGate(0u, 32, SERVER);

    std::vector<share*> attrs(neles), tags(neles);
    for (auto i=0u; i<neles; ++i) {
        attr[i] = bc->PutSharedINGate(tuples[i][0], 32);
        attr[i] = yc->PutB2YGate(attr[i]);
        attrs[i] = bc->PutSharedINGate(tuples[i][nattrs], 32);
        attrs[i] = yc->PutB2YGate(attrs[i]);
        if (i == 0) {
            tags[i] = zero;
        } else {
            tags[i] = yc->PutEQGate(attr[i], attr[i-1]);
        }
    }
    GenCompactionCircuit(g, attrs, tags);
    for (auto i=0u; i<g; ++i) {
        attrs[i] = bc->PutY2BGate(attrs[i]);
        attrs[i] = bc->PutSharedOUTGate(attrs[i]);
    }
    gParty.ExecCircuit();
    gParty.Update(context);
    degtable.resize(g);
    for (auto i=0u; i<g; ++i) {
        degtable[i] = attrs[i] -> get_clear_value<uint32_t>();
    }
    // RevealResult(degtable, context);
    gParty.Reset();
    return;
}

void GenSampleTable(std::vector<uint32_t> degtable, std::vector<std::vector<uint32_t>> &sampletable, uint32_t s, uint32_t k, CircSMPContext &context) {
    gParty.Init(context.address, context.port, context.role);
    Circuit *bc = gParty.GetCircuit(S_BOOL);
    Circuit *yc = gParty.GetCircuit(S_YAO);
    uint32_t g = degtable.size();
    std::vector<share*> deg(g), startpos(g), strak(g), tuples(g);
    share* shrk = bc->PutSharedINGate(k, 32);
    shrk = yc->PutB2YGate(shrk);
    share* twoshrk = yc->PutADDGate(shrk, shrk);
    share* zero = yc->PutCONSGate((uint64_t)0, 32);
    for (auto i=0u; i<g; ++i) {
        deg[i] = bc->PutSharedINGate(degtable[i], 32);
        deg[i] = yc->PutB2YGate(deg[i]);
        if (i == 0u) {
            startpos[i] = zero;
        } else {
            startpos[i] = yc->PutADDGate(startpos[i-1], deg[i-1]);
        }
        // yc->PutPrintValueGate(startpos[i], "start pos");
        share* selbit = yc->PutGTGate(deg[i], shrk);
        strak[i] = yc->PutMUXGate(shrk, deg[i], selbit);
        tuples[i] = yc->PutCombinerGate(startpos[i], deg[i]);
        share* comp = yc->PutGTGate(deg[i], twoshrk);
        tuples[i] = yc->PutCombinerGate(tuples[i], comp);
        // yc->PutPrintValueGate(tuples[i], " ===>  tuples");
    }
    GenExpansionCircuit(tuples, strak, s, context);

    for (auto i=0u; i<s; ++i) {
        tuples[i] = bc -> PutY2BGate(tuples[i]);
        // bc->PutPrintValueGate(tuples[i], "   ");
        tuples[i] = bc -> PutSharedOUTGate(tuples[i]);
    }

    gParty.ExecCircuit();
    gParty.Update(context);

    sampletable.resize(s);
    for (auto i=0u; i<s; ++i) {
        uint32_t *vals, nval, bitl;
        tuples[i] -> get_clear_value_vec(&vals, &bitl, &nval);
        uint32_t val = 0, k = 0;
        for (auto j=0u; j<nval; ++j) {
            if (k == 32) {
                sampletable[i].push_back(val);
                k = 0; val = 0;
            }
            val += vals[j] << k;
            k += 1;
        }
        sampletable[i].push_back(val);
    }
    // sampletable = {start_position, degree, ki < 2di or not (i.e., useful samples)}
    RevealResult(sampletable, context);
    return;
}

void GenExpansionCircuit(std::vector<share*> &tuples, std::vector<share*> mut, uint32_t meles, CircSMPContext context) {
    Circuit* yc = gParty.GetCircuit(S_YAO);
    uint32_t neles = tuples.size(), nvals = tuples[0] -> get_nvals();
    std::vector<share*> presum(neles);
    share* zero = yc -> PutCONSGate((uint64_t)0, 32);
    share* zerotuple = yc -> PutCONSGate((uint64_t)0, nvals);
    zerotuple = yc->PutCombinerGate(zerotuple);

    presum[0] = mut[0];
    for (auto i=1u; i<neles; ++i) {
        presum[i] = yc->PutADDGate(presum[i-1], mut[i]);
    }
    uint32_t logm = ceil(log2(meles));
    std::vector<std::vector<share*>> levelvals (logm + 2), leveldis (logm + 2);

    levelvals[0].resize(meles);
    leveldis[0].resize(meles);
    for (auto i=0u; i<meles; ++i) {
        if (i < neles) {
            levelvals[0][i] = tuples[i];
            if (i == 0) {
                leveldis[0][i] = zero;
            } else {
                leveldis[0][i] = presum[i-1];
                share* loci = yc->PutCONSGate((uint64_t)i, 32);
                leveldis[0][i] = yc->PutSUBGate(leveldis[0][i], loci);
            }
        } else {
            levelvals[0][i] = zerotuple;
            leveldis[0][i] = zero;
        }
        // yc->PutPrintValueGate(levelvals[0][i], ("0," + std::to_string(i) + " --> level vals").c_str());
        // yc->PutPrintValueGate(leveldis[0][i], ("0," + std::to_string(i) + " level dis").c_str());
    }

    uint32_t l = 0;
    for (int i=logm; i>=0; --i) {
        l += 1;
        levelvals[l].resize(meles);
        leveldis[l].resize(meles);
        uint32_t jump = 1U << i;
        share* shrjump = yc->PutCONSGate((uint64_t)jump, 32);
        for (auto j=0; j<meles; ++j) {
            levelvals[l][j] = zerotuple;
            leveldis[l][j] = zero;

            if (j >= jump) {
                share* selbit = leveldis[l-1][j-jump]->get_wire_ids_as_share(i);
                // yc->PutPrintValueGate(selbit, (std::to_string(l) + "," + std::to_string(j) + "," + std::to_string(jump) + " *** jump bit ").c_str());
                share* selbits = yc->PutRepeaterGate(nvals, selbit);
                // yc->PutPrintValueGate(selbits, (std::to_string(l) + "," + std::to_string(j) + "," + std::to_string(jump) + " *** jump bits ").c_str());
                // yc->PutPrintValueGate(levelvals[l-1][j-jump], (std::to_string(l) + "," + std::to_string(j) + "," + std::to_string(jump) + " *** jump values ").c_str());
                levelvals[l][j] = yc->PutMUXGate(levelvals[l-1][j-jump], levelvals[l][j], selbits);
                // yc->PutPrintValueGate(levelvals[l][j], (std::to_string(l) + "," + std::to_string(j) + "," + std::to_string(jump) + " *** jumped values ").c_str());
                leveldis[l][j] = yc->PutMUXGate( yc->PutSUBGate(leveldis[l-1][j-jump], shrjump) , leveldis[l][j], selbit);
            }

            {
                share* selbit = leveldis[l-1][j]->get_wire_ids_as_share(i);
                share* eqzero = yc->PutEQGate(yc->PutSplitterGate(levelvals[l-1][j]), yc->PutSplitterGate(zerotuple));
                // yc->PutPrintValueGate(eqzero, (std::to_string(l) + "," + std::to_string(j) + " =========== equal to zero or not ").c_str());
                selbit = yc->PutXORGate(selbit, eqzero);
                share* selbits = yc->PutRepeaterGate(nvals, selbit);
                levelvals[l][j] = yc->PutMUXGate(levelvals[l][j], levelvals[l-1][j], selbits);
                leveldis[l][j] = yc->PutMUXGate(leveldis[l][j], leveldis[l-1][j], selbit);
            }
            // yc->PutPrintValueGate(leveldis[l][j], (std::to_string(l) + "," + std::to_string(j) + " level dis").c_str());
            // yc->PutPrintValueGate(levelvals[l][j], (std::to_string(l) + "," + std::to_string(j) + " --> level vals").c_str());
        }
    }

    // for (auto i=0u; i<meles; ++i) {
    //     yc->PutPrintValueGate(levelvals[l][i], ("before copy tuples " + std::to_string(i)).c_str());
    // }
    tuples.resize(meles);
    for (auto i=1u; i<meles; ++i) {
        share* selbit = yc->PutEQGate( yc->PutSplitterGate(levelvals[l][i]), yc->PutSplitterGate(zerotuple) );
        // yc->PutPrintValueGate(selbit, "selbit");
        share* selbits = yc->PutRepeaterGate(nvals, selbit);
        tuples[i] = yc->PutMUXGate(tuples[i-1], levelvals[l][i], selbits);
        // yc->PutPrintValueGate(tuples[i], "expanded tuples");
    }
}

void GenRandomIDs(std::vector<std::vector<uint32_t>> IDs, std::vector<std::vector<uint32_t>> &samples, uint32_t neles, uint32_t jumps, CircSMPContext &context) {

    gParty.Init(context.address, context.port, context.role);
    Circuit* bc = gParty.GetCircuit(S_BOOL);
    Circuit* yc = gParty.GetCircuit(S_YAO);

}

void StratifiedSampling(uint32_t g, std::vector<std::vector<uint32_t>> tuples, std::vector<std::vector<uint32_t>> &samples, CircSMPContext &context) {
    return;
}