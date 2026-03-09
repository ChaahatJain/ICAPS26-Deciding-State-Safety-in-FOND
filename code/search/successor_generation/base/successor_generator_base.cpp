//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "successor_generator_base.h"
#include "../../../parser/ast/automaton.h"
#include "../../../parser/ast/edge.h"
#include "../../../parser/ast/model.h"
#include "../../../parser/ast/location.h"
#include "../../../parser/ast/transient_value.h"
#include "../../fd_adaptions/state.h"
#include "../../information/model_information.h"
#include "../update.h"

#ifdef ENABLE_TERMINAL_STATE_SUPPORT

#include "../../../parser/ast/expression/expression.h"
#include "../../factories/configuration.h"
#include "../../information/property_information.h"

#endif

SuccessorGeneratorBase::SuccessorGeneratorBase(const PLAJA::Configuration& config, const Model& model_ref):
    model(&model_ref)
    , modelInformation(&model_ref.get_model_information())
    , syncInformation(&get_model_information().get_synchronisation_information())
    , numberOfEnabledOps(0)
#ifdef ENABLE_TERMINAL_STATE_SUPPORT
    , terminalStateCondition(nullptr)
#endif
{
    MAYBE_UNUSED(config)

#ifdef ENABLE_TERMINAL_STATE_SUPPORT
    {
        const auto* prop_info = config.get_sharable_const<PropertyInformation>(PLAJA::SharableKey::PROP_INFO);
        if (prop_info) { terminalStateCondition = prop_info->get_terminal(); }
    }
#endif

    AutomatonIndex_type automaton_index;
    // initialise edges:
    edgesPerAutoPerLoc.resize(get_sync_information().num_automata_instances);
    std::size_t j;
    for (automaton_index = 0; automaton_index < get_sync_information().num_automata_instances; ++automaton_index) {
        const Automaton* automaton = get_model().get_automatonInstance(automaton_index);
        edgesPerAutoPerLoc[automaton_index].resize(automaton->get_number_locations());
        const std::size_t num_edges = automaton->get_number_edges();
        for (j = 0; j < num_edges; ++j) {
            const Edge* edge = automaton->get_edge(j);
            edgesPerAutoPerLoc[automaton_index][edge->get_location_value()].push_back(edge);
        }
    }
}

SuccessorGeneratorBase::~SuccessorGeneratorBase() = default;

/**********************************************************************************************************************/

struct SuccessorGeneratorEvaluator {

    template<typename State_t>
    inline static void evaluate_successor(const SuccessorGeneratorBase* this_, const Update& update, State_t* successor) {
        // Set transient variables to their initial value:
        this_->initialise_transient(*successor);
        successor->refresh_written_to(); // transient variables may be reassigned during the execution of the update

        // Execute update:
        update.evaluate(successor);

        // TODO here we would need to compute STEPS-cost

        // Reset transient variables w.r.t. successor locations:
        successor->refresh_written_to(); // for transients
        this_->execute_transient_values(*successor);
    }

};

/**********************************************************************************************************************/

#ifdef ENABLE_TERMINAL_STATE_SUPPORT

bool SuccessorGeneratorBase::is_terminal(const StateBase& state) const {
    PLAJA_ASSERT(PLAJA_GLOBAL::enableTerminalStateSupport)
    const auto* terminal = get_terminal_state_condition();
    return terminal and terminal->evaluate_integer(state);
}

#endif

void SuccessorGeneratorBase::initialise_transient(StateBase& state) const {
    const auto& initial_values = get_model_information().get_initial_values();
    for (auto index: get_model_information().get_transient_variables()) { state.template assign_int<true>(index, initial_values.get_int(index)); }
}

void SuccessorGeneratorBase::execute_transient_values(StateBase& state) const {
    // Assign each transient its initial value, valid as transient values in locations do not reference transient variables
    initialise_transient(state);
    state.refresh_written_to(); // only if *subsequently* a transient variable is assigned twice, this is a modelling error

    // If transient value in any current location assigns transient variable, overwrite
    const auto num_automata_instances = get_sync_information().num_automata_instances;
    std::size_t j, m;
    for (AutomatonIndex_type automaton_index = 0; automaton_index < num_automata_instances; ++automaton_index) {
        const PLAJA::integer loc_val = state.get_int(automaton_index);
        const Location* location = get_model().get_automatonInstance(automaton_index)->get_location(loc_val);
        m = location->get_number_transientValues();
        for (j = 0; j < m; ++j) { location->get_transientValue(j)->evaluate(&state, &state); }
    }
}

std::unique_ptr<State> SuccessorGeneratorBase::compute_successor(const Update& update, const State& source) const {
    if (get_model_information().get_transient_variables().empty()) { // special case
        // std::cout << "Inside successor update computation:"; source.dump(true);
        return update.evaluate(source);
    } else {
        std::unique_ptr<State> successor = source.to_construct();
        SuccessorGeneratorEvaluator::evaluate_successor(this, update, successor.get());
        successor->finalize();
        return successor;
    }
}

StateValues SuccessorGeneratorBase::compute_successor(const Update& update, const StateValues& source) const {
    if (get_model_information().get_transient_variables().empty()) { // special case
        return update.evaluate(source);
    } else {
        StateValues successor(source);
        SuccessorGeneratorEvaluator::evaluate_successor(this, update, &successor);
        return successor;
    }
}

FCT_IF_DEBUG(bool SuccessorGeneratorBase::has_transient_variables() const { return not get_model_information().get_transient_variables().empty(); })

/**********************************************************************************************************************/

SuccessorGeneratorBase::ActionIdIterator::ActionIdIterator(const SynchronisationInformation* sync_info, bool include_silent):
    currentAction(include_silent ? ACTION::silentAction : 0)
    , numActions(sync_info->num_actions) {
    PLAJA_ASSERT(ACTION::silentAction == -1)
    PLAJA_ASSERT(ACTION::silentLabel == ACTION::silentAction)
    PLAJA_ASSERT(ModelInformation::action_id_to_label(42) == 42) // Sanity.
}