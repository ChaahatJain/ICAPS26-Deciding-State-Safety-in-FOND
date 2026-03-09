//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#ifndef PLAJA_BDD_QUERY_H
#define PLAJA_BDD_QUERY_H

#include <memory>
#include <set>
#include <vector>
#include "../../../utils/floating_utils.h"
#include "bdd_context.h"
#include <stack>
#include <tuple>
#include <map>
#include "../../successor_generation/action_op.h"
#include "../../../parser/ast/expression/special_cases/linear_expression.h"


/**
 * PlaJA view of a Veritas input query.
 */
namespace BDD_IN_PLAJA {

    class Query final {

    private:
        std::shared_ptr<Context> context;
        BDD eq_to_bdd(const LinearExpression* expr, int target) const;
        BDD guard_to_bdd(ActionOp action_op);
        BDD update_to_bdd(const Update& update);
        std::vector<BDD> updates_to_bdd(ActionOp action_op);
        std::vector<int> targets_in_updates(ActionOp action_op);
        BDD get_unused_var_biimp(std::vector<int> variables) { return context->get_unused_var_biimp(variables); }
        BDD nary_to_bdd(const NaryExpression* guard) const;
        BDD binary_op_expression_to_bdd(const BinaryOpExpression* bin) const;
        BDD unary_to_bdd(const UnaryOpExpression* expr) const;

    public:
        explicit Query();
        ~Query();

        Query(Query&& parent) = delete;
        DELETE_ASSIGNMENT(Query)
 
        // vars:
        [[nodiscard]] inline int get_domain_size(int var) const { return context->get_upper_bound(var) - context->get_lower_bound(var); }

        //
        [[nodiscard]] inline int get_lower_bound_int(int var) const {
            return static_cast<int>(std::round(context->get_lower_bound(var)));
        }

        [[nodiscard]] inline int get_upper_bound_int(int var) const {
            return static_cast<int>(std::round(context->get_upper_bound(var)));
        }

        BDD get_false() const { return context->get_false(); }
        BDD get_true() const { return context->get_true(); }

        BDD get_relational_product(BDD rho, BDD update, bool abstractSource = false, bool debug = false);

        void reorderVariables(bool dynamic = false) { context->reorderVariables(dynamic); }
        
        void printVariableOrdering(BDD b) { context->printOrderingBDD(b); }
        // inputs:
        [[nodiscard]] inline std::size_t number_of_inputs() const { return context->get_number_of_inputs(); }

        void add_var_to_query(int var_index, int lb, int ub) {
            context->add_var(var_index, lb, ub);
        }
        FCT_IF_DEBUG(void dump() const { context->dump(); })

        void operator_to_bdd(ActionOp action_op);
        BDD get_guard(ActionOpID_type action_op_id) const { return context->get_operator_guard(action_op_id); }
        std::vector<BDD> get_updates(ActionOpID_type action_op_id) const { return context->get_operator_updates(action_op_id); }

        BDD lin_expr_to_bdd(const LinearExpression* expr, int target = -1) const;
        BDD bound_to_bdd(const LinearExpression* expr, bool is_output = false) const;
        BDD state_condition_to_bdd(const StateConditionExpression* state) const;

        inline BDD source_to_target_variables(BDD b) const {
            return b.SwapVariables(context->get_source_variables(), context->get_target_variables());
        }
        inline BDD target_to_source_variables(BDD b) const {
            return b.SwapVariables(context->get_target_variables(), context->get_source_variables());
        }

        void printSatisfyingAssignment(BDD b, std::vector<BDD> debug) const { context->printSatisfyingAssignment(b, debug); }

        BDD computeConjunctionTree(std::vector<BDD> bdds) const;
        BDD computeDisjunctionTree(std::vector<BDD> bdds) const;

        void saveBDD(const char* filepath, BDD b) const { context->saveBDD(filepath, b); }
        BDD loadBDD(const char* filepath) const { return context->loadBDD(filepath); }
        void generateCounterExample(std::vector<int> positive, std::vector<int> negative, std::vector<BDD> debug) { context->generate_counter_example(positive, negative, debug); }
    };

}

#endif //PLAJA_BDD_QUERY_H
