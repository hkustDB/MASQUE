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

void GenSampleTable(std::vector<uint32_t> degtable, std::vector<std::vector<uint32_t>> &sampletable, uint32_t s, uint32_t k, CircSMPContext &context);
void GenCompactionCircuit(uint32_t g, std::vector<share*> &attrs, std::vector<share*> tags);
void GenExpansionCircuit(std::vector<share*> &tuples, std::vector<share*> mut, uint32_t meles, CircSMPContext context);
void DegreeClassification(uint32_t g, std::vector<std::vector<uint32_t>> &tuples, std::vector<uint32_t> &degtable, CircSMPContext &context);
void StratifiedSampling(uint32_t g, std::vector<std::vector<uint32_t>> tuples, std::vector<std::vector<uint32_t>> &samples, CircSMPContext &context);