//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "policy_teacher.h"
#include "../exception/constructor_exception.h"
#include "../parser/ast/iterators/model_iterator.h"
#include "../parser/ast/type/array_type.h"
#include "../parser/ast/automaton.h"
#include "../parser/ast/model.h"
#include "../parser/ast/variable_declaration.h"
#include "../search/factories/policy_learning/ql_options.h"
#include "../search/factories/configuration.h"
#include "../search/information/model_information.h"
#include "../search/information/property_information.h"
#include "../search/states/state_values.h"

namespace PARSER { extern std::unique_ptr<Model> parse(const std::string* filename, const PLAJA::OptionParser* option_parser, bool check_constants); }
namespace SEMANTICS_CHECKER { extern void check_semantics(AstElement* ast_element); }

namespace POLICY_TEACHER {

    const std::string exceptionPrefix("Policy teacher construction: "); // NOLINT(*-err58-cpp)

}

/**********************************************************************************************************************/

PolicyTeacher::PolicyTeacher(const PLAJA::Configuration& config):
    parentPropInfo(config.get_sharable_const<PropertyInformation>(PLAJA::SharableKey::PROP_INFO))
    , parentModel(parentPropInfo->get_model())
    , teacherModel(parse_teacher_model(config)) {

    compute_variable_interface();

}

PolicyTeacher::~PolicyTeacher() = default;

/**********************************************************************************************************************/

std::unique_ptr<Model> PolicyTeacher::parse_teacher_model(const PLAJA::Configuration& config) {

    PLAJA_ASSERT(config.has_value_option(PLAJA_OPTION::teacherModel))

    auto teacher_model = PARSER::parse(&config.get_value_option_string(PLAJA_OPTION::teacherModel), config.share_option_parser(), false); // TODO, should actually share config.

    if (not teacher_model) {
        throw ConstructorException("Invalid policy model: " + config.get_value_option_string(PLAJA_OPTION::teacherModel));
    }

    if (teacher_model) {
        SEMANTICS_CHECKER::check_semantics(teacher_model.get());
        teacher_model->compute_model_information();
    }

    return teacher_model;

}

std::unique_ptr<PolicyTeacher> PolicyTeacher::construct_policy_teacher(const PLAJA::Configuration& config) {

    if (not config.has_value_option(PLAJA_OPTION::teacherModel)) { return nullptr; }

    if (config.get_bool_option(PLAJA_OPTION::teacherExplore)) {
        return POLICY_TEACHER::construct_explorer_teacher(config);
    } else {
        return POLICY_TEACHER::construct_policy_model(config);
    }

}

void PolicyTeacher::compute_variable_interface() {

    { /* Locations. */

        std::unordered_map<std::string, const Automaton*> locations_parent_model;

        /* Collect parent locations. */
        for (auto it = ModelIterator::automatonInstanceIterator(*parentModel); !it.end(); ++it) {

            const auto& name = it->get_name();

            if (locations_parent_model.count(name)) {
                throw ConstructorException(PLAJA_UTILS::string_f("%s duplicate automaton instance name (%s).", POLICY_TEACHER::exceptionPrefix.c_str(), name.c_str()));
            }

            locations_parent_model.emplace(name, it());

        }

        /* Set interface. */
        std::unordered_set<std::string> locations_teacher_model;
        for (auto it = ModelIterator::automatonInstanceIterator(*teacherModel); !it.end(); ++it) {

            const auto& name = it->get_name();

            if (not locations_teacher_model.insert(name).second) {
                throw ConstructorException(PLAJA_UTILS::string_f("%s duplicate automaton instance name (%s).", POLICY_TEACHER::exceptionPrefix.c_str(), name.c_str()));
            }

            const auto it_parent = locations_parent_model.find(name);

            if (it_parent == locations_parent_model.end()) { continue; }

            PLAJA_ASSERT(it->get_index() != AUTOMATON::invalid);
            PLAJA_ASSERT(it_parent->second->get_index() != AUTOMATON::invalid);

            stateIndexInterfaceInt.emplace_back(it->get_index(), it_parent->second->get_index());

        }

    }

    { /* Vars. */

        std::unordered_map<std::string, const VariableDeclaration*> vars_parent_model; // TODO might try per automaton.

        /* Collect parent vars. */
        for (auto it = parentModel->variableIterator(); !it.end(); ++it) {
            const auto* variable = it.variable();
            const auto& name = variable->get_name();

            if (vars_parent_model.count(name)) {
                throw ConstructorException(PLAJA_UTILS::string_f("%s duplicate variable name (%s).", POLICY_TEACHER::exceptionPrefix.c_str(), name.c_str()));
            }

            vars_parent_model.emplace(name, variable);
        }

        /* Set interface. */
        std::unordered_set<std::string> vars_teacher_model;
        for (auto it = teacherModel->variableIterator(); !it.end(); ++it) {
            const auto* variable = it.variable();
            const auto& name = variable->get_name();

            if (not vars_teacher_model.insert(name).second) {
                throw ConstructorException(PLAJA_UTILS::string_f("%s duplicate variable name (%s).", POLICY_TEACHER::exceptionPrefix.c_str(), name.c_str()));
            }

            const auto it_parent = vars_parent_model.find(name);

            if (it_parent == vars_parent_model.end()) { continue; }

            const auto* var_parent = it_parent->second;

            PLAJA_ASSERT(variable->get_id() != VARIABLE::none);
            PLAJA_ASSERT(var_parent->get_id() != VARIABLE::none);

            PLAJA_ASSERT(variable->get_type())
            PLAJA_ASSERT(var_parent->get_type())
            if (not variable->get_type()->is_assignable_from(*var_parent->get_type())) {
                throw ConstructorException(PLAJA_UTILS::string_f("%s teacher model type is not assignable from parent variable (%s).", POLICY_TEACHER::exceptionPrefix.c_str(), name.c_str()));
            }

            /* Get state index. */
            auto state_index = variable->get_index();
            PLAJA_ASSERT(state_index != STATE_INDEX::none)
            auto state_index_parent = var_parent->get_index();
            PLAJA_ASSERT(state_index_parent != STATE_INDEX::none)

            const auto var_size = variable->get_size();
            PLAJA_ASSERT(var_size > 0)

            bool is_float;

            /* Array. */
            if (variable->is_array() or var_parent->is_array()) {

                if (not(variable->is_array() and var_parent->is_array())
                    or var_size != var_parent->get_size()
                    ) {
                    throw ConstructorException(PLAJA_UTILS::string_f("%s array conflict for variable (%s).", POLICY_TEACHER::exceptionPrefix.c_str(), name.c_str()));
                }

                is_float = PLAJA_UTILS::cast_ptr<ArrayType>(variable->get_type())->is_floating_array();

            } else {
                is_float = variable->get_type()->is_floating_type();
            }

            for (std::size_t offset = 0; offset < var_size; ++offset) {

                if (is_float) {
                    stateIndexInterfaceFloat.emplace_back(state_index, state_index_parent);
                } else {
                    stateIndexInterfaceInt.emplace_back(state_index, state_index_parent);
                }

            }

        }

    }

    PLAJA_ASSERT(not stateIndexInterfaceInt.empty())

}

/**********************************************************************************************************************/

StateValues PolicyTeacher::translate_state_parent(const StateBase& state_parent) const {
    StateValues policy_model_state(teacherModel->get_model_information().get_initial_values());

    /* Integer. */
    for (const auto& var_mapping: stateIndexInterfaceInt) {
        policy_model_state.assign_int<false>(var_mapping.first, state_parent.get_int(var_mapping.second));
    }

    /* Float. */
    for (const auto& var_mapping: stateIndexInterfaceFloat) {
        policy_model_state.assign_float<false>(var_mapping.first, state_parent.get_float(var_mapping.second));
    }

    return policy_model_state;
}