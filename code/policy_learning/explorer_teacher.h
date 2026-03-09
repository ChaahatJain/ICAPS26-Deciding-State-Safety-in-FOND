//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_EXPLORER_TEACHER_H
#define PLAJA_EXPLORER_TEACHER_H

#include <memory>
#include "policy_teacher.h"

class SpaceExplorer;

class ExplorerTeacher final: public PolicyTeacher {

private:
    std::unique_ptr<PropertyInformation> propInfo;
    std::unique_ptr<PLAJA::Configuration> configExplorer;
    std::unique_ptr<SpaceExplorer> explorer;

public:
    explicit ExplorerTeacher(const PLAJA::Configuration& config);
    ~ExplorerTeacher() override;
    DELETE_CONSTRUCTOR(ExplorerTeacher)

    [[nodiscard]] ActionLabel_type get_action(const StateBase& state) const override;

};

#endif //PLAJA_EXPLORER_TEACHER_H
