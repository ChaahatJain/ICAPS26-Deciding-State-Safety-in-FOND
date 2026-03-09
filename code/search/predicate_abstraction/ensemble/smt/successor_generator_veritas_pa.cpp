//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#include "successor_generator_veritas_pa.h"
#include "../../../../utils/utils.h"
#include "action_op_veritas_pa.h"
#include "model_veritas_pa.h"

VERITAS_IN_PLAJA::SuccessorGeneratorPA::SuccessorGeneratorPA(const SuccessorGeneratorC& successor_generator, const ModelVeritasPA& model):
    SuccessorGenerator(successor_generator, model, false) {

    initialize_operators();
}

VERITAS_IN_PLAJA::SuccessorGeneratorPA::~SuccessorGeneratorPA() = default;

//

const ModelVeritasPA& VERITAS_IN_PLAJA::SuccessorGeneratorPA::get_model_pa() const { return PLAJA_UTILS::cast_ref<ModelVeritasPA>(get_model()); }

std::unique_ptr<ActionOpVeritas> VERITAS_IN_PLAJA::SuccessorGeneratorPA::initialize_op(const ActionOp& action_op) const { return std::make_unique<ActionOpVeritasPA>(action_op, get_model_pa()); }

//

const ActionOpVeritasPA& VERITAS_IN_PLAJA::SuccessorGeneratorPA::get_action_op_pa(ActionOpID_type op_id) const { return PLAJA_UTILS::cast_ref<ActionOpVeritasPA>(get_action_op(op_id)); }