//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SUCCESSOR_GENERATOR_PA_MARABOU_H
#define PLAJA_SUCCESSOR_GENERATOR_PA_MARABOU_H

#include "../../../smt_nn/successor_generation/successor_generator_marabou.h"

class ActionOpMarabouPA;
class ModelMarabouPA;

namespace MARABOU_IN_PLAJA {

class SuccessorGeneratorPA final: public MARABOU_IN_PLAJA::SuccessorGenerator {

private:
    [[nodiscard]] const ModelMarabouPA& get_model_pa() const;
    [[nodiscard]] std::unique_ptr<ActionOpMarabou> initialize_op(const ActionOp& action_op) const override;

public:
    SuccessorGeneratorPA(const SuccessorGeneratorC& successor_generator, const ModelMarabouPA& model);
    ~SuccessorGeneratorPA() override;
    DELETE_CONSTRUCTOR(SuccessorGeneratorPA)

    [[nodiscard]] const ActionOpMarabouPA& get_action_op_pa(ActionOpID_type op_id) const;

};

}


#endif //PLAJA_SUCCESSOR_GENERATOR_PA_MARABOU_H
