#pragma once

#include "../core/Utils.h"
#include "../core/Party.h"
#include "../core/PermutationNetwork.h"
#include "../core/Sort/SortingNetwork.h"
#include "../core/Sort/ShuffleQSort.h"

#include "abycore/sharing/sharing.h"
#include "abycore/circuit/booleancircuits.h"
#include "abycore/circuit/arithmeticcircuits.h"
#include "abycore/circuit/circuit.h"
#include "abycore/aby/abyparty.h"

// sample_size = number_of_batches * batch_size [total number of generated WR samples]
void UniformWoRSample(std::vector<std::vector<uint32_t>> tuples, std::vector<std::vector<uint32_t>> &samples, uint32_t sample_size, CircSMPContext &context);
void FKJoin(std::vector<std::vector<uint32_t>> R, std::vector<std::vector<uint32_t>> S, std::vector<std::vector<uint32_t>> &T, CircSMPContext &context);
void PointerJumping(std::vector<std::vector<uint32_t>> &L, CircSMPContext &context);
std::vector<uint32_t> GenWoRIndices(uint32_t n, uint32_t m, uint32_t batch_size, CircSMPContext &context);
void GetSamples(std::vector<uint32_t> indices, std::vector<std::vector<uint32_t>> tuples, std::vector<std::vector<uint32_t>> &samples, CircSMPContext &context);
void WoRSample(std::vector<std::vector<uint32_t>> tuples, std::vector<std::vector<uint32_t>> &samples, uint32_t batch_cnt, uint32_t batch_size, CircSMPContext &context);
void GenRandomIDs(std::vector<uint32_t> &IDs, uint32_t s, uint32_t d, CircSMPContext &context);
share* PutMODGate(share* a, share* b, Circuit* circ);