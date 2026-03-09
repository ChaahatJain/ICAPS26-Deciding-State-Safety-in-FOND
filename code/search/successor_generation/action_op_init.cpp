//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "action_op_init.h"
#include "../../exception/runtime_exception.h"
#include "../../parser/ast/expression/expression.h"
#include "../../parser/ast/destination.h"
#include "../../parser/ast/edge.h"
#include "../../utils/floating_utils.h"
#include "../../utils/utils.h"
#include "../information/model_information.h"
#include "../states/state_base.h"

PreOpInit::PreOpInit(const Edge& edge_ref, AutomatonIndex_type automaton_index, const Model& model_ref):
    edgePtr(&edge_ref)
    , automatonIndex(automaton_index)
    , model(&model_ref)
    , initialised(false) {
    PLAJA_ASSERT(edgePtr)
}

PreOpInit::~PreOpInit() = default;

inline const Edge& PreOpInit::get_edge() const {
    PLAJA_ASSERT(edgePtr)
    return *edgePtr;
}

void PreOpInit::initialise(const StateBase& state) {
    PLAJA_ASSERT(edgePtr)

    const auto& edge = get_edge();

    const std::size_t l = edge.get_number_destinations();
    probabilities.reserve(l);
    destinations.reserve(l);

    STMT_IF_RUNTIME_CHECKS(PLAJA::Prob_type probability_sum = 0)
    for (std::size_t i = 0; i < l; ++i) {

        /* Probability. */
        const Expression* probability_exp = edge.get_destination(i)->get_probabilityExpression();
        if (probability_exp) {

            const auto probability = probability_exp->evaluate_floating(state);
            RUNTIME_ASSERT(PLAJA_FLOATS::lte(0.0, probability) and PLAJA_FLOATS::lte(probability, 1.0), PLAJA_UTILS::to_string_with_precision(probability, PLAJA::probabilityPrecision) + PLAJA_EXCEPTION::probabilityOutOfBounds)
            if (PLAJA_FLOATS::is_zero(probability, PLAJA::probabilityPrecision)) { continue; }
            STMT_IF_RUNTIME_CHECKS(probability_sum += probability)
            probabilities.push_back(probability);

        } else {
            STMT_IF_RUNTIME_CHECKS(probability_sum += 1)
            probabilities.push_back(1);
        }

        // Destination (only if probability > 0). */
        destinations.emplace_back(edge.get_destination(i));
    }
    RUNTIME_ASSERT(PLAJA_FLOATS::equal(probability_sum, 1.0, PLAJA::probabilityPrecision), PLAJA_EXCEPTION::probabilitySumIs + PLAJA_UTILS::to_string_with_precision(probability_sum, PLAJA::probabilityPrecision))

    initialised = true; // Mark initialised.
}

ActionOpID_type PreOpInit::compute_additive_action_op_id(SyncIndex_type sync_index) const {
    return ModelInformation::get_model_info(*model).compute_action_op_id_sync_additive(sync_index, automatonIndex, get_edge().get_id());
}

/** operator **********************************************************************************************************/

ActionOpInit::ActionOpInit(const Edge& edge, const StateBase& state, ActionOpID_type action_op_id, const Model& model):
    ActionOpBase(ACTION::silentAction, action_op_id)
    , syncId(SYNCHRONISATION::nonSyncID) {

    PLAJA_ASSERT(edges.empty())
    edges.push_back(&edge);

    PreOpInit pre_op(edge, AUTOMATON::invalid, model);
    pre_op.initialise(state);

    probabilities = std::move(pre_op.probabilities);

    /* Update. */
    const std::size_t l = pre_op.destinations.size();
    updates.reserve(l);
    for (std::size_t i = 0; i < l; ++i) { updates.emplace_back(model, pre_op.destinations[i]); }

    PLAJA_ASSERT(probabilities.size() == updates.size())
}

ActionOpInit::ActionOpInit(const PreOpInit& pre_op, ActionOpID_type additive_id_sync, SyncIndex_type sync_index):
    ActionOpBase(ModelInformation::action_id_to_label(ModelInformation::get_model_info(*pre_op.model).sync_index_to_action(sync_index)), additive_id_sync + pre_op.compute_additive_action_op_id(sync_index))
    , syncId(ModelInformation::sync_index_to_id(sync_index))
    , probabilities(pre_op.probabilities) {
    PLAJA_ASSERT(pre_op.is_initialised())

    PLAJA_ASSERT(edges.empty())
    edges.push_back(&pre_op.get_edge());

    /* Update. */
    const std::size_t l = pre_op.destinations.size();
    updates.reserve(l);
    for (std::size_t i = 0; i < l; ++i) { updates.emplace_back(*pre_op.model, pre_op.destinations[i]); }

    PLAJA_ASSERT(probabilities.size() == updates.size())
}

ActionOpInit::~ActionOpInit() = default;

/**
 * Adaption of produceWith method from https://github.com/prismmodelchecker/prism/blob/8394df8e1a0058cec02f47b0437b185e3ae667d7/prism/src/simulator/ChoiceListFlexi.java (PRISM, March 20, 2019).
*/
void ActionOpInit::product_with(const PreOpInit& pre_op) {
    PLAJA_ASSERT(not pre_op.destinations.empty()) // Operator must not be empty.

    opId += pre_op.compute_additive_action_op_id(ModelInformation::sync_id_to_index(syncId)); // Add impact of the edge to the pre_op-id-sum, this puts a restriction on how the action pre_op id is computed (e.g. as in ModelInformation).
    edges.push_back(&pre_op.get_edge());

    const std::size_t l = pre_op.destinations.size();
    const std::size_t ll = updates.size();

    /* Reserve. */
    updates.reserve(ll * l); // ll + ll * (l - 1)
    probabilities.reserve(ll * l);

    /* Loop through each (ith) element of new operator (skipping first). */
    for (std::size_t i = 1; i < l; ++i) {
        const auto pi = pre_op.probabilities[i];
        // loop through each (jth) element of existing operator
        for (std::size_t j = 0; j < ll; ++j) {
            // create new element (i,j) of product
            updates.emplace_back(updates[j]); // copy constructor explicitly implemented to preserve capacity (s. update.cpp)
            updates.back().add_destination(pre_op.destinations[i]);
            probabilities.push_back(pi * probabilities[j]);
        }
    }

    /* Modify elements of current operator to get (0,j) elements of product. */
    const auto pi = pre_op.probabilities[0];
    const Destination* dest = pre_op.destinations[0];
    for (std::size_t j = 0; j < ll; ++j) {
        updates[j].add_destination(dest);
        probabilities[j] = pi * probabilities[j];
    }

}



