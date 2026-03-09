//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#ifndef PLAJA_VERITAS_CONTEXT_H
#define PLAJA_VERITAS_CONTEXT_H

#include <memory>
#include <map>
#include "using_veritas.h"
#include "forward_smt_veritas.h"
#include "../../utils/utils.h"
#include "../../assertions.h"
#include "../../assertions.h"
#include "../using_search.h"
#include <stack>
#include "../smt/base/smt_context.h"

namespace VERITAS_IN_PLAJA {

    /**
     * Context for Veritas queries.
    */
    class Context : public PLAJA::SmtContext {

    private:
        struct VariableInformation {
            VeritasFloating_type lb;
            VeritasFloating_type ub;
            bool isInteger;
            bool isIndicator;

            explicit VariableInformation(VeritasFloating_type lb_ = VERITAS_IN_PLAJA::negative_infinity, VeritasFloating_type ub_ = VERITAS_IN_PLAJA::infinity, bool is_integer = false):
                lb(lb_)
                , ub(ub_)
                , isInteger(is_integer) { CHECK_VERITAS_INF_BOUNDS(lb, ub) }

            ~VariableInformation() = default;
            DEFAULT_CONSTRUCTOR(VariableInformation)
        };

        static unsigned int contextCounter;

        unsigned int contextId;
        VeritasVarIndex_type nextVar;
        std::vector<VeritasVarIndex_type> EqAuxVars;
        std::map<ActionOpID_type, VeritasVarIndex_type> operatorAuxVars;
        std::map<VeritasVarIndex_type, VariableInformation> variableInformation; // ordered map to mark integer/float vars in ASC when loading state variables
        std::stack<std::map<VeritasVarIndex_type, VariableInformation>> variableInformationMap;
    public:
        Context();
        ~Context();
        DELETE_CONSTRUCTOR(Context)

        void push() { variableInformationMap.push(variableInformation); }
        void pop() { variableInformation = variableInformationMap.top(); variableInformationMap.pop(); }
        void clear() { std::stack<std::map<VeritasVarIndex_type, VariableInformation>>().swap(variableInformationMap); }
        std::unique_ptr<VeritasConstraint> trivially_true();
        std::unique_ptr<VeritasConstraint> trivially_false();

        inline bool operator==(const Context& other) const {
            PLAJA_ASSERT(contextId != other.contextId or this == &other)
            return contextId == other.contextId;
        }

        inline bool operator!=(const Context& other) const { return not((*this) == other); }

        [[nodiscard]] inline VeritasVarIndex_type max_var() const { return nextVar - 1; }

        [[nodiscard]] inline std::size_t get_number_of_variables() const { return variableInformation.size(); }

        [[nodiscard]] inline bool exists(VeritasVarIndex_type var) const {
            PLAJA_ASSERT(var < nextVar)
            return variableInformation.count(var);
        }

        inline VeritasVarIndex_type add_var(VeritasFloating_type lb = VERITAS_IN_PLAJA::negative_infinity, VeritasFloating_type ub = VERITAS_IN_PLAJA::infinity, bool is_integer = false) {
            auto var = nextVar++;
            variableInformation.emplace(var, VariableInformation(lb, ub, is_integer));
            return var;
        }

        inline VeritasVarIndex_type add_operator_indicator_var(ActionOpID_type action_op) {
            auto var = nextVar++;
            operatorAuxVars.emplace(action_op, var);
            return var; 
        }

        inline VeritasVarIndex_type add_eq_aux_var() {
            auto var = nextVar++;
            EqAuxVars.push_back(var);
            return var;
        }

        [[nodiscard]] inline VeritasFloating_type get_lower_bound(VeritasVarIndex_type var) const {
            PLAJA_ASSERT(exists(var))
            return variableInformation.at(var).lb;
        }

        [[nodiscard]] inline VeritasFloating_type get_upper_bound(VeritasVarIndex_type var) const {
            PLAJA_ASSERT(exists(var))
            return variableInformation.at(var).ub;
        }

        bool tighten_bounds(VeritasVarIndex_type var, VeritasFloating_type lb, VeritasFloating_type ub);

        [[nodiscard]] inline bool is_marked_integer(VeritasVarIndex_type var) const {
            if (not exists(var)) return false;
            PLAJA_ASSERT(exists(var))
            return variableInformation.at(var).isInteger;
        }

        inline void mark_integer(VeritasVarIndex_type var) {
            PLAJA_ASSERT(exists(var))
            variableInformation.at(var).isInteger = true;
        }
        VeritasVarIndex_type get_aux_var(ActionOpID_type op) { return operatorAuxVars[op]; }

        std::vector<VeritasVarIndex_type> get_aux_vars() const {
            std::vector<VeritasVarIndex_type> vars;
            vars.reserve(EqAuxVars.size() + operatorAuxVars.size());
            for (auto var : EqAuxVars) {
                vars.push_back(var);
            }
            for (auto var_pair : operatorAuxVars) {
                vars.push_back(var_pair.second);
            }
            return vars; 
        } 

        inline int num_aux_vars() { return EqAuxVars.size() + operatorAuxVars.size(); }

        class VariableIterator {
            friend VERITAS_IN_PLAJA::Context;
        private:
            std::map<VeritasVarIndex_type, VariableInformation>::const_iterator it;
            std::map<VeritasVarIndex_type, VariableInformation>::const_iterator itEnd;

            explicit VariableIterator(const std::map<VeritasVarIndex_type, VariableInformation>& var_info):
                it(var_info.cbegin())
                , itEnd(var_info.cend()) {}

        public:
            ~VariableIterator() = default;
            DELETE_CONSTRUCTOR(VariableIterator)

            inline void operator++() { ++it; }

            [[nodiscard]] inline bool end() const { return it == itEnd; }

            //
            [[nodiscard]] inline VeritasVarIndex_type var() const { return it->first; }

            //
            [[nodiscard]] inline VeritasFloating_type lower_bound() const { return it->second.lb; }

            [[nodiscard]] inline VeritasFloating_type upper_bound() const { return it->second.ub; }

            //
            [[nodiscard]] inline bool is_integer() const { return it->second.isInteger; }

        };

        [[nodiscard]] inline VariableIterator variableIterator() const { return VariableIterator(variableInformation); }

        FCT_IF_DEBUG(void dump() const;)

    };

    class ContextView {
    private:
        std::shared_ptr<Context> context;
        VeritasVarIndex_type lowerBoundVar; // NOLINT(*-use-default-member-init)
        VeritasVarIndex_type upperBoundVar;
        VeritasVarIndex_type lowerBoundInput; // NOLINT(*-use-default-member-init)
        VeritasVarIndex_type upperBoundInput;

    public:
        explicit ContextView(std::shared_ptr<Context> context_)
            :
            context(std::move(context_))
            , lowerBoundVar(0)
            , upperBoundVar(context->max_var())
            , lowerBoundInput(0)
            , upperBoundInput(context->max_var()) {}

        ~ContextView() = default;
        DELETE_CONSTRUCTOR(ContextView)

        [[nodiscard]] inline const Context& _context() const { return *context; }

        [[nodiscard]] inline std::shared_ptr<Context> share_context() const { return context; }

        inline void set_lower_bound_var(VeritasVarIndex_type lb) {
            PLAJA_ASSERT(PLAJA_UTILS::is_non_negative(lb))
            lowerBoundVar = lb;
        }

        inline void set_upper_bound_var(VeritasVarIndex_type ub) {
            PLAJA_ASSERT(ub <= context->max_var())
            upperBoundVar = ub;
        }

        inline void set_lower_bound_input(VeritasVarIndex_type lb) {
            PLAJA_ASSERT(PLAJA_UTILS::is_non_negative(lb))
            lowerBoundInput = lb;
        }

        inline void set_upper_bound_input(VeritasVarIndex_type ub) {
            PLAJA_ASSERT(ub <= context->max_var())
            upperBoundInput = ub;
        }

        [[nodiscard]] inline std::size_t get_number_of_variables() const { return upperBoundVar + 1; }

        class VariableIterator {
            friend VERITAS_IN_PLAJA::ContextView;
        private:
            const ContextView* contextView;
            Context::VariableIterator it;

            explicit VariableIterator(const ContextView& context_view):
                contextView(&context_view)
                , it(context_view._context().variableIterator()) { while (it.var() < contextView->lowerBoundVar) { ++it; } }

        public:
            ~VariableIterator() = default;
            DELETE_CONSTRUCTOR(VariableIterator)

            inline void operator++() { ++it; }

            [[nodiscard]] inline bool end() const { return it.end() or it.var() > contextView->upperBoundVar; }

            //
            [[nodiscard]] inline VeritasVarIndex_type var() const { return it.var(); }

            //
            [[nodiscard]] inline VeritasFloating_type lower_bound() const { return it.lower_bound(); }

            [[nodiscard]] inline VeritasFloating_type upper_bound() const { return it.upper_bound(); }

            //
            [[nodiscard]] inline bool is_integer() const { return it.is_integer(); }

        };

        [[nodiscard]] inline VariableIterator variableIterator() const { return VariableIterator(*this); }

    };

}

#endif //PLAJA_VERITAS_CONTEXT_H
