//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "racetrack_model_info.h"
#include "../../../exception/constructor_exception.h"
#include "../../../option_parser/option_parser.h"
#include "../../../parser/ast/iterators/model_iterator.h"
#include "../../../parser/ast/action.h"
#include "../../../parser/ast/automaton.h"
#include "../../../parser/ast/edge.h"
#include "../../../parser/ast/composition.h"
#include "../../../parser/ast/synchronisation.h"
#include "../../../parser/ast/variable_declaration.h"
#include "../model_information.h"

//extern:
namespace PLAJA_GLOBAL { extern const PLAJA::OptionParser* optionParser; }
namespace PLAJA_OPTION { extern const std::string nn_for_racetrack; }
namespace PLAJA_UTILS {
    extern std::vector<std::string> split(const std::string& str, const std::string& delimiter);
    extern const std::string commaString;
    extern const std::string quoteString;
}

RacetrackModelInfo::RacetrackModelInfo(const Model& model):
    model(model)
    , xPosVar(nullptr)
    , yPosVar(nullptr)
    , xVelVar(nullptr)
    , yVelVar(nullptr) {
    nonDeterministicLabels.reserve(9);
    nonDeterministicOps.reserve(9);

    // Collect non-deterministic actions

    std::vector<std::string> non_deterministic_actions { "accel0_-1_-1", "accel0_-1_0", "accel0_-1_1", "accel0_0_-1", "accel0_0_0", "accel0_0_1", "accel0_1_-1", "accel0_1_0", "accel0_1_1" }; // x and y are inverted in the learning tool
    const Composition* system = model.get_system();

    for (const auto& action_name: non_deterministic_actions) { // for each non-det. action ...

        ActionID_type action_id = ACTION::nullAction;

        // action ID/label
        for (auto it = ModelIterator::actionIterator(model); !it.end(); ++it) {
            if (it->get_name() == action_name) {
                action_id = it->get_id();
                nonDeterministicLabels.push_back(action_id);
                nonDeterministicLabelsSet.insert(action_id);
                break;
            }
        }

        if (action_id == ACTION::nullAction) { break; } // invalid model

        // (single) corresponding action operator, i.e, racetrack is deterministic with respect to action labels
        for (SyncIndex_type sync_index = 0; sync_index < system->get_number_syncs(); ++sync_index) { // under the sync. constraints ...
            const Synchronisation* sync = system->get_sync(sync_index);
            if (sync->get_resultID() == action_id) { // for the corresponding sync. constraint ...
                PLAJA_ASSERT(sync->get_result() == action_name)
                ActionOpStructure op_str;
                op_str.syncID = sync->get_syncID();
                for (AutomatonIndex_type automaton_index = 0; automaton_index < sync->get_size_synchronise(); ++automaton_index) {
                    if (sync->get_syncActionID(automaton_index) != ACTION::nullAction) {
                        op_str.participatingEdges.emplace_back(automaton_index, extract_edge_id(automaton_index, sync->get_syncActionID(automaton_index)));
                    }
                }
                nonDeterministicOps.push_back(model.get_model_information().compute_action_op_id(op_str));
            }
        }

    }

    // sanity check:
    if (nonDeterministicLabels.size() != 9 || nonDeterministicOps.size() != 9) {
        throw ConstructorException("Racetrack missing/additional actions!");
    }


    // extract names of position variables
    std::string x_pos_name = "car0_x";
    std::string y_pos_name = "car0_y";
    if (PLAJA_GLOBAL::optionParser->has_value_option(PLAJA_OPTION::nn_for_racetrack)) {
        std::vector<std::string> split_vec = PLAJA_UTILS::split(PLAJA_GLOBAL::optionParser->get_value_option_string(PLAJA_OPTION::nn_for_racetrack), PLAJA_UTILS::commaString);
        if (split_vec.size() == 2) {
            x_pos_name = std::move(split_vec[0]);
            y_pos_name = std::move(split_vec[1]);
        } else {
            throw ConstructorException(PLAJA_UTILS::quoteString + PLAJA_OPTION::nn_for_racetrack + PLAJA_UTILS::quoteString + " does not specify valid variable names.");
        }
    }

    // collect variables of interest
    for (auto var_it = model.variableIterator(); !var_it.end(); ++var_it) {
        const VariableDeclaration* var_decl = var_it.variable();
        if (var_decl->get_name() == x_pos_name) {
            xPosVar = var_decl;
        } else if (var_decl->get_name() == y_pos_name) {
            yPosVar = var_decl;
        } else if (var_decl->get_name() == "car0_dx") {
            xVelVar = var_decl;
        } else if (var_decl->get_name() == "car0_dy") {
            yVelVar = var_decl;
        }
    }

    // sanity check:
    if (!(xPosVar && yPosVar && xVelVar && yVelVar)) {
        throw ConstructorException("Racetrack variables missing!");
    }
}

RacetrackModelInfo::~RacetrackModelInfo() = default;

EdgeID_type RacetrackModelInfo::extract_edge_id(AutomatonIndex_type automaton_index, ActionID_type action_id) const {
    PLAJA_ASSERT(action_id != ACTION::silentAction and action_id != ACTION::nullAction)

    const Automaton* automaton = model.get_automatonInstance(automaton_index);
    EdgeID_type edge_id = EDGE::nullEdge;
    for (std::size_t i = 0; i < automaton->get_number_edges(); ++i) {
        const Edge* edge = automaton->get_edge(i);
        if (edge->get_action_id() == action_id) {
            if (edge_id != EDGE::nullEdge) { throw PlajaException(edge->get_action_name() + " labels more than one edge in " + automaton->get_name()); }
            edge_id = edge->get_id();
            PLAJA_ASSERT(edge_id != EDGE::nullEdge)
        }
    }

    if (edge_id == EDGE::nullEdge) {
        PLAJA_ASSERT(action_id == model.get_action(action_id)->get_id())
        throw PlajaException("No edge for action with id " + model.get_action(action_id)->get_name() + " in " + automaton->get_name());
    }

    return edge_id;
}

