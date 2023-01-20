#include "PermutationNetwork.h"

uint32_t EstimateGates(uint32_t neles) {
    if (neles <= 1) return 0;
    return neles - 1 + EstimateGates(neles / 2) + EstimateGates((neles + 1) / 2);
}

    void GenSelectionBits(const uint32_t *permuIndices, int size, bool *bits)
    {
        if (size == 2)
            bits[0] = permuIndices[0];
        if (size <= 2)
            return;

        uint32_t *invPermuIndices = new uint32_t[size];
        for (int i = 0; i < size; i++)
            invPermuIndices[permuIndices[i]] = i;

        bool odd = size & 1;

        // Solve the edge coloring problem

        // flag=0: non-specified; flag=1: upperNetwork; flag=2: lowerNetwork
        char *leftFlag = new char[size]();
        char *rightFlag = new char[size]();
        int rightPointer = size - 1;
        int leftPointer;
        while (rightFlag[rightPointer] == 0)
        {
            rightFlag[rightPointer] = 2;
            leftPointer = permuIndices[rightPointer];
            leftFlag[leftPointer] = 2;
            if (odd && leftPointer == size - 1)
                break;
            leftPointer = leftPointer & 1 ? leftPointer - 1 : leftPointer + 1;
            leftFlag[leftPointer] = 1;
            rightPointer = invPermuIndices[leftPointer];
            rightFlag[rightPointer] = 1;
            rightPointer = rightPointer & 1 ? rightPointer - 1 : rightPointer + 1;
        }
        for (int i = 0; i < size - 1; i++)
        {
            rightPointer = i;
            while (rightFlag[rightPointer] == 0)
            {
                rightFlag[rightPointer] = 2;
                leftPointer = permuIndices[rightPointer];
                leftFlag[leftPointer] = 2;
                leftPointer = leftPointer & 1 ? leftPointer - 1 : leftPointer + 1;
                leftFlag[leftPointer] = 1;
                rightPointer = invPermuIndices[leftPointer];

                rightFlag[rightPointer] = 1;
                rightPointer = rightPointer & 1 ? rightPointer - 1 : rightPointer + 1;
            }
        }
        delete[] invPermuIndices;

        // Determine bits on left gates
        int halfSize = size / 2;
        for (int i = 0; i < halfSize; i++)
            bits[i] = leftFlag[2 * i] == 2;

        int upperIndex = halfSize;
        int uppergateNum = EstimateGates(halfSize);
        int lowerIndex = upperIndex + uppergateNum;
        int rightGateIndex = lowerIndex + (odd ? EstimateGates(halfSize + 1) : uppergateNum);
        // Determine bits on right gates
        for (int i = 0; i < halfSize - 1; i++)
            bits[rightGateIndex + i] = rightFlag[2 * i] == 2;
        if (odd)
            bits[rightGateIndex + halfSize - 1] = rightFlag[size - 2] == 1;

        delete[] leftFlag;
        delete[] rightFlag;

        // Compute upper network
        uint32_t *upperIndices = new uint32_t[halfSize];
        for (int i = 0; i < halfSize - 1 + odd; i++)
            upperIndices[i] = permuIndices[2 * i + bits[rightGateIndex + i]] / 2;
        if (!odd)
            upperIndices[halfSize - 1] = permuIndices[size - 2] / 2;
        GenSelectionBits(upperIndices, halfSize, bits + upperIndex);
        delete[] upperIndices;

        // Compute lower network
        int lowerSize = halfSize + odd;
        uint32_t *lowerIndices = new uint32_t[lowerSize];
        for (int i = 0; i < halfSize - 1 + odd; i++)
            lowerIndices[i] = permuIndices[2 * i + 1 - bits[rightGateIndex + i]] / 2;
        if (odd)
            lowerIndices[halfSize] = permuIndices[size - 1] / 2;
        else
            lowerIndices[halfSize - 1] = permuIndices[2 * halfSize - 1] / 2;
        GenSelectionBits(lowerIndices, lowerSize, bits + lowerIndex);
        delete[] lowerIndices;
    }

void GetSelectionBits(std::vector<uint32_t> indices, std::vector<bool> &selbits) {
    uint32_t neles = indices.size();
    uint32_t upper = neles / 2;
    uint32_t lower = neles - upper;

    if (neles <= 1) {
        return;
    }
    if (neles == 2) {
        selbits.push_back(indices[0] == 1);
        return;
    }

    bool odd = neles & 1;
    std::vector<uint32_t> invindices (neles);
    for (auto i=0; i<neles; ++i) {
        invindices[indices[i]] = i;
    }

    // 2-coloring: 0 - unspecified; 1 - upper network; 2 - lower network
    std::vector<uint32_t> leftFlag (neles), rightFlag (neles);
    uint32_t leftPointer, rightPointer = neles - 1;
    while (rightFlag[rightPointer] == 0) {
        rightFlag[rightPointer] = 2;
        leftPointer = indices[rightPointer];
        leftFlag[leftPointer] = 2;
        if (odd && leftPointer == neles - 1) {
            break;
        }
        leftPointer = leftPointer ^ 1;
        leftFlag[leftPointer] = 1;
        rightPointer = invindices[leftPointer];
        rightFlag[rightPointer] = 1;
        rightPointer = rightPointer ^ 1;
    }
    for (uint32_t i = 0; i < neles - 1; ++i) {
        rightPointer = i;
        while (rightFlag[rightPointer] == 0) {
            rightFlag[rightPointer] = 2;
            leftPointer = indices[rightPointer];
            leftFlag[leftPointer] = 2;
            leftPointer = leftPointer ^ 1;
            leftFlag[leftPointer] = 1;
            rightPointer = invindices[leftPointer];
            rightFlag[rightPointer] = 1;
            rightPointer = rightPointer ^ 1;
        }
    }

    // left gate
    for (uint32_t i=0; i<upper; ++i) {
        selbits.push_back( (leftFlag[i+i] == 2) );
    }

    // right gate
    std::vector<bool> rightbits;
    for (uint32_t i=0; i<upper - 1; ++i) {
        rightbits.push_back( (rightFlag[i+i] == 2) );
    }
    if (odd) {
        rightbits.push_back( (rightFlag[neles - 2] == 1) );
    }
    
    std::vector<uint32_t> upperindices(upper), lowerindices(lower);
    // upper network
    for (uint32_t i=0; i<upper - 1; ++i) {
        upperindices[i] = indices[i+i+rightbits[i]] / 2;
    }
    if (!odd) {
        upperindices[upper - 1] = indices[neles - 2] / 2;
    }

    // lower network
    for (uint32_t i=0; i<upper - 1 + odd; ++i) {
        lowerindices[i] = indices[i+i+1-rightbits[i]] / 2;
    }
    if (odd) {
        lowerindices[upper] = indices[neles - 1] / 2;
    } else {
        lowerindices[upper - 1] = indices[upper + upper - 1] / 2;
    }

    std::vector<bool> upperbits, lowerbits;
    GetSelectionBits(upperindices, upperbits);
    selbits.insert (selbits.end(), upperbits.begin(), upperbits.end());
    GetSelectionBits(lowerindices, lowerbits);
    selbits.insert (selbits.end(), lowerbits.begin(), lowerbits.end());
    selbits.insert (selbits.end(), rightbits.begin(), rightbits.end());
    std::cerr << 12345 << ' ' << neles << ' ' << upperbits.size() << ' ' << lowerbits.size() << ' ' << rightbits.size() << std::endl;
    return;
}

std::vector<share*> gselbits;

void GeneratePermCircuit(std::vector<share*> inwires, std::vector<share*> &outwires, uint32_t &pos, Circuit* circ) {
    uint32_t neles = inwires.size();
    if (neles <= 1) {
        outwires.insert(outwires.end(), inwires.begin(), inwires.end());
        return;
    }

    uint32_t upper = neles / 2;
    bool odd = neles & 1;

    std::vector<share*> upperinwires, upperoutwires;
    std::vector<share*> lowerinwires, loweroutwires;

    for (auto i=0; i<upper; ++i) {
        share** swapout = circ->PutCondSwapGate(inwires[i+i], inwires[i+i+1], gselbits[pos+i], TRUE);
        upperinwires.push_back(swapout[0]);
        lowerinwires.push_back(swapout[1]); 
    }
    if (odd) {
        lowerinwires.push_back(inwires[neles-1]);
    }
    pos += upper;

    GeneratePermCircuit(upperinwires, upperoutwires, pos, circ);
    GeneratePermCircuit(lowerinwires, loweroutwires, pos, circ);

    for (auto i=0; i<(neles - 1) / 2; ++i) {
        share** swapout = circ->PutCondSwapGate(upperoutwires[i], loweroutwires[i], gselbits[pos+i], TRUE);
        outwires.push_back(swapout[0]);
        outwires.push_back(swapout[1]);
    }
    pos += (neles - 1) / 2;
    if (odd) {
        outwires.push_back(loweroutwires[upper]);
    } else {
        outwires.push_back(upperoutwires[upper - 1]);
        outwires.push_back(loweroutwires[upper - 1]);
    }

    return;
}

void PermutationNetwork(std::vector<uint32_t> &vals, std::vector<uint32_t> perm, e_role perm_holder, e_role role,
                        e_sharing intype, e_sharing outtype) {
    Circuit* circ = gParty.GetCircuit(S_BOOL);
    Circuit* ac = gParty.GetCircuit(S_ARITH);
    Circuit* yc = gParty.GetCircuit(S_YAO);
    uint32_t neles = vals.size();
    std::vector<share*> inwires(neles), outwires;
    std::vector<bool> selbits;

    // if (role == perm_holder) {
    //     GetSelectionBits(perm, selbits);
    // } else {
    //     selbits.resize(EstimateGates(neles));
    // }
    selbits.resize(EstimateGates(neles));
    if (role == perm_holder) {
        bool *tmpbits = new bool[selbits.size()];
        GenSelectionBits(perm.data(), perm.size(), tmpbits);
        for (auto i=0u; i<selbits.size(); ++i) {
            selbits[i] = tmpbits[i];
        }
    }
    // input gates
    // values
    if (intype == S_BOOL) {
        for (auto i=0; i<neles; ++i) {
            inwires[i] = circ -> PutSharedINGate(vals[i], 32);
        }
    } else {
        for (auto i=0; i<neles; ++i) {
            inwires[i] = ac -> PutSharedINGate(vals[i], 32);
            inwires[i] = circ -> PutA2BGate(inwires[i], yc);
        }
    }
    // selection bits
    gselbits.clear();
    gselbits.resize(selbits.size());
    for (auto i=0; i<selbits.size(); ++i) {
        gselbits[i] = circ -> PutINGate((uint32_t)selbits[i], 1, perm_holder);
    }
    uint32_t pos = 0;
    GeneratePermCircuit(inwires, outwires, pos, circ);

    // output gates
    if (outtype == S_BOOL) {
        for (auto i=0; i<neles; ++i) {
            outwires[i] = circ -> PutSharedOUTGate(outwires[i]);   
        }
    } else {
        for (auto i=0; i<neles; ++i) {
            outwires[i] = ac -> PutB2AGate(outwires[i]);
            outwires[i] = ac -> PutSharedOUTGate(outwires[i]);
        }
    }
    gParty.ExecCircuit();
    
    for (auto i=0; i<neles; ++i) {
        vals[i] = outwires[i] -> get_clear_value<uint32_t>();
    }
    return;
}


void PermutationNetwork(std::vector< std::vector<uint32_t> > &tuples, std::vector<uint32_t> perm, e_role perm_holder, e_role role,
                        e_sharing intype, e_sharing outtype) {
    Circuit* circ = gParty.GetCircuit(S_BOOL);
    Circuit* ac = gParty.GetCircuit(S_ARITH);
    Circuit* yc = gParty.GetCircuit(S_YAO);
    uint32_t neles = tuples.size(), nattrs = tuples[0].size();
    std::vector<share*> inwires(neles), outwires;
    std::vector<bool> selbits;

    // if (role == perm_holder) {
    //     GetSelectionBits(perm, selbits);
    // } else {
    //     selbits.resize(EstimateGates(neles));
    // }
    selbits.resize(EstimateGates(neles));
    if (role == perm_holder) {
        bool *tmpbits = new bool[selbits.size()];
        GenSelectionBits(perm.data(), perm.size(), tmpbits);
        for (auto i=0u; i<selbits.size(); ++i) {
            selbits[i] = tmpbits[i];
        }
    }
    // input gates
    // values
    if (intype == S_BOOL) {
        for (auto i=0; i<neles; ++i) {
            inwires[i] = circ -> PutSharedSIMDINGate(nattrs, (uint32_t*)tuples[i].data(), 32);
        }
    } else {
        for (auto i=0; i<neles; ++i) {
            inwires[i] = ac -> PutSharedSIMDINGate(nattrs, (uint32_t*)tuples[i].data(), 32);
            inwires[i] = circ -> PutA2BGate(inwires[i], yc);
        }
    }
    // selection bits
    gselbits.clear();
    gselbits.resize(selbits.size());
    for (auto i=0; i<selbits.size(); ++i) {
        gselbits[i] = circ -> PutINGate((uint32_t)selbits[i], 1, perm_holder);
    }
    uint32_t pos = 0;
    GeneratePermCircuit(inwires, outwires, pos, circ);

    // output gates
    if (outtype == S_BOOL) {
        for (auto i=0; i<neles; ++i) {
            outwires[i] = circ -> PutSharedOUTGate(outwires[i]);   
        }
    } else {
        for (auto i=0; i<neles; ++i) {
            outwires[i] = ac -> PutB2AGate(outwires[i]);
            outwires[i] = ac -> PutSharedOUTGate(outwires[i]);
        }
    }
    
    gParty.ExecCircuit();
    
    uint32_t nvals, bitlen, *vals;
    for (auto i=0; i<neles; ++i) {
        outwires[i] -> get_clear_value_vec(&vals, &bitlen, &nvals);
        assert(bitlen == 32);
        assert(nvals == nattrs);
        for (auto j=0; j<nattrs; ++j) {
            tuples[i][j] = vals[j];
        }
    }
    return;
}