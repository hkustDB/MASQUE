#pragma once

#include "Party.h"
#include "Utils.h"

#include "sharing/sharing.h"
#include "circuit/booleancircuits.h"
#include "circuit/booleancircuits.h"
#include "circuit/arithmeticcircuits.h"
#include "circuit/circuit.h"
#include "aby/abyparty.h"

uint32_t EstimateGates(uint32_t neles);
void GenSelectionBits(const uint32_t *permuIndices, int size, bool *bits);
void GetSelectionBits(std::vector<uint32_t> indices, std::vector<bool> &selbits);
void GeneratePermCircuit(std::vector<share*> inwires, std::vector<share*> &outwires, uint32_t &pos, Circuit* circ);
extern std::vector<share*> gselbits;

// Waksman Permutation Network ("On Arbitrary Waksman Networks and their Vulnerability")
// - vals / tuples: dataset
// - perm: permutation indices
// - perm_holder: [SERVER/CLIENT] holds the perm
void PermutationNetwork(std::vector<uint32_t> &vals, std::vector<uint32_t> perm, e_role perm_holder, e_role role, e_sharing intype = S_BOOL, e_sharing outtype = S_BOOL);
void PermutationNetwork(std::vector< std::vector<uint32_t> > &tuples, std::vector<uint32_t> perm, e_role perm_holder, e_role role, e_sharing intype = S_BOOL, e_sharing outtype = S_BOOL);
