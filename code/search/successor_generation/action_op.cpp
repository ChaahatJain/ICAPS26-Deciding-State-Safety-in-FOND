//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "action_op.h"
#include "../../exception/runtime_exception.h"
#include "../../parser/ast/expression/integer_value_expression.h"
#include "../../parser/ast/automaton.h"
#include "../../parser/ast/destination.h"
#include "../../parser/ast/edge.h"
#include "../../parser/ast/model.h"
#include "../../utils/floating_utils.h"
#include "../../utils/utils.h"
#include "../states/state_base.h"
#include "../information/model_information.h"

namespace PLAJA_GLOBAL { extern const Model* currentModel; }

/**********************************************************************************************************************/

ActionOp::ProbabilityStructure::ProbabilityStructure():
    probability(0) {}

inline void ActionOp::ProbabilityStructure::add_factor(const Expression* factor) {
    if (factor) { multiplication.push_back(factor); }
}

ActionOp::ProbabilityStructure::ProbabilityStructure(std::vector<const Destination*>& destinations):
    probability(-1) {
    for (const auto& dest: destinations) { add_factor(dest->get_probabilityExpression()); }
}

ActionOp::ProbabilityStructure::~ProbabilityStructure() = default;

[[nodiscard]] PLAJA::Prob_type ActionOp::ProbabilityStructure::compute(const StateBase* state) const {
    PLAJA::Prob_type probability_product = 1;
    for (const auto* factor: multiplication) {
        const auto prob_fac = factor->evaluateFloating(state);
        RUNTIME_ASSERT(PLAJA_FLOATS::lte(0.0, prob_fac) and PLAJA_FLOATS::lte(prob_fac, 1.0), PLAJA_UTILS::to_string_with_precision(prob_fac, PLAJA::floatingPrecision) + PLAJA_EXCEPTION::probabilityOutOfBounds)
        probability_product *= prob_fac;
    }
    return probability_product;
}

/**********************************************************************************************************************/

ActionOp::ActionOp(const Edge* edge, ActionOpID_type additive_id, const Model& model, SyncID_type sync_id):
    ActionOpBase(ModelInformation::action_id_to_label(model.get_model_information().sync_id_to_action(sync_id)), additive_id) {

    PLAJA_ASSERT(edge)

    if (sync_id != SYNCHRONISATION::nonSyncID) {
        edges.reserve(model.get_model_information().get_synchronisation_information().synchronisations[ModelInformation::sync_id_to_index(sync_id)].size());
    }
    edges.push_back(edge);

    const std::size_t l = edge->get_number_destinations();
    probabilityStructures.resize(l);
    updates.reserve(l);

    for (std::size_t i = 0; i < l; ++i) {
        probabilityStructures[i].add_factor(edge->get_destination(i)->get_probabilityExpression());
        updates.emplace_back(model, edge->get_destination(i));
    }
}

ActionOp::ActionOp(std::vector<const Edge*> edges_r, SyncID_type sync_id, ActionOpID_type op_id, const Model& model):
    ActionOpBase(ModelInformation::action_id_to_label(model.get_model_information().sync_id_to_action(sync_id)), op_id) {

    edges = std::move(edges_r);
    PLAJA_ASSERT(not edges.empty())

    // for reservation
    std::size_t op_size = 1;
    for (const auto* edge: edges) { op_size *= edge->get_number_destinations(); }

    // compute updates structures:
    probabilityStructures.reserve(op_size);
    updates.reserve(op_size);
    std::vector<const Destination*> destinations;
    compute_updates(destinations, 0, model);
}

ActionOp::~ActionOp() = default;

/**********************************************************************************************************************/

/**
 * Adaption of product_with in action_op_init.cpp,
 * which is an adaption of produceWith method from https://github.com/prismmodelchecker/prism/blob/8394df8e1a0058cec02f47b0437b185e3ae667d7/prism/src/simulator/ChoiceListFlexi.java (PRISM, March 20, 2019).
*/
void ActionOp::product_with(const Edge* edge) {
    const std::size_t l = edge->get_number_destinations();
    const std::size_t ll = updates.size();
    // reserve:
    updates.reserve(ll * l); // ll + ll * (l - 1)
    probabilityStructures.reserve(ll * l);
    // loop through each (ith) element of new operator (skipping first)
    for (std::size_t i = 1; i < l; ++i) {
        const auto* destination = edge->get_destination(i);
        const auto* probability = destination->get_probabilityExpression();
        // loop through each (jth) element of existing operator
        for (std::size_t j = 0; j < ll; ++j) {
            // create new element (i,j) of product
            updates.emplace_back(updates[j]); // copy constructor explicitly implemented to preserve capacity (s. update.cpp)
            updates.back().add_destination(destination);
            probabilityStructures.emplace_back(probabilityStructures[j]);
            probabilityStructures.back().add_factor(probability);
        }
    }
    // modify elements of current operator to get (0,j) elements of product
    PLAJA_ASSERT(edge->get_number_destinations() > 0)
    const auto* destination = edge->get_destination(0);
    const auto* probability = destination->get_probabilityExpression();
    for (std::size_t j = 0; j < ll; ++j) {
        updates[j].add_destination(destination);
        probabilityStructures[j].add_factor(probability);
    }
}

void ActionOp::product_with(const Edge* edge, ActionOpID_type additive_id) {
    edges.push_back(edge);
    opId += additive_id; // add impact of the edge to the operator-ID-sum, this puts a restriction on how the operator ID is computed (e.g. as in ModelInformation)
    product_with(edge);
}

/**********************************************************************************************************************/

std::unique_ptr<ActionOp> ActionOp::construct_operator(const ActionOpStructure& action_op_structure, const Model& model) {
    const ActionOpID_type op_id = model.get_model_information().compute_action_op_id(action_op_structure);

    std::vector<const Edge*> edges_;
    for (const auto& [automaton_index, edge_id]: action_op_structure.participatingEdges) {
        edges_.push_back(model.get_automatonInstance(automaton_index)->get_edge_by_id(edge_id));
    }

    return std::unique_ptr<ActionOp>(new ActionOp(edges_, action_op_structure.syncID, op_id, model));
}

std::unique_ptr<ActionOp> ActionOp::construct_operator(std::vector<const Edge*> edges_, SyncID_type sync_id, const Model& model) {
    ActionOpStructure action_op_structure;
    action_op_structure.syncID = sync_id;
    for (const auto* edge: edges_) {
        action_op_structure.participatingEdges.emplace_back(edge->get_location_index(), edge->get_id());
    }
    const ActionOpID_type op_id = model.get_model_information().compute_action_op_id(action_op_structure);

    return std::unique_ptr<ActionOp>(new ActionOp(std::move(edges_), sync_id, op_id, model));
}

void ActionOp::compute_updates(std::vector<const Destination*>& destinations, std::size_t index, const Model& model) { // NOLINT(misc-no-recursion)
    if (index == edges.size()) {
        probabilityStructures.emplace_back(destinations);
        updates.emplace_back(model, destinations);
    } else {
        for (auto it = edges[index]->destinationIterator(); !it.end(); ++it) {
            destinations.push_back(it());
            compute_updates(destinations, index + 1, model);
            destinations.pop_back();
        }
    }
}

void ActionOp::initialise(const StateBase* source_state) {
    PLAJA_ASSERT(source_state)
    for (auto& prob_struct: probabilityStructures) { prob_struct.initialise(source_state); }
#ifdef RUNTIME_CHECKS
    PLAJA::Prob_type probability_sum = 0;
    for (auto& prob_struct: probabilityStructures) { probability_sum += prob_struct.probability; }
    RUNTIME_ASSERT(PLAJA_FLOATS::equal(probability_sum, 1.0, PLAJA::probabilityPrecision), PLAJA_EXCEPTION::probabilitySumIs + PLAJA_UTILS::to_string_with_precision(probability_sum, PLAJA::floatingPrecision))
#endif
}

PLAJA::Prob_type ActionOp::has_zero_probability(UpdateIndex_type index) const { return PLAJA_FLOATS::is_zero(get_cached_probability(index), PLAJA::probabilityPrecision); }

bool ActionOp::is_enabled(const StateBase& state) const {
    for (const Edge* edge: edges) {
        if (state.get_int(edge->get_location_index()) != edge->get_location_value()) { return false; }
        const auto* guard = edge->get_guardExpression();
        if (guard and not guard->evaluateInteger(&state)) { return false; }
    }
    return true;
}

PLAJA::Prob_type ActionOp::evaluate_probability(UpdateIndex_type index, const StateBase& state) const {
    PLAJA_ASSERT(index < probabilityStructures.size())
    return probabilityStructures[index].compute(&state);
}

bool ActionOp::has_zero_probability(UpdateIndex_type index, const StateBase& state) const {
    return PLAJA_FLOATS::is_zero(evaluate_probability(index, state), PLAJA::probabilityPrecision);
}
