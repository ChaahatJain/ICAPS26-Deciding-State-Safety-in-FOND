//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_MARABOU_CONTEXT_H
#define PLAJA_MARABOU_CONTEXT_H

#include <map>
#include <unordered_set>
#include "../../include/marabou_include/forward_marabou.h"
#include "../../include/ct_config_const.h"
#include "../../utils/utils.h"
#include "../../assertions.h"
#include "../smt/base/smt_context.h"
#include "../using_search.h"
#include "using_marabou.h"
#include "forward_smt_nn.h"

class GRBEnv; // Forward declaration Gurobi.

/* Placed here for convenience. */
namespace PLAJA_OPTION_DEFAULT {

    constexpr bool perOpAppDis(PLAJA_GLOBAL::enableApplicabilityFiltering);
    constexpr bool marabouDoSlackForLabelSel(PLAJA_GLOBAL::enableApplicabilityFiltering);
    constexpr bool marabouOptSlackForLabelSel(PLAJA_GLOBAL::enableApplicabilityFiltering);
    constexpr bool marabouPreOptDis(true);

}

namespace MARABOU_IN_PLAJA {

    /**
     * Context for Marabou queries.
    */
    class Context: public PLAJA::SmtContext {

    private:
        struct VariableInformation {
            MarabouFloating_type lb;
            MarabouFloating_type ub;
            bool isInteger;
            bool isInput;
            bool isOutput;

            explicit VariableInformation(MarabouFloating_type lb_ = FloatUtils::negativeInfinity(), MarabouFloating_type ub_ = FloatUtils::infinity(), bool is_integer = false, bool is_input = false, bool is_output = false):
                lb(lb_)
                , ub(ub_)
                , isInteger(is_integer)
                , isInput(is_input)
                , isOutput(is_output) { CHECK_MARABOU_INF_BOUNDS(lb, ub) }

            ~VariableInformation() = default;
            DEFAULT_CONSTRUCTOR(VariableInformation)
        };

        static unsigned int contextCounter;

        unsigned int contextId;
        MarabouVarIndex_type nextVar;
        std::map<MarabouVarIndex_type, VariableInformation> variableInformation; // ordered map to mark input/output vars in ASC when loading query (-> preserve encoding of old version)

#ifdef SUPPORT_GUROBI

        std::unique_ptr<GRBEnv> grbEnv;

        void init_grb_env();
#else

        inline void init_grb_env() { PLAJA_ABORT }

#endif

#ifdef MARABOU_DIS_BASELINE_SUPPORT
        bool perOpAppDis;
        bool doSlackForLabelSel;
        bool optSlackForLabelSel;
        bool preOptDis;
#endif

    public:
        Context();
        explicit Context(const InputQuery& input_query/*, const std::unordered_set<MarabouVarIndex_type>* integer_vars = nullptr*/); // customize context for query
        ~Context() override;
        DELETE_CONSTRUCTOR(Context)

#ifdef MARABOU_DIS_BASELINE_SUPPORT

        [[nodiscard]] inline bool do_per_op_app_dis() const {
            PLAJA_ASSERT(PLAJA_GLOBAL::enableApplicabilityFiltering)
            return perOpAppDis;
        }

        [[nodiscard]] inline bool do_slack_for_label_sel() const {
            PLAJA_ASSERT(PLAJA_GLOBAL::enableApplicabilityFiltering)
            PLAJA_ASSERT(PLAJA_GLOBAL::marabouDisBaselineSupport)
            return doSlackForLabelSel;
        }

        [[nodiscard]] inline bool do_opt_slack_for_label_sel() const {
            PLAJA_ASSERT(PLAJA_GLOBAL::enableApplicabilityFiltering)
            return optSlackForLabelSel;
        }

        [[nodiscard]] inline bool do_pre_opt_dis() const { return preOptDis; }

#else

        [[nodiscard]] inline bool do_per_op_app_dis() const {
            PLAJA_ASSERT(PLAJA_GLOBAL::enableApplicabilityFiltering)
            return PLAJA_OPTION_DEFAULT::perOpAppDis;
        }

        [[nodiscard]] inline bool do_slack_for_label_sel() const { PLAJA_ABORT }

        [[nodiscard]] inline bool do_opt_slack_for_label_sel() const {
            PLAJA_ASSERT(PLAJA_GLOBAL::enableApplicabilityFiltering)
            return PLAJA_OPTION_DEFAULT::marabouOptSlackForLabelSel;
        }

        [[nodiscard]] inline bool do_pre_opt_dis() const { return PLAJA_OPTION_DEFAULT::marabouPreOptDis; }

#endif

#ifdef SUPPORT_GUROBI

        [[nodiscard]] GRBEnv* share_grb_env() const { return grbEnv.get(); }

#else

        [[nodiscard]] GRBEnv* share_grb_env() const { PLAJA_ABORT }

#endif

        std::unique_ptr<MarabouConstraint> trivially_true();
        std::unique_ptr<MarabouConstraint> trivially_false();

        inline bool operator==(const Context& other) const {
            PLAJA_ASSERT(contextId != other.contextId or this == &other)
            return contextId == other.contextId;
        }

        inline bool operator!=(const Context& other) const { return not((*this) == other); }

        [[nodiscard]] inline MarabouVarIndex_type max_var() const { return nextVar - 1; }

        [[nodiscard]] inline std::size_t get_number_of_variables() const { return variableInformation.size(); }

        [[nodiscard]] inline bool exists(MarabouVarIndex_type var) const {
            PLAJA_ASSERT(var < nextVar)
            return variableInformation.count(var);
        }

        inline MarabouVarIndex_type add_var(MarabouFloating_type lb = FloatUtils::negativeInfinity(), MarabouFloating_type ub = FloatUtils::infinity(), bool is_integer = false, bool is_input = false, bool is_output = false) {
            auto var = nextVar++;
            variableInformation.emplace(var, VariableInformation(lb, ub, is_integer, is_input, is_output));
            return var;
        }

        [[nodiscard]] inline MarabouFloating_type get_lower_bound(MarabouVarIndex_type var) const {
            PLAJA_ASSERT(exists(var))
            return variableInformation.at(var).lb;
        }

        [[nodiscard]] inline MarabouFloating_type get_upper_bound(MarabouVarIndex_type var) const {
            PLAJA_ASSERT(exists(var))
            return variableInformation.at(var).ub;
        }

        bool tighten_bounds(MarabouVarIndex_type var, MarabouFloating_type lb, MarabouFloating_type ub);

        [[nodiscard]] inline bool is_marked_integer(MarabouVarIndex_type var) const {
            PLAJA_ASSERT(exists(var))
            return variableInformation.at(var).isInteger;
        }

        inline void mark_integer(MarabouVarIndex_type var) {
            PLAJA_ASSERT(exists(var))
            PLAJA_ASSERT(not is_marked_output(var))
            variableInformation.at(var).isInteger = true;
        }

        [[nodiscard]] inline bool is_marked_input(MarabouVarIndex_type var) const {
            PLAJA_ASSERT(exists(var))
            return variableInformation.at(var).isInput;
        }

        inline void mark_input(MarabouVarIndex_type var) {
            PLAJA_ASSERT(exists(var))
            variableInformation.at(var).isInput = true;
        }

        [[nodiscard]] inline bool is_marked_output(MarabouVarIndex_type var) const {
            PLAJA_ASSERT(exists(var))
            return variableInformation.at(var).isOutput;
        }

        inline void mark_output(MarabouVarIndex_type var) {
            PLAJA_ASSERT(exists(var))
            variableInformation.at(var).isOutput = true;
        }

        class VariableIterator {
            friend MARABOU_IN_PLAJA::Context;
        private:
            std::map<MarabouVarIndex_type, VariableInformation>::const_iterator it;
            std::map<MarabouVarIndex_type, VariableInformation>::const_iterator itEnd;

            explicit VariableIterator(const std::map<MarabouVarIndex_type, VariableInformation>& var_info):
                it(var_info.cbegin())
                , itEnd(var_info.cend()) {}

        public:
            ~VariableIterator() = default;
            DELETE_CONSTRUCTOR(VariableIterator)

            inline void operator++() { ++it; }

            [[nodiscard]] inline bool end() const { return it == itEnd; }

            //
            [[nodiscard]] inline MarabouVarIndex_type var() const { return it->first; }

            //
            [[nodiscard]] inline MarabouFloating_type lower_bound() const { return it->second.lb; }

            [[nodiscard]] inline MarabouFloating_type upper_bound() const { return it->second.ub; }

            //
            [[nodiscard]] inline bool is_integer() const { return it->second.isInteger; }

            [[nodiscard]] inline bool is_input() const { return it->second.isInput; }

            [[nodiscard]] inline bool is_output() const { return it->second.isOutput; }
        };

        [[nodiscard]] inline VariableIterator variableIterator() const { return VariableIterator(variableInformation); }

        FCT_IF_DEBUG(void dump() const;)

    };

/**********************************************************************************************************************/

    class ContextView {
    private:
        std::shared_ptr<Context> context;
        MarabouVarIndex_type lowerBoundVar; // NOLINT(*-use-default-member-init)
        MarabouVarIndex_type upperBoundVar;
        MarabouVarIndex_type lowerBoundInput; // NOLINT(*-use-default-member-init)
        MarabouVarIndex_type upperBoundInput;

    public:
        explicit ContextView(std::shared_ptr<Context> context_):
            context(std::move(context_))
            , lowerBoundVar(0)
            , upperBoundVar(context->max_var())
            , lowerBoundInput(0)
            , upperBoundInput(context->max_var()) {}

        ~ContextView() = default;
        DELETE_CONSTRUCTOR(ContextView)

        [[nodiscard]] inline const Context& _context() const { return *context; }

        [[nodiscard]] inline std::shared_ptr<Context> share_context() const { return context; }

        inline void set_lower_bound_var(MarabouVarIndex_type var_lb) {
            PLAJA_ASSERT(PLAJA_UTILS::is_non_negative(var_lb))
            lowerBoundVar = var_lb;
        }

        inline void set_upper_bound_var(MarabouVarIndex_type var_ub) {
            PLAJA_ASSERT(var_ub <= context->max_var())
            upperBoundVar = var_ub;
        }

        inline void set_lower_bound_input(MarabouVarIndex_type var_lb) {
            PLAJA_ASSERT(PLAJA_UTILS::is_non_negative(var_lb))
            lowerBoundInput = var_lb;
        }

        inline void set_upper_bound_input(MarabouVarIndex_type var_ub) {
            PLAJA_ASSERT(var_ub <= context->max_var())
            upperBoundInput = var_ub;
        }

        [[nodiscard]] inline std::size_t get_number_of_variables() const { return upperBoundVar + 1; }

        class VariableIterator {
            friend MARABOU_IN_PLAJA::ContextView;
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
            [[nodiscard]] inline MarabouVarIndex_type var() const { return it.var(); }

            //
            [[nodiscard]] inline MarabouFloating_type lower_bound() const { return it.lower_bound(); }

            [[nodiscard]] inline MarabouFloating_type upper_bound() const { return it.upper_bound(); }

            //
            [[nodiscard]] inline bool is_integer() const { return it.is_integer(); }

            [[nodiscard]] inline bool is_input() const { return contextView->lowerBoundInput <= it.var() and it.var() <= contextView->upperBoundInput and it.is_input(); }

            [[nodiscard]] inline bool is_output() const { return it.is_output(); }
        };

        [[nodiscard]] inline VariableIterator variableIterator() const { return VariableIterator(*this); }

    };

}

#endif //PLAJA_MARABOU_CONTEXT_H
