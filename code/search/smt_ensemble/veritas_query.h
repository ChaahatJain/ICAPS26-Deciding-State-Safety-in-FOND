//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#ifndef PLAJA_VERITAS_QUERY_H
#define PLAJA_VERITAS_QUERY_H

#include <memory>
#include <set>
#include <vector>
#include "../../utils/floating_utils.h"
#include "../information/jani2ensemble/using_jani2ensemble.h"
#include "forward_smt_veritas.h"
#include "veritas_context.h"
#include "addtree.hpp"
#include "predicates/Equation.h"
#include "predicates/Tightening.h"
#include "predicates/Disjunct.h"
#include "constraints_in_veritas.h"
#include <stack>
#include <tuple>
#include <map>

/**
 * PlaJA view of a Veritas input query.
 */

namespace VERITAS_IN_PLAJA {

/**
 * Base class for incremental interface shared by query and solver.
 */
    class QueryConstructable {

    protected:
        std::shared_ptr<VERITAS_IN_PLAJA::Context> context;
        std::vector<VERITAS_IN_PLAJA::Equation> equations;
        std::vector<VERITAS_IN_PLAJA::Equation> updates;
        std::vector<VERITAS_IN_PLAJA::Disjunct> disjuncts;
        explicit QueryConstructable(std::shared_ptr<VERITAS_IN_PLAJA::Context> c);
        std::stack<std::vector<VERITAS_IN_PLAJA::Equation>> equationStack;
        std::stack<std::vector<VERITAS_IN_PLAJA::Equation>> updateStack;
        std::stack<std::vector<VERITAS_IN_PLAJA::Disjunct>> disjunctStack;

    public:
        virtual ~QueryConstructable() = 0;
        DEFAULT_CONSTRUCTOR(QueryConstructable)

        [[nodiscard]] inline std::shared_ptr<VERITAS_IN_PLAJA::Context> share_context() const { return context; }

        [[nodiscard]] inline VERITAS_IN_PLAJA::Context& _context() const { return *context; }
        
        [[nodiscard]] inline bool exists(VeritasVarIndex_type var) const {
            return context->exists(var);
        }
        // bounds:
        [[nodiscard]] VeritasFloating_type get_lower_bound(VeritasVarIndex_type var) const {
            PLAJA_ASSERT(exists(var))
            return context->get_lower_bound(var);
        }

        [[nodiscard]] VeritasFloating_type get_upper_bound(VeritasVarIndex_type var) const {
            PLAJA_ASSERT(exists(var))
            return context->get_upper_bound(var);
        }
        
        void tighten_lower_bound(VeritasVarIndex_type var, VeritasFloating_type lb) { context->tighten_bounds(var, lb, VERITAS_IN_PLAJA::infinity); }
        void tighten_upper_bound(VeritasVarIndex_type var, VeritasFloating_type ub) { context->tighten_bounds(var, VERITAS_IN_PLAJA::negative_infinity, ub); }

        void push() { context->push(); equationStack.push(equations); updateStack.push(updates); disjunctStack.push(disjuncts); }
        void pop() {  equations = equationStack.top(); context->pop(); equationStack.pop(); updates = updateStack.top(); updateStack.pop(); disjuncts = disjunctStack.top(); disjunctStack.pop(); }
        void clear() { std::stack<std::vector<VERITAS_IN_PLAJA::Equation>>().swap(equationStack); context->clear(); std::stack<std::vector<VERITAS_IN_PLAJA::Equation>>().swap(updateStack); std::stack<std::vector<VERITAS_IN_PLAJA::Disjunct>>().swap(disjunctStack); }

        void add_equation(const VERITAS_IN_PLAJA::Equation& eq) { equations.push_back(eq); }
        void add_update(const VERITAS_IN_PLAJA::Equation& eq) { updates.push_back(eq); }
        void add_disjunct(const VERITAS_IN_PLAJA::Disjunct dj) { disjuncts.push_back(dj); }

        std::vector<VERITAS_IN_PLAJA::Equation> getEquations() const {
            std::vector<VERITAS_IN_PLAJA::Equation> eqs;
            eqs.reserve(equations.size() + updates.size());
            for (auto&& eq: equations) {
                eqs.push_back(eq);
            }

            for (auto&& up: updates) {
                eqs.push_back(up);
            }
            
            return eqs;
        }

        struct UpdatePropagation {std::vector<std::pair<veritas::FloatT, veritas::FeatId>> source_vars; veritas::FloatT scalar; };

        std::map<veritas::FeatId, UpdatePropagation> get_small_updates() const {
            // std::vector<std::tuple<veritas::FeatId, veritas::FloatT, veritas::FeatId, veritas::FloatT>> upds;

            std::map<veritas::FeatId, UpdatePropagation> updateMap;

            // upds.reserve(updates.size());
            for (auto eq: updates) {
                auto tup = eq.get_update();
                UpdatePropagation upd;
                upd.source_vars = std::get<1>(tup);
                upd.scalar = std::get<2>(tup);
                updateMap[std::get<0>(tup)] = upd;
            }
            return updateMap;
        }

        [[nodiscard]] inline veritas::AddTree generateEqTrees(veritas::AddTree policy) const { 
            veritas::AddTree trees(1, veritas::AddTreeType::REGR); 
            bool added = false;
            for (auto eq: equations) { 
                auto ts = eq.to_trees(*context, false, policy);
                if (ts.size() == 0) {
                    continue; 
                    return veritas::AddTree(0, veritas::AddTreeType::REGR);
                } // This can happen when no falsifying constraints exist for the equation we are encoding.
                trees.add_trees(ts); 
                added = true;

                auto bounds = eq.get_tighter_bounds(*context);
                context->tighten_bounds(std::get<0>(bounds), std::get<1>(bounds), std::get<2>(bounds));
            }

            for (auto eq: updates) { 
                auto ts = eq.to_trees(*context, true, policy);
                trees.add_trees(ts);
                added = true;
            }

            for (auto dj : disjuncts) {
                auto ts = dj.to_trees(context, policy);
                trees.add_trees(ts);
                added = true;
                PLAJA_LOG("Warning: This has not been exhaustively tested since no benchmarks have disjunctive guards currently.")
            }
            if (!added) {
                // This can happen when no falsifying constraints exist for the equation we are encoding.
                return veritas::AddTree(0, veritas::AddTreeType::REGR);
            }
            return trees; 
        }
    };

    class VeritasQuery final: public QueryConstructable {

    private:
        veritas::AddTree inputQuery;

    public:
        explicit VeritasQuery(std::shared_ptr<VERITAS_IN_PLAJA::Context> c);
        explicit VeritasQuery(const VERITAS_IN_PLAJA::ContextView& view);
        explicit VeritasQuery(const veritas::AddTree& input_query/*, std::unordered_set<VeritasVarIndex_type>* integer_vars = nullptr*/); // handcraft query
        explicit VeritasQuery(const veritas::AddTree& input_query, std::shared_ptr<VERITAS_IN_PLAJA::Context> c, const std::vector<VeritasVarIndex_type>& input_indices);
        VeritasQuery(const VeritasQuery& parent);
        ~VeritasQuery() final;

        VeritasQuery(VeritasQuery&& parent) = delete;
        DELETE_ASSIGNMENT(VeritasQuery)

        [[nodiscard]] veritas::AddTree get_input_query() const { return inputQuery; }

        void set_input_query(const veritas::AddTree& policy) { inputQuery = policy; }
 
        // vars:

        [[nodiscard]] inline VeritasFloating_type get_domain_size(VeritasVarIndex_type var) const { return get_upper_bound(var) - get_lower_bound(var); }

        //
        [[nodiscard]] inline VeritasInteger_type get_lower_bound_int(VeritasVarIndex_type var) const {
            PLAJA_ASSERT(is_marked_integer(var))
            PLAJA_ASSERT(PLAJA_FLOATS::is_integer(get_lower_bound(var)))
            return static_cast<VeritasInteger_type>(std::round(get_lower_bound(var)));
        }

        [[nodiscard]] inline VeritasInteger_type get_upper_bound_int(VeritasVarIndex_type var) const {
            PLAJA_ASSERT(is_marked_integer(var))
            PLAJA_ASSERT(PLAJA_FLOATS::is_integer(get_upper_bound(var)))
            return static_cast<VeritasInteger_type>(std::round(get_upper_bound(var)));
        }

        [[nodiscard]] inline VeritasInteger_type get_domain_size_int(VeritasVarIndex_type var) const { return std::max(0, get_upper_bound_int(var) - get_lower_bound_int(var) + 1); }

        inline void set_lower_bound(VeritasVarIndex_type var, VeritasFloating_type lb) {
            PLAJA_ASSERT(exists(var))
            context->tighten_bounds(var, lb, VERITAS_IN_PLAJA::infinity);
            CHECK_VERITAS_INF_BOUNDS(get_lower_bound(var), get_upper_bound(var))
        }


        inline void set_upper_bound(VeritasVarIndex_type var, VeritasFloating_type ub) {
            PLAJA_ASSERT(exists(var))
            context->tighten_bounds(var, VERITAS_IN_PLAJA::negative_infinity, ub);
        }


        // inputs:
        [[nodiscard]] inline std::size_t number_of_inputs() const { return context->get_number_of_variables(); }

        [[nodiscard]] inline bool has_continuous_inputs() const { 
            for (auto var_it = context->variableIterator(); !var_it.end(); ++var_it) {
                if (not var_it.is_integer()) return false;
            }
            return true;
        }

        [[nodiscard]] veritas::FlatBox generate_input_list() const;

        std::vector<VeritasVarIndex_type> _integer_vars() const;

        // integer:
        [[nodiscard]] inline bool is_marked_integer(VeritasVarIndex_type var) const {
            PLAJA_ASSERT(exists(var))
            return context->is_marked_integer(var);
        }

        inline void mark_integer(VeritasVarIndex_type var) {
            PLAJA_ASSERT(exists(var))
            context->mark_integer(var);
        }

        [[nodiscard]] inline const veritas::AddTree& _query() const { return inputQuery; }

        
        void dump() const;

        class VariableIterator {
            friend VERITAS_IN_PLAJA::VeritasQuery;
        private:
            const VERITAS_IN_PLAJA::VeritasQuery* query;
            VeritasVarIndex_type currentVar; // NOLINT(*-use-default-member-init)

            explicit VariableIterator(const VERITAS_IN_PLAJA::VeritasQuery& query_):
                query(&query_)
                , currentVar(0) {}

        public:
            ~VariableIterator() = default;
            DELETE_CONSTRUCTOR(VariableIterator)

            inline void operator++() { ++currentVar; }

            [[nodiscard]] inline bool end() const { return currentVar >= query->context->get_number_of_variables(); }

            //
            [[nodiscard]] inline VeritasVarIndex_type var() const {
                PLAJA_ASSERT(query->exists(currentVar))
                return currentVar;
            }

            //
            [[nodiscard]] inline VeritasFloating_type lower_bound() const { return query->get_lower_bound(currentVar); }

            [[nodiscard]] inline VeritasFloating_type upper_bound() const { return query->get_upper_bound(currentVar); }

            //
            [[nodiscard]] inline bool is_integer() const { return query->is_marked_integer(currentVar); }
        };

        [[nodiscard]] inline VariableIterator variableIterator() const { return VariableIterator(*this); }

    };

}

#endif //PLAJA_VERITAS_QUERY_H
