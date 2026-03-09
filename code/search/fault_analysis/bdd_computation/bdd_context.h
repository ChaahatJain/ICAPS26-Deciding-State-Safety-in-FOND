//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#ifndef PLAJA_BDD_CONTEXT_H
#define PLAJA_BDD_CONTEXT_H

#include <memory>
#include <map>
#include "../../../utils/utils.h"
#include "../../../assertions.h"
#include "../../using_search.h"
#include <stack>
#include "../../smt/base/smt_context.h"
#include <iostream>
#include <fstream>
#include "cuddObj.hh"
#include <cmath>
#include <utility>
extern "C" {
    #include "dddmp.h"
}

namespace BDD_IN_PLAJA {
    /**
     * Context for Veritas queries.
    */
    class Context : public PLAJA::SmtContext {
    public:
        struct Addend {
            int var_id;
            int factor;
            int lb;

            explicit Addend(int id_, int f_): var_id(id_), factor(f_) {}
            ~Addend() = default;
            DEFAULT_CONSTRUCTOR(Addend)
        };
    private:
        

        struct VariableInformation {
            int lb;
            int ub;
            std::vector<BDD> bits;

            explicit VariableInformation(int lb_, int ub_, std::vector<BDD> bits_):
                lb(lb_)
                , ub(ub_),
                bits(bits_) {}

            ~VariableInformation() = default;
            DEFAULT_CONSTRUCTOR(VariableInformation)
        };

        struct ActionOpBDD {
            BDD guard;
            std::vector<BDD> updates;
            std::vector<std::vector<int>> target_variables;

            explicit ActionOpBDD(BDD g, std::vector<BDD> u, std::vector<std::vector<int>> variables):
                guard(g)
                , updates(u)
                , target_variables(variables)
                {}

            ~ActionOpBDD() = default;
            DEFAULT_CONSTRUCTOR(ActionOpBDD)
        };

        Cudd manager;
        unsigned int contextCounter;

        std::map<ActionOpID_type, ActionOpBDD> operator_cache; // We store it as a pair because guard and update must be separated.
        std::map<int, VariableInformation> variableInformation; // ordered VarIndex, <lb, ub, bits>

    public:
        Context();
        ~Context();
        DELETE_CONSTRUCTOR(Context)

        [[nodiscard]] inline std::size_t get_number_of_inputs() const { return variableInformation.size()/2; }
        [[nodiscard]] inline std::size_t get_number_of_variables() const { return variableInformation.size(); }

        [[nodiscard]] inline bool exists(int var) const {
            return variableInformation.count(var);
        }

        void add_var(int var_index, int lb, int ub);

        void printOrdering() const;

        void printOrderingBDD (BDD b) const;

        void reorderVariables(bool dynamic);

        [[nodiscard]] inline BDD get_var_bit(int var_index, int bit) const {
            PLAJA_ASSERT(exists(var_index))
            return variableInformation.at(var_index).bits.at(bit);
        }

        [[nodiscard]] inline size_t get_num_bits(int var_index) const {
            PLAJA_ASSERT(exists(var_index))
            return variableInformation.at(var_index).bits.size();
        }

        inline BDD get_false() const {
            return BDD(manager, Cudd_ReadLogicZero(manager.getManager()));
        }

        inline BDD get_true() const {
            return BDD(manager, Cudd_ReadOne(manager.getManager()));
        }

        std::vector<BDD> get_target_variables() const;

        std::vector<BDD> get_source_variables() const;

        inline void save_operator_BDD(ActionOpID_type action_op, BDD guard, std::vector<BDD> updates, std::vector<std::vector<int>> target_variables) {
            operator_cache.insert_or_assign(action_op, ActionOpBDD(guard, updates, target_variables));
        }

        [[nodiscard]] inline bool exists_operator(ActionOpID_type action_op) const {
            return operator_cache.count(action_op);
        }

        [[nodiscard]] inline BDD get_operator_guard(ActionOpID_type action_op) const {
            PLAJA_ASSERT(operator_cache.count(action_op) > 0)
            return operator_cache.at(action_op).guard;
        }

        [[nodiscard]] inline std::vector<BDD> get_operator_updates(ActionOpID_type action_op) const {
            PLAJA_ASSERT(operator_cache.count(action_op) > 0)
            return operator_cache.at(action_op).updates;
        }

        BDD get_unused_var_biimp(std::vector<int> variables) const;

        [[nodiscard]] inline int get_lower_bound(int var) const {
            PLAJA_ASSERT(exists(var))
            return variableInformation.at(var).lb;
        }

        [[nodiscard]] inline int get_upper_bound(int var) const {
            PLAJA_ASSERT(exists(var))
            return variableInformation.at(var).ub;
        }

        FCT_IF_DEBUG(void dump() const {
            for (const auto& pair: variableInformation) {
                std::cout << "Variable: " << pair.first << std::endl;
                for (auto b : pair.second.bits) {
                    std::cout << b << std::endl;
                }
            }
        };)

        BDD get_unused_biimp(BDD rho, BDD update) const;

        void printSatisfyingAssignment(BDD b, std::vector<BDD> debug) const;

        void generate_counter_example(std::vector<int> positive, std::vector<int> negatives, std::vector<BDD> debug);
        void saveBDD(const char* filepath, BDD b) const;
        BDD loadBDD(const char* filepath) const;

    };

}

#endif //PLAJA_BDD_CONTEXT_H
