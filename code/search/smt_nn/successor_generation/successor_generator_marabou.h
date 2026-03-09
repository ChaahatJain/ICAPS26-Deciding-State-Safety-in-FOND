//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SUCCESSOR_GENERATOR_MARABOU_H
#define PLAJA_SUCCESSOR_GENERATOR_MARABOU_H

#include <memory>
#include <vector>
#include "../../../utils/default_constructors.h"
#include "../../successor_generation/forward_successor_generation.h"
#include "../../using_search.h"
#include "../forward_smt_nn.h"

namespace MARABOU_IN_PLAJA {

    class SuccessorGenerator {

    private:
        const ModelMarabou* model;
        const SuccessorGeneratorC* successorGenerator;
        std::vector<std::unique_ptr<ActionOpMarabou>> nonSyncOps;
        std::vector<std::vector<std::unique_ptr<ActionOpMarabou>>> syncOps;
        // per label
        std::vector<const ActionOpMarabou*> silentOps;
        std::vector<std::vector<const ActionOpMarabou*>> labeledOps;

    protected:
        void initialize_operators();
        [[nodiscard]] virtual std::unique_ptr<ActionOpMarabou> initialize_op(const ActionOp& action_op) const;
        void cache_per_label();

    public:
        SuccessorGenerator(const SuccessorGeneratorC& successor_generator, const ModelMarabou& model, bool init = true);
        virtual ~SuccessorGenerator();
        DELETE_CONSTRUCTOR(SuccessorGenerator)

        [[nodiscard]] inline const ModelMarabou& get_model() const { return *model; }

        void generate_steps();
        void increment();

        [[nodiscard]] const ActionOpMarabou& get_action_op(ActionOpID_type op_id) const;
        [[nodiscard]] const std::vector<const ActionOpMarabou*>& get_action_structure(ActionLabel_type label) const;

        [[nodiscard]] ActionLabel_type get_action_label(ActionOpID_type action_op_id) const;

        void add_guard(MARABOU_IN_PLAJA::QueryConstructable& query, ActionOpID_type op_id, bool do_locs, StepIndex_type step = 0) const;
        void add_update(MARABOU_IN_PLAJA::QueryConstructable& query, ActionOpID_type op_id, UpdateIndex_type update, bool do_locs, bool do_copy, StepIndex_type step = 0) const;
        void add_action_op(MARABOU_IN_PLAJA::QueryConstructable& query, ActionOpID_type op_id, UpdateIndex_type update, bool do_locs, bool do_copy, StepIndex_type step = 0) const;
        void add_guard_disjunction(MARABOU_IN_PLAJA::QueryConstructable& query, ActionLabel_type label, StepIndex_type step) const;

        [[nodiscard]] std::unique_ptr<MarabouConstraint> guards_to_marabou(ActionLabel_type label, StepIndex_type step) const;
        [[nodiscard]] std::unique_ptr<MarabouConstraint> action_to_marabou(ActionLabel_type label, StepIndex_type step) const;
        [[nodiscard]] std::unique_ptr<MarabouConstraint> expansion_to_marabou(StepIndex_type step) const;

    };

}

#endif //PLAJA_SUCCESSOR_GENERATOR_MARABOU_H
