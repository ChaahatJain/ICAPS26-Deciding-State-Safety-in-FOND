//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#include "veritas_query.h"
#include "../../utils/utils.h"
#include "veritas_context.h"

VERITAS_IN_PLAJA::QueryConstructable::QueryConstructable(std::shared_ptr<VERITAS_IN_PLAJA::Context> c):
    context(std::move(c)) {}

VERITAS_IN_PLAJA::QueryConstructable::~QueryConstructable() = default;

/**********************************************************************************************************************/

VERITAS_IN_PLAJA::VeritasQuery::VeritasQuery(std::shared_ptr<VERITAS_IN_PLAJA::Context> c):
    QueryConstructable(std::move(c)), inputQuery(veritas::AddTree(0, veritas::AddTreeType::REGR)) {}

VERITAS_IN_PLAJA::VeritasQuery::VeritasQuery(const VERITAS_IN_PLAJA::ContextView& context_view):
    QueryConstructable(context_view.share_context()), inputQuery(veritas::AddTree(0, veritas::AddTreeType::REGR)) {}

VERITAS_IN_PLAJA::VeritasQuery::VeritasQuery(const VeritasQuery& parent):
    QueryConstructable(parent.context)
    , inputQuery(parent._query()) {
    // PLAJA_ASSERT(inputQuery.getNumberOfVariables() <= context->get_number_of_variables()) // comment out maybe
}

VERITAS_IN_PLAJA::VeritasQuery::VeritasQuery(const veritas::AddTree& input_query/*, std::unordered_set<VeritasVarIndex_type>* integer_vars*/):
    QueryConstructable(std::make_shared<VERITAS_IN_PLAJA::Context>(/*, integer_vars*/))
    , inputQuery(input_query) {
    // PLAJA_ASSERT(inputQuery.getNumberOfVariables() == context->get_number_of_variables()) // comment out maybe
}

VERITAS_IN_PLAJA::VeritasQuery::VeritasQuery(const veritas::AddTree& input_query, std::shared_ptr<VERITAS_IN_PLAJA::Context> c, const std::vector<VeritasVarIndex_type>& input_indices): 
 QueryConstructable(std::move(c)), inputQuery(input_query) {
    std::cout << "Input indices in constructor: " << input_indices.size() << std::endl;
}


VERITAS_IN_PLAJA::VeritasQuery::~VeritasQuery() = default;
//


//

veritas::FlatBox VERITAS_IN_PLAJA::VeritasQuery::generate_input_list() const {
    veritas::FlatBox inputs;
    inputs.reserve(context->get_number_of_variables() + context->num_aux_vars());
    for (auto var_it = context->variableIterator(); !var_it.end(); ++var_it) {
        if (var_it.is_integer()) {
            veritas::Interval ival = {var_it.lower_bound() + VERITAS_IN_PLAJA::offset, var_it.upper_bound() + 1 + VERITAS_IN_PLAJA::offset};
            inputs.push_back(ival);
        } else {
            veritas::Interval ival = {var_it.lower_bound(), var_it.upper_bound() + 0.1};
            inputs.push_back(ival);
        }
    }
    for (int i = 0; i < context->num_aux_vars(); ++i) {
        veritas::Interval ival = {0, 2};
        inputs.push_back(ival);
    }
    return inputs;
}

 std::vector<VeritasVarIndex_type> VERITAS_IN_PLAJA::VeritasQuery::_integer_vars() const {
    std::vector<VeritasVarIndex_type> inputs;
    for (auto var_it = context->variableIterator(); !var_it.end(); ++var_it) {
        if (var_it.is_integer()) {
            inputs.push_back(var_it.var());
        }
    }
    return inputs;
}


void VERITAS_IN_PLAJA::VeritasQuery::dump() const { 
    std::stringstream query;
    addtree_to_json(query, inputQuery);
    std::cout << query.str() << "\n";
    std::flush(std::cout);
    }