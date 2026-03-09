//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#ifndef PLAJA_SUCCESSOR_GENERATOR_VERITAS_H
#define PLAJA_SUCCESSOR_GENERATOR_VERITAS_H

#include <memory>
#include <vector>
#include "../../../utils/default_constructors.h"
#include "../../successor_generation/forward_successor_generation.h"
#include "../../using_search.h"
#include "../forward_smt_veritas.h"
#include "../../../parser/ast/model.h"
#include "action_op_veritas.h"

namespace VERITAS_IN_PLAJA {

    class SuccessorGenerator {

    private:
        const ModelVeritas* model;
        const SuccessorGeneratorC* successorGenerator;
        std::vector<std::unique_ptr<ActionOpVeritas>> nonSyncOps;
        std::vector<std::vector<std::unique_ptr<ActionOpVeritas>>> syncOps;
        // per label
        std::vector<const ActionOpVeritas*> silentOps;
        std::vector<std::vector<const ActionOpVeritas*>> labeledOps;

    protected:
        void initialize_operators();
        [[nodiscard]] virtual std::unique_ptr<ActionOpVeritas> initialize_op(const ActionOp& action_op) const;
        void cache_per_label();

    public:
        SuccessorGenerator(const SuccessorGeneratorC& successor_generator, const ModelVeritas& model, bool init = true);
        virtual ~SuccessorGenerator();
        DELETE_CONSTRUCTOR(SuccessorGenerator)

        [[nodiscard]] inline const ModelVeritas& get_model() const { return *model; }

        void generate_steps();
        void increment();

        [[nodiscard]] const ActionOpVeritas& get_action_op(ActionOpID_type op_id) const;
        [[nodiscard]] const std::vector<const ActionOpVeritas*>& get_action_structure(ActionLabel_type label) const;

        [[nodiscard]] ActionLabel_type get_action_label(ActionOpID_type action_op_id) const;

        void add_guard(VERITAS_IN_PLAJA::QueryConstructable& query, ActionOpID_type op_id, bool do_locs, StepIndex_type step = 0) const;
        void add_update(VERITAS_IN_PLAJA::QueryConstructable& query, ActionOpID_type op_id, UpdateIndex_type update, bool do_locs, bool do_copy, StepIndex_type step = 0) const;
        void add_action_op(VERITAS_IN_PLAJA::QueryConstructable& query, ActionOpID_type op_id, UpdateIndex_type update, bool do_locs, bool do_copy, StepIndex_type step = 0) const;
        
        veritas::Tree get_label_filter_tree(ActionLabel_type action_label);
        std::vector<veritas::AddTree> get_applicability_filter();
        veritas::AddTree get_indicator_trees();

        // veritas::Tree get_naive_filter_tree(ActionLabel_type action_label);
        
        // void add_guard_disjunction(VERITAS_IN_PLAJA::QueryConstructable& query, ActionLabel_type label, StepIndex_type step) const;

        // [[nodiscard]] std::unique_ptr<VeritasConstraint> guards_to_veritas(ActionLabel_type label, StepIndex_type step) const;
        // [[nodiscard]] std::unique_ptr<DisjunctionInVeritas> action_to_veritas(ActionLabel_type label, StepIndex_type step) const;
        // [[nodiscard]] std::unique_ptr<DisjunctionInVeritas> expansion_to_veritas(StepIndex_type step) const;

    };

}

#endif //PLAJA_SUCCESSOR_GENERATOR_VERITAS_H
