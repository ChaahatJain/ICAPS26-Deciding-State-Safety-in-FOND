//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#include "bdd_context.h"

BDD_IN_PLAJA::Context::Context():
    manager(0,0), contextCounter(0) {}

BDD_IN_PLAJA::Context::~Context() = default;

void BDD_IN_PLAJA::Context::add_var(int var_index, int lb, int ub) {
    auto num_bits = static_cast<int>(std::ceil(std::log2(1 + ub - lb)));
    std::vector<BDD> bits;
    for (auto bit = 0; bit < num_bits; ++bit) {
        BDD b = manager.bddVar(contextCounter);
        ++contextCounter;
        bits.push_back(b);
    }
    variableInformation.insert_or_assign(var_index, VariableInformation(lb, ub, bits));
}

void BDD_IN_PLAJA::Context::printOrdering() const {
            for (int i = 0; i < manager.ReadSize(); ++i) {
                int varIndex = manager.ReadPerm(i);
                std::cout << "Position " << varIndex << " for variable: " << i << std::endl;
            }
        }

void BDD_IN_PLAJA::Context::printOrderingBDD(BDD b) const {
    auto mgr = b.manager();
    for (int i = 0; i < Cudd_ReadSize(mgr); ++i) {
        int varIndex = Cudd_ReadPerm(mgr, i);
        std::cout << "Position " << varIndex << " for variable: " << i << std::endl;
    }
}

void BDD_IN_PLAJA::Context::reorderVariables(bool dynamic) {
    if (!dynamic) {
        auto numVars = manager.ReadSize();
        std::vector<int> newOrder(numVars);
        int j = 0;
        for (int i = 0; i < numVars/2; ++i) {
            newOrder[j++] = i;
            newOrder[j++] = i + numVars/2;
        }
    
        int* orderArray = newOrder.data();
        manager.ShuffleHeap(orderArray);
    } else {
        // printOrdering();
        manager.ReduceHeap();
        // printOrdering();
    }
}

std::vector<BDD> BDD_IN_PLAJA::Context::get_target_variables() const {
    std::vector<BDD> targets;
    for (auto key = get_number_of_inputs(); key < get_number_of_variables(); ++key) {
        auto bits = variableInformation.at(key).bits;
        for (auto b : bits) {
            targets.push_back(b);
        }
    }
    return targets;
}

std::vector<BDD> BDD_IN_PLAJA::Context::get_source_variables() const {
    std::vector<BDD> source;
    for (auto key = 0; key < get_number_of_inputs(); ++key) {
        auto bits = variableInformation.at(key).bits;
        for (auto b : bits) {
            source.push_back(b);
        }
    }
    return source;
}

BDD BDD_IN_PLAJA::Context::get_unused_var_biimp(std::vector<int> variables) const {
    auto q = get_true();
    for (auto key = get_number_of_inputs(); key < get_number_of_variables(); ++key) {
        bool key_found = false;
        for (auto v :variables) {
            if (v == key) {
                key_found = true;
            }
        }
        if (key_found) continue;
        auto source_bits = variableInformation.at(key - get_number_of_inputs()).bits;
        auto bits = variableInformation.at(key).bits;   
        for (auto i = 0; i < bits.size(); ++i) {
            q = q & ((bits[i] & source_bits[i]) | (~bits[i] & ~source_bits[i]));
        }             
    }
    // Check each variable against each BDD
    return q;
}

BDD BDD_IN_PLAJA::Context::get_unused_biimp(BDD rho, BDD update) const {
    auto rho_vars = Cudd_SupportIndex(manager.getManager(), rho.getNode());
    auto update_vars = Cudd_SupportIndex(manager.getManager(), update.getNode());
    BDD biimp = get_true();
    for (int i = 0; i < manager.ReadSize(); ++i) {
        if (not rho_vars[i]) continue;
        if (update_vars[i]) continue;
        BDD target = get_true();
        BDD source = get_false();
        if (i >= get_source_variables().size()) {
            target = manager.bddVar(i);
            source = manager.bddVar(i - get_source_variables().size());
        } else {
            target = manager.bddVar(i + get_source_variables().size());
            source = manager.bddVar(i);
        }
        BDD b = (target & source) | (~target & ~source);
        biimp = biimp & b;
    }
    
    return biimp;
}

void BDD_IN_PLAJA::Context::printSatisfyingAssignment(BDD b, std::vector<BDD> debug) const {
    // int num_inputs = get_source_variables().size();
    auto num_solutions = Cudd_CountMinterm(manager.getManager(), b.getNode(), manager.ReadSize());
    std::cout << "Num solutions: " << num_solutions << std::endl;
    DdGen *gen;
    int *cube;
    CUDD_VALUE_TYPE value;
    gen = Cudd_FirstCube(manager.getManager(), b.getNode(), &cube, &value);
    if (gen != NULL) {
        std::vector<int> assignment(manager.ReadSize(), 2);  // Initialize with "don't care" (2)

        // Loop through the cube array (number of BDD variables)
    #ifndef NDEBUG
        for (int i = 0; i < manager.ReadSize(); ++i) {
            if (cube[i] == 0) {
                std::cout << "x" << i << " = 0" << std::endl;
                assignment[i] = 0;
            } else if (cube[i] == 1) {
                std::cout << "x" << i << " = 1" << std::endl;
                assignment[i] = 1;
            } else {
                std::cout << "x" << i << " = don't care" << std::endl;
            }
        }
    #endif

        // Check if the assignment satisfies the BDD
        BDD assignmentBdd = manager.bddOne();
        for (int i = 0; i < manager.ReadSize(); ++i) {
            if (assignment[i] == 0) {
                assignmentBdd &= !manager.bddVar(i);
            } else if (assignment[i] == 1) {
                assignmentBdd &= manager.bddVar(i);
            }
        }
        std::cout << "Assignment: " << assignmentBdd << std::endl;
        for (auto f : debug) {
            if (assignmentBdd <= f) {
                std::cout << "The assignment satisfies the BDD " << f << std::endl;
            } else {
                std::cout << "The assignment does not satisfy the BDD." << f << std::endl;
            }
        }
        Cudd_GenFree(gen);
    } else {
        std::cout << "No satisfying assignment found." << std::endl;
    }
}

void BDD_IN_PLAJA::Context::saveBDD(const char* filepath, BDD b) const {
    const char *filename = "unsafe_region.bdd";
    int mode = DDDMP_MODE_BINARY;
    Dddmp_VarInfoType varorder = DDDMP_VARDEFAULT;

    int result = Dddmp_cuddBddStore(manager.getManager(), const_cast<char*>(filename), b.getNode(), NULL, NULL,
                                    mode, varorder, const_cast<char*>(filepath), NULL);

    if (result != 1) {
        std::cerr << "Error storing BDD to " << filepath << std::endl;
        PLAJA_ABORT
    }

    std::cout << "BDD stored successfully to " << filepath << std::endl;
}

BDD BDD_IN_PLAJA::Context::loadBDD(const char* filepath) const {
 
    // Load BDD using DDDMP
    DdNode *loadedBDD = Dddmp_cuddBddLoad(manager.getManager(), DDDMP_VAR_MATCHIDS, NULL, NULL, NULL,
                                          DDDMP_MODE_BINARY, const_cast<char*>(filepath), NULL);

    if (loadedBDD == NULL) {
        std::cerr << "Error loading BDD from " << filepath << std::endl;
        abort();
    }

    std::cout << "BDD loaded successfully from " << filepath << std::endl;

    // Convert DDNode to BDD
    return BDD(manager, loadedBDD);
}

void BDD_IN_PLAJA::Context::generate_counter_example(std::vector<int> positive, std::vector<int> negative, std::vector<BDD> debug) {
    BDD b = get_true();
    for (auto i : positive) {
        b = b & manager.bddVar(i);
    }

    for (auto i : negative) {
        b = b & !manager.bddVar(i);
    }

    for (auto f : debug) {
        if (b <= f) {
            std::cout << "The assignment satisfies the BDD " << Cudd_DagSize(f.getNode()) << std::endl;
        } else {
            std::cout << "The assignment does not satisfy the BDD." << Cudd_DagSize(f.getNode()) << std::endl;
        }
    }
}
