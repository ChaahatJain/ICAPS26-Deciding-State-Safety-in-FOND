//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#ifndef PLAJA_SUCCESSOR_GENERATOR_PA_VERITAS_H
#define PLAJA_SUCCESSOR_GENERATOR_PA_VERITAS_H

#include "../../../smt_ensemble/successor_generation/successor_generator_veritas.h"

class ActionOpVeritasPA;
class ModelVeritasPA;

namespace VERITAS_IN_PLAJA {

class SuccessorGeneratorPA final: public VERITAS_IN_PLAJA::SuccessorGenerator {

private:
    [[nodiscard]] const ModelVeritasPA& get_model_pa() const;
    [[nodiscard]] std::unique_ptr<ActionOpVeritas> initialize_op(const ActionOp& action_op) const override;

public:
    SuccessorGeneratorPA(const SuccessorGeneratorC& successor_generator, const ModelVeritasPA& model);
    ~SuccessorGeneratorPA() override;
    DELETE_CONSTRUCTOR(SuccessorGeneratorPA)

    [[nodiscard]] const ActionOpVeritasPA& get_action_op_pa(ActionOpID_type op_id) const;

};

}


#endif //PLAJA_SUCCESSOR_GENERATOR_PA_VERITAS_H
