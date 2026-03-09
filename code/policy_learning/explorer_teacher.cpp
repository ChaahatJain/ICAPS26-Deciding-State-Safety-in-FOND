//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "explorer_teacher.h"
#include "../exception/constructor_exception.h"
#include "../option_parser/plaja_options.h"
#include "../parser/ast/model.h"
#include "../search/factories/non_prob_search/non_prob_search_options.h"
#include "../search/factories/configuration.h"
#include "../search/factories/search_engine_factory.h"
#include "../search/information/property_information.h"
#include "../search/non_prob_search/space_explorer.h"
#include "../search/states/state_values.h"
#include "../utils/structs_utils/update_op_structure.h"
#include "../globals.h"

namespace POLICY_TEACHER {

    std::unique_ptr<PolicyTeacher> construct_explorer_teacher(const PLAJA::Configuration& config) {
        return std::make_unique<ExplorerTeacher>(config);
    }

}

/**********************************************************************************************************************/

ExplorerTeacher::ExplorerTeacher(const PLAJA::Configuration& config):
    PolicyTeacher(config) {

    if (teacherModel->get_number_properties() != 1) {
        throw ConstructorException("Explorer teacher: Goal ambiguity more than one property in teacher model.");
    }

    auto factory = SEARCH_ENGINE_FACTORY::construct_explorer_factory();
    factory->supports_model(*teacherModel);

    configExplorer = std::make_unique<PLAJA::Configuration>(config);
    configExplorer->delete_sharable(PLAJA::SharableKey::STATS);
    configExplorer->delete_sharable_ptr(PLAJA::SharableKey::STATS);
    configExplorer->disable_value_option(PLAJA_OPTION::stats_file);
    configExplorer->set_flag(PLAJA_OPTION::save_intermediate_stats, false);
    configExplorer->disable_value_option(PLAJA_OPTION::nn_interface);
    configExplorer->set_bool_option(PLAJA_OPTION::computeGoalPaths, false);

    propInfo = PropertyInformation::analyse_property(*configExplorer, *teacherModel->get_property(0), *teacherModel);
    factory->supports_property(*propInfo);
    factory->adapt_configuration(*configExplorer, propInfo.get());

    explorer = PLAJA_UTILS::cast_unique<SpaceExplorer>(factory->construct_engine(*configExplorer));

    const auto* current_model_tmp = teacherModel.get();
    std::swap(current_model_tmp, PLAJA_GLOBAL::currentModel); // Update global model.
    explorer->search();
    explorer->compute_goal_path_states();
    std::swap(current_model_tmp, PLAJA_GLOBAL::currentModel);
}

ExplorerTeacher::~ExplorerTeacher() = default;

ActionLabel_type ExplorerTeacher::get_action(const StateBase& state_parent) const {
    const auto teacher_model_state = translate_state_parent(state_parent);

    const auto* current_model_tmp = teacherModel.get();
    std::swap(current_model_tmp, PLAJA_GLOBAL::currentModel); // Update global model.

    const auto update_op = explorer->get_goal_op(explorer->search_goal_path(teacher_model_state));

    std::swap(current_model_tmp, PLAJA_GLOBAL::currentModel);

    if (update_op == ACTION::noneUpdateOp) { return ACTION::noneLabel; }
    return explorer->get_label(explorer->get_update_op(update_op).actionOpID);
}