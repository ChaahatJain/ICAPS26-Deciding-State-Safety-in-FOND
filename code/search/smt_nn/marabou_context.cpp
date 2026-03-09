//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "gurobi_c++.h"
#include "marabou_context.h"
#include "../../include/marabou_include/input_query.h"
#include "../../option_parser/option_parser.h"
#include "../../utils/utils.h"
#include "constraints_in_marabou.h"

namespace PLAJA_OPTION {

    extern const std::string perOpAppDis;
    extern const std::string marabouDoSlackForLabelSel;
    extern const std::string marabouOptSlackForLabelSel;
    extern const std::string marabouPreOptDis;

}

/**********************************************************************************************************************/

unsigned int MARABOU_IN_PLAJA::Context::contextCounter = 0;

MARABOU_IN_PLAJA::Context::Context():
    PLAJA::SmtContext()
    , contextId(contextCounter++)
    , nextVar(0)
#ifdef MARABOU_DIS_BASELINE_SUPPORT
    , perOpAppDis(not PLAJA_GLOBAL::debug or PLAJA_GLOBAL::optionParser ? PLAJA_GLOBAL::get_bool_option(PLAJA_OPTION::perOpAppDis) : PLAJA_OPTION_DEFAULT::perOpAppDis)
    , doSlackForLabelSel(not PLAJA_GLOBAL::debug or PLAJA_GLOBAL::optionParser ? PLAJA_GLOBAL::get_bool_option(PLAJA_OPTION::marabouDoSlackForLabelSel) : PLAJA_OPTION_DEFAULT::marabouDoSlackForLabelSel)
    , optSlackForLabelSel(not PLAJA_GLOBAL::debug or PLAJA_GLOBAL::optionParser ? PLAJA_GLOBAL::get_bool_option(PLAJA_OPTION::marabouOptSlackForLabelSel) : PLAJA_OPTION_DEFAULT::marabouOptSlackForLabelSel)
    , preOptDis(not PLAJA_GLOBAL::debug or PLAJA_GLOBAL::optionParser ? PLAJA_GLOBAL::get_bool_option(PLAJA_OPTION::marabouPreOptDis) : PLAJA_OPTION_DEFAULT::marabouPreOptDis)
#endif
{
    init_grb_env();
}

MARABOU_IN_PLAJA::Context::Context(const InputQuery& input_query/*, const std::unordered_set<MarabouVarIndex_type>* integer_vars*/):
    contextId(contextCounter++)
    , nextVar(0)
#ifdef MARABOU_DIS_BASELINE_SUPPORT
    , perOpAppDis(not PLAJA_GLOBAL::debug or PLAJA_GLOBAL::optionParser ? PLAJA_GLOBAL::get_bool_option(PLAJA_OPTION::perOpAppDis) : PLAJA_OPTION_DEFAULT::perOpAppDis)
    , doSlackForLabelSel(not PLAJA_GLOBAL::debug or PLAJA_GLOBAL::optionParser ? PLAJA_GLOBAL::get_bool_option(PLAJA_OPTION::marabouDoSlackForLabelSel) : PLAJA_OPTION_DEFAULT::marabouDoSlackForLabelSel)
    , optSlackForLabelSel(not PLAJA_GLOBAL::debug or PLAJA_GLOBAL::optionParser ? PLAJA_GLOBAL::get_bool_option(PLAJA_OPTION::marabouOptSlackForLabelSel) : PLAJA_OPTION_DEFAULT::marabouOptSlackForLabelSel)
    , preOptDis(not PLAJA_GLOBAL::debug or PLAJA_GLOBAL::optionParser ? PLAJA_GLOBAL::get_bool_option(PLAJA_OPTION::marabouPreOptDis) : PLAJA_OPTION_DEFAULT::marabouPreOptDis)
#endif
{
    init_grb_env();

    for (MarabouVarIndex_type var = 0; var < input_query.getNumberOfVariables(); ++var) {
        add_var(input_query.getLowerBound(var), input_query.getUpperBound(var),
                input_query.isInteger(var), // integer_vars and integer_vars->count(var),
                input_query._variableToInputIndex.exists(var), input_query._variableToOutputIndex.exists(var));
    }
}

MARABOU_IN_PLAJA::Context::~Context() = default;

#ifdef SUPPORT_GUROBI

void MARABOU_IN_PLAJA::Context::init_grb_env() {
    grbEnv = std::make_unique<GRBEnv>(true);
    grbEnv->set(GRB_IntParam_OutputFlag, 0); // Suppress output.
    grbEnv->start();
    GlobalConfiguration::GRB_ENV = grbEnv.get(); // Share as default;
}

#endif

/* */

std::unique_ptr<MarabouConstraint> MARABOU_IN_PLAJA::Context::trivially_true() { return std::make_unique<ConjunctionInMarabou>(*this); }

std::unique_ptr<MarabouConstraint> MARABOU_IN_PLAJA::Context::trivially_false() {
    PLAJA_ASSERT(get_number_of_variables() > 0)
    std::unique_ptr<ConjunctionInMarabou> conjunction_in_marabou(new ConjunctionInMarabou(*this));
    conjunction_in_marabou->add_bound(0, Tightening::LB, 1);
    conjunction_in_marabou->add_bound(0, Tightening::UB, -1);
    PLAJA_ASSERT(conjunction_in_marabou->is_trivially_false())
    return conjunction_in_marabou;
}

/* */

bool MARABOU_IN_PLAJA::Context::tighten_bounds(MarabouVarIndex_type var, MarabouFloating_type lb, MarabouFloating_type ub) {
    PLAJA_ASSERT(exists(var))
    auto& var_info = variableInformation[var];
    const bool differ = var_info.lb != lb or ub != var_info.ub;
    // tightest possible domain
    if (var_info.lb < lb) { var_info.lb = lb; }
    if (ub < var_info.ub) { var_info.ub = ub; }
    CHECK_MARABOU_INF_BOUNDS(var_info.lb, var_info.ub)
    return differ;
}

//

#ifndef NDEBUG

void MARABOU_IN_PLAJA::Context::dump() const {

    for (auto it = variableIterator(); !it.end(); ++it) {
        PLAJA_LOG(PLAJA_UTILS::string_f("Var %i, lb: %f, ub: %f, input: %i, output: %i, integer: %i", it.var(), it.lower_bound(), it.upper_bound(), it.is_input(), it.is_output(), it.is_integer()));
    }

}

#endif
