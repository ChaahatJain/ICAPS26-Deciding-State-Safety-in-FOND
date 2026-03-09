//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "marabou_query.h"
#include "../../include/marabou_include/disjunction_constraint.h"
#include "../../utils/utils.h"
#include "marabou_context.h"

MARABOU_IN_PLAJA::QueryConstructable::QueryConstructable(std::shared_ptr<MARABOU_IN_PLAJA::Context> c):
    context(std::move(c)) {}

MARABOU_IN_PLAJA::QueryConstructable::~QueryConstructable() = default;

/**********************************************************************************************************************/

MARABOU_IN_PLAJA::MarabouQuery::MarabouQuery(std::shared_ptr<MARABOU_IN_PLAJA::Context> c):
    QueryConstructable(std::move(c)) {
    inputQuery.setNumberOfVariables(context->get_number_of_variables());
    for (auto var_it = context->variableIterator(); !var_it.end(); ++var_it) {
        // bounds
        inputQuery.setLowerBound(var_it.var(), var_it.lower_bound());
        inputQuery.setUpperBound(var_it.var(), var_it.upper_bound());
        // inputQuery.setSolutionValue()
        // input / output ?
        if (var_it.is_input()) { mark_input(var_it.var()); }
        if (var_it.is_output()) { mark_output(var_it.var()); }
        // integer ?
        if (var_it.is_integer()) { mark_integer(var_it.var()); }
    }
}

MARABOU_IN_PLAJA::MarabouQuery::MarabouQuery(const MARABOU_IN_PLAJA::ContextView& context_view):
    QueryConstructable(context_view.share_context()) {
    inputQuery.setNumberOfVariables(context_view.get_number_of_variables());
    for (auto var_it = context_view.variableIterator(); !var_it.end(); ++var_it) {
        // bounds
        inputQuery.setLowerBound(var_it.var(), var_it.lower_bound());
        inputQuery.setUpperBound(var_it.var(), var_it.upper_bound());
        // inputQuery.setSolutionValue()
        // input / output ?
        if (var_it.is_input()) { mark_input(var_it.var()); }
        if (var_it.is_output()) { mark_output(var_it.var()); }
        // integer ?
        if (var_it.is_integer()) { mark_integer(var_it.var()); }
    }
}

MARABOU_IN_PLAJA::MarabouQuery::MarabouQuery(const MarabouQuery& parent):
    QueryConstructable(parent.context)
    , inputQuery(parent._query()) {
    PLAJA_ASSERT(inputQuery.getNumberOfVariables() <= context->get_number_of_variables())
#ifndef NDEBUG
    for (auto var_it = variableIterator(); !var_it.end(); ++var_it) {
        // bounds
        PLAJA_ASSERT(context->get_lower_bound(var_it.var()) <= var_it.lower_bound());
        PLAJA_ASSERT(var_it.upper_bound() <= context->get_upper_bound(var_it.var()));
        // integer ?
        PLAJA_ASSERT(var_it.is_integer() == context->is_marked_integer(var_it.var()))
        // input / output ?
        PLAJA_ASSERT(not var_it.is_input() or context->is_marked_input(var_it.var())) // query may not consider some variable to be no input
        PLAJA_ASSERT(var_it.is_output() == context->is_marked_output(var_it.var()))
    }
#endif
}

MARABOU_IN_PLAJA::MarabouQuery::MarabouQuery(const InputQuery& input_query/*, std::unordered_set<MarabouVarIndex_type>* integer_vars*/):
    QueryConstructable(std::make_shared<MARABOU_IN_PLAJA::Context>(input_query/*, integer_vars*/))
    , inputQuery(input_query) {
    PLAJA_ASSERT(inputQuery.getNumberOfVariables() == context->get_number_of_variables())
#ifndef NDEBUG
    for (auto var_it = context->variableIterator(); !var_it.end(); ++var_it) {
        // bounds
        PLAJA_ASSERT(var_it.lower_bound() <= inputQuery.getLowerBound(var_it.var()));
        PLAJA_ASSERT(inputQuery.getUpperBound(var_it.var()) <= var_it.upper_bound());
        // integer ?
        PLAJA_ASSERT(var_it.is_integer() == is_marked_integer(var_it.var()))
        // input / output ?
        PLAJA_ASSERT(var_it.is_input() == is_marked_input(var_it.var()))
        PLAJA_ASSERT(var_it.is_output() == is_marked_output(var_it.var()))
    }
#endif
}

MARABOU_IN_PLAJA::MarabouQuery::~MarabouQuery() = default;

//

void MARABOU_IN_PLAJA::MarabouQuery::remove_aux_var_to(unsigned int new_num_vars) {
    PLAJA_ASSERT(new_num_vars <= inputQuery.getNumberOfVariables())

    for (MarabouVarIndex_type aux_var = inputQuery.getNumberOfVariables() - 1; aux_var >= new_num_vars; --aux_var) {
        PLAJA_ASSERT(aux_var != MARABOU_IN_PLAJA::noneVar)
        _query()._lowerBounds.erase(aux_var);
        _query()._upperBounds.erase(aux_var);
        PLAJA_ASSERT_EXPECT(not _query().isInteger(aux_var))
        PLAJA_ASSERT_EXPECT(not is_marked_input(aux_var))
        PLAJA_ASSERT_EXPECT(not is_marked_output(aux_var))
    }

    _query().setNumberOfVariables(new_num_vars);
}

//

void MARABOU_IN_PLAJA::MarabouQuery::add_disjunction(DisjunctionConstraint* disjunction) { add_pl_constraint(disjunction); }

void MARABOU_IN_PLAJA::MarabouQuery::remove_pl_constraint(std::size_t n) {
    auto& pl_constraints = inputQuery._plConstraints;
    while (n-- > 0) {
        delete pl_constraints.back();
        pl_constraints.popBack();
    }
}

//

NLR::NetworkLevelReasoner* MARABOU_IN_PLAJA::MarabouQuery::provide_nlr() {
    auto* nlr = inputQuery.getNetworkLevelReasoner();

    if (not nlr) {
        if (inputQuery.constructNetworkLevelReasoner()) { nlr = inputQuery.getNetworkLevelReasoner(); }
        else { return nullptr; }
    }

    // prepare
    nlr->clearConstraintTightenings();
    nlr->clearInputOutputConstraints();
    nlr->obtainCurrentBounds(_query());
    _query().collectInputConstraints(*nlr);
    _query().collectOutputConstraints(*nlr);

    return nlr;
}

void MARABOU_IN_PLAJA::MarabouQuery::update_nlr() {
    PLAJA_ASSERT(inputQuery.getNetworkLevelReasoner())
    inputQuery.getNetworkLevelReasoner()->obtainCurrentBounds(_query());
}

//

std::list<MarabouVarIndex_type> MARABOU_IN_PLAJA::MarabouQuery::generate_input_list() const {
    std::list<MarabouVarIndex_type> inputs;
    for (const auto var_input: inputQuery._variableToInputIndex) { inputs.push_back(var_input.first); }
    return inputs;
}

void MARABOU_IN_PLAJA::MarabouQuery::dump() const { this->inputQuery.dump(); }

void MARABOU_IN_PLAJA::MarabouQuery::save_query(const std::string& filename) const { inputQuery.saveQuery(filename, false); }