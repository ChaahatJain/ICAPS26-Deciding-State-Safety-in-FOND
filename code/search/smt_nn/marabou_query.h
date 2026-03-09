//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_MARABOU_QUERY_H
#define PLAJA_MARABOU_QUERY_H

#include <memory>
#include <unordered_set>
#include "../../include/marabou_include/input_query.h"
#include "../../utils/floating_utils.h"
#include "../information/jani2nnet/using_jani2nnet.h"
#include "forward_smt_nn.h"
#include "marabou_context.h"

/**
 * PlaJA view of a Marabou input query.
 */
namespace MARABOU_IN_PLAJA {

/**
 * Base class for incremental interface shared by query and solver.
 */
    class QueryConstructable {

    protected:
        std::shared_ptr<MARABOU_IN_PLAJA::Context> context;
        explicit QueryConstructable(std::shared_ptr<MARABOU_IN_PLAJA::Context> c);

    public:
        virtual ~QueryConstructable() = 0;
        DEFAULT_CONSTRUCTOR(QueryConstructable)

        [[nodiscard]] inline std::shared_ptr<MARABOU_IN_PLAJA::Context> share_context() const { return context; }

        [[nodiscard]] inline MARABOU_IN_PLAJA::Context& _context() const { return *context; }

        [[nodiscard]] virtual const InputQuery& get_input_query() const = 0;

        virtual MarabouVarIndex_type add_aux_var() = 0;

        [[nodiscard]] virtual MarabouFloating_type get_lower_bound(MarabouVarIndex_type var) const = 0;
        [[nodiscard]] virtual MarabouFloating_type get_upper_bound(MarabouVarIndex_type var) const = 0;
        virtual void tighten_lower_bound(MarabouVarIndex_type var, MarabouFloating_type lb) = 0;
        virtual void tighten_upper_bound(MarabouVarIndex_type var, MarabouFloating_type ub) = 0;

        [[nodiscard]] virtual std::size_t get_num_equations() const = 0;
        virtual void add_equation(const Equation& eq) = 0;
        virtual void add_equation(Equation&& eq) = 0;
        virtual void add_equations(std::list<Equation>&& equations) = 0;

        [[nodiscard]] virtual std::size_t get_num_pl_constraints() const = 0;
        virtual void add_pl_constraint(PiecewiseLinearConstraint* pl_constraint) = 0;
        virtual void add_pl_constraint(const PiecewiseLinearConstraint* pl_constraint) = 0;

        virtual void add_disjunction(DisjunctionConstraint* disjunction) = 0;

        // efficient bound computation
        [[nodiscard]] virtual NLR::NetworkLevelReasoner* provide_nlr() = 0;
        virtual void update_nlr() = 0;

        template<bool relax>
        inline void set_tightening(const Tightening& tightening) {
            switch (tightening._type) {
                case Tightening::LB: {
                    tighten_lower_bound(tightening._variable, relax ? std::floor(tightening._value) : tightening._value);
                    break;
                }
                case Tightening::UB: {
                    tighten_upper_bound(tightening._variable, relax ? std::ceil(tightening._value) : tightening._value);
                    break;
                }
                default: PLAJA_ABORT
            }
        }

    };

    class MarabouQuery final: public QueryConstructable {

    private:
        InputQuery inputQuery;

    public:
        explicit MarabouQuery(std::shared_ptr<MARABOU_IN_PLAJA::Context> c);
        explicit MarabouQuery(const MARABOU_IN_PLAJA::ContextView& view);
        explicit MarabouQuery(const InputQuery& input_query/*, std::unordered_set<MarabouVarIndex_type>* integer_vars = nullptr*/); // handcraft query
        MarabouQuery(const MarabouQuery& parent);
        ~MarabouQuery() final;

        MarabouQuery(MarabouQuery&& parent) = delete;
        DELETE_ASSIGNMENT(MarabouQuery)

        [[nodiscard]] const InputQuery& get_input_query() const override { return inputQuery; }

        // vars:

        [[nodiscard]] inline bool exists(MarabouVarIndex_type var) const {
            // PLAJA_ASSERT(context->exists(var))
            return var < inputQuery.getNumberOfVariables();
        }

        // aux vars:

        MarabouVarIndex_type add_aux_var() final { return inputQuery.introduceAuxVar(); }

        void remove_aux_var_to(unsigned int new_num_vars);

        // bounds:
        [[nodiscard]] MarabouFloating_type get_lower_bound(MarabouVarIndex_type var) const final {
            PLAJA_ASSERT(exists(var))
            return inputQuery.getLowerBound(var);
        }

        [[nodiscard]] MarabouFloating_type get_upper_bound(MarabouVarIndex_type var) const final {
            PLAJA_ASSERT(exists(var))
            return inputQuery.getUpperBound(var);
        }

        [[nodiscard]] inline MarabouFloating_type get_domain_size(MarabouVarIndex_type var) const { return get_upper_bound(var) - get_lower_bound(var); }

        //
        [[nodiscard]] inline MarabouInteger_type get_lower_bound_int(MarabouVarIndex_type var) const {
            PLAJA_ASSERT(is_marked_integer(var))
            PLAJA_ASSERT(PLAJA_FLOATS::is_integer(get_lower_bound(var)))
            return static_cast<MarabouInteger_type>(std::round(inputQuery.getLowerBound(var)));
        }

        [[nodiscard]] inline MarabouInteger_type get_upper_bound_int(MarabouVarIndex_type var) const {
            PLAJA_ASSERT(is_marked_integer(var))
            PLAJA_ASSERT(PLAJA_FLOATS::is_integer(get_upper_bound(var)))
            return static_cast<MarabouInteger_type>(std::round(inputQuery.getUpperBound(var)));
        }

        [[nodiscard]] inline MarabouInteger_type get_domain_size_int(MarabouVarIndex_type var) const { return std::max(0, get_upper_bound_int(var) - get_lower_bound_int(var) + 1); }

        inline void set_lower_bound(MarabouVarIndex_type var, MarabouFloating_type lb) {
            PLAJA_ASSERT(exists(var))
            inputQuery.setLowerBound(var, lb);
            CHECK_MARABOU_INF_BOUNDS(get_lower_bound(var), get_upper_bound(var))
        }

        void tighten_lower_bound(MarabouVarIndex_type var, MarabouFloating_type lb) final { inputQuery.tightenLowerBound(var, lb); }

        inline void set_upper_bound(MarabouVarIndex_type var, MarabouFloating_type ub) {
            PLAJA_ASSERT(exists(var))
            inputQuery.setUpperBound(var, ub);
            CHECK_MARABOU_INF_BOUNDS(get_lower_bound(var), get_upper_bound(var))
        }

        void tighten_upper_bound(MarabouVarIndex_type var, MarabouFloating_type ub) final { inputQuery.tightenUpperBound(var, ub); }

        // inputs:
        [[nodiscard]] inline std::size_t number_of_inputs() const { return inputQuery.getNumInputVariables(); }

        [[nodiscard]] inline bool has_continuous_inputs() const { return std::any_of(inputQuery._variableToInputIndex.begin(), inputQuery._variableToInputIndex.end(), [this](const auto& var_input) { return not is_marked_integer(var_input.first); }); }

        [[nodiscard]] inline bool is_marked_input(MarabouVarIndex_type var) const {
            PLAJA_ASSERT(exists(var))
            return inputQuery._variableToInputIndex.exists(var);
        }

        [[nodiscard]] inline InputIndex_type get_input_index(MarabouVarIndex_type var) const {
            PLAJA_ASSERT(is_marked_input(var))
            return inputQuery._variableToInputIndex.at(var);
        }

        template<bool check = true> inline void mark_input(MarabouVarIndex_type var) {
            PLAJA_ASSERT(exists(var))
            if (not check or not is_marked_input(var)) { inputQuery.markInputVariable(var, number_of_inputs()); }
        }

        template<bool check = true> inline void unmark_input(MarabouVarIndex_type var) {
            PLAJA_ASSERT(exists(var))
            if (not check or is_marked_input(var)) {
                auto input_index = get_input_index(var);
                inputQuery._variableToInputIndex.erase(var);
                inputQuery._inputIndexToVariable.erase(input_index);
            }
        }

        [[nodiscard]] std::list<MarabouVarIndex_type> generate_input_list() const;

        // integer:
        [[nodiscard]] inline bool is_marked_integer(MarabouVarIndex_type var) const {
            PLAJA_ASSERT(exists(var))
            return _query().isInteger(var);
        }

        inline void mark_integer(MarabouVarIndex_type var) {
            PLAJA_ASSERT(exists(var))
            PLAJA_ASSERT(not is_marked_output(var))
            _query().markInteger(var);
        }

        inline void unmark_integer(MarabouVarIndex_type var) {
            PLAJA_ASSERT(exists(var))
            PLAJA_ASSERT(not is_marked_output(var))
            _query().unmarkInteger(var);
        }

        [[nodiscard]] inline const Set<MarabouVarIndex_type>& _integer_vars() const { return _query().getIntegerVars(); }

        // outputs:
        [[nodiscard]] inline std::size_t number_of_outputs() const { return inputQuery.getNumOutputVariables(); }

        [[nodiscard]] inline bool is_marked_output(MarabouVarIndex_type var) const {
            PLAJA_ASSERT(exists(var))
            return inputQuery._variableToOutputIndex.exists(var);
        }

        [[nodiscard]] inline OutputIndex_type get_output_index(MarabouVarIndex_type var) const {
            PLAJA_ASSERT(is_marked_output(var))
            return inputQuery._variableToOutputIndex.at(var);
        }

        template<bool check = true> inline void mark_output(MarabouVarIndex_type var) {
            PLAJA_ASSERT(exists(var))
            if (not check or not is_marked_output(var)) { inputQuery.markOutputVariable(var, number_of_outputs()); }
        }

        template<bool check = true> inline void unmark_output(MarabouVarIndex_type var) {
            PLAJA_ASSERT(exists(var))
            if (not check or is_marked_output(var)) {
                auto output_index = get_output_index(var);
                inputQuery._variableToOutputIndex.erase(var);
                inputQuery._outputIndexToVariable.erase(output_index);
            }
        }

        [[nodiscard]] inline const InputQuery& _query() const { return inputQuery; }

        [[nodiscard]] inline InputQuery& _query() { return inputQuery; }

        // equations:

        [[nodiscard]] std::size_t get_num_equations() const final { return _query().getEquations().size(); }

        void add_equation(const Equation& eq) final { _query().addEquation(eq); }

        void add_equation(Equation&& eq) final { _query().addEquation(std::move(eq)); }

        void add_equations(std::list<Equation>&& equations) final { _query()._equations.append(std::move(equations)); }

        void remove_equation(std::size_t n) {
            auto& equations = inputQuery._equations;
            while (n-- > 0) { equations.popBack(); }
        }

        // pl constraints:

        [[nodiscard]] std::size_t get_num_pl_constraints() const final { return _query().getPiecewiseLinearConstraints().size(); }

        void add_pl_constraint(PiecewiseLinearConstraint* pl_constraint) final { _query().addPiecewiseLinearConstraint(pl_constraint); }

        void add_pl_constraint(const PiecewiseLinearConstraint* pl_constraint) final { _query().addPiecewiseLinearConstraint(pl_constraint->duplicateConstraint()); }

        void add_disjunction(DisjunctionConstraint* disjunction) final;

        void remove_pl_constraint(std::size_t n);

        [[nodiscard]] NLR::NetworkLevelReasoner* provide_nlr() final;
        void update_nlr() final;

        void dump() const;
        void save_query(const std::string& filename) const;

        class VariableIterator {
            friend MARABOU_IN_PLAJA::MarabouQuery;
        private:
            const MARABOU_IN_PLAJA::MarabouQuery* query;
            MarabouVarIndex_type currentVar; // NOLINT(*-use-default-member-init)

            explicit VariableIterator(const MARABOU_IN_PLAJA::MarabouQuery& query_):
                query(&query_)
                , currentVar(0) {}

        public:
            ~VariableIterator() = default;
            DELETE_CONSTRUCTOR(VariableIterator)

            inline void operator++() { ++currentVar; }

            [[nodiscard]] inline bool end() const { return currentVar >= query->_query().getNumberOfVariables(); }

            //
            [[nodiscard]] inline MarabouVarIndex_type var() const {
                PLAJA_ASSERT(query->exists(currentVar))
                return currentVar;
            }

            //
            [[nodiscard]] inline MarabouFloating_type lower_bound() const { return query->get_lower_bound(currentVar); }

            [[nodiscard]] inline MarabouFloating_type upper_bound() const { return query->get_upper_bound(currentVar); }

            //
            [[nodiscard]] inline bool is_integer() const { return query->is_marked_integer(currentVar); }

            [[nodiscard]] inline bool is_input() const { return query->is_marked_input(currentVar); }

            [[nodiscard]] inline bool is_output() const { return query->is_marked_output(currentVar); }
        };

        [[nodiscard]] inline VariableIterator variableIterator() const { return VariableIterator(*this); }

    };

}

#endif //PLAJA_MARABOU_QUERY_H
