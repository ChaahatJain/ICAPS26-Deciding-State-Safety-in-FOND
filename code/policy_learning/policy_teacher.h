//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_POLICY_TEACHER_H
#define PLAJA_POLICY_TEACHER_H

#include <list>
#include <memory>
#include "../search/factories/forward_factories.h"
#include "../search/information/forward_information.h"
#include "../search/states/forward_states.h"
#include "../search/using_search.h"
#include "../utils/default_constructors.h"
#include "forward_policy_learning.h"

class Model;

class PolicyTeacher {

protected:
    const PropertyInformation* parentPropInfo;
    const Model* parentModel;
    /**/
    std::unique_ptr<Model> teacherModel;

private:
    std::list<std::pair<VariableIndex_type, VariableIndex_type>> stateIndexInterfaceInt;
    std::list<std::pair<VariableIndex_type, VariableIndex_type>> stateIndexInterfaceFloat;

    [[nodiscard]] static std::unique_ptr<Model> parse_teacher_model(const PLAJA::Configuration& config);
    void compute_variable_interface();

protected:
    [[nodiscard]] StateValues translate_state_parent(const StateBase& state_parent) const;

    explicit PolicyTeacher(const PLAJA::Configuration& config);
public:
    virtual ~PolicyTeacher() = 0;
    DELETE_CONSTRUCTOR(PolicyTeacher)

    /**
     * Teacher proposes an action for the provided state.
     * May return ACTION::noneLabel.
     */
    [[nodiscard]] virtual ActionLabel_type get_action(const StateBase& state) const = 0;

    [[nodiscard]] static std::unique_ptr<PolicyTeacher> construct_policy_teacher(const PLAJA::Configuration& config);

};

namespace POLICY_TEACHER {

    extern std::unique_ptr<PolicyTeacher> construct_explorer_teacher(const PLAJA::Configuration& config);
    extern std::unique_ptr<PolicyTeacher> construct_policy_model(const PLAJA::Configuration& config);

}

#endif //PLAJA_POLICY_TEACHER_H
