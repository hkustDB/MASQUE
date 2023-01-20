#pragma once

#include "../Party.h"
#include "../Utils.h"

#include "sharing/sharing.h"
#include "circuit/booleancircuits.h"
#include "circuit/booleancircuits.h"
#include "circuit/arithmeticcircuits.h"
#include "circuit/circuit.h"
#include "aby/abyparty.h"

// Bitonic sort on {(key, value)}
void GenBitonicMerger(std::vector<share*> &keys, std::vector<share*> &vals, uint32_t pos, uint32_t len, bool direction, Circuit* circ);
void GenBitonicSort(std::vector<share*> &keys, std::vector<share*> &vals, uint32_t pos, uint32_t len, bool direction, Circuit* circ);

// Sort 'vals' according to 'keys'
// direction: 0 - ascending; 1 - decending
void SortingNetwork(std::vector<uint64_t> keys, std::vector<std::vector<uint32_t>> &vals, e_role role, bool direction = TRUE,
                    e_sharing keytype = S_BOOL, e_sharing intype = S_BOOL, e_sharing outtype = S_BOOL);
// Circuit depth to sort 2^k elements is (2k+1)^2
