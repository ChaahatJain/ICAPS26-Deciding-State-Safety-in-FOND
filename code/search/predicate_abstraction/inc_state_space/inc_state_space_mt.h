//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_INC_STATE_SPACE_MT_H
#define PLAJA_INC_STATE_SPACE_MT_H

#include "../../../stats/forward_stats.h"
#include "../match_tree/pa_match_tree.h"
#include "incremental_state_space.h"
#include "source_node.h"

namespace SEARCH_SPACE_PA {

    struct MtNodeInformation final: public PaMtNodeInformation {

    private:
        SourceNodeP<const PAMatchTree::Node*> sourceNode;

    public:
        MtNodeInformation() = default;
        ~MtNodeInformation() override = default;
        DELETE_CONSTRUCTOR(MtNodeInformation)

        [[nodiscard]] const SourceNodeP<const PAMatchTree::Node*>& get_source_node() const { return sourceNode; }

        [[nodiscard]] SourceNodeP<const PAMatchTree::Node*>& get_source_node() { return sourceNode; }

    };

    class IncStateSpaceMt final: public IncStateSpace {
        friend IncStateSpace;

    private:
        std::shared_ptr<PAMatchTree> matchTree;
        PLAJA::StatsBase* stats;

        void set_excluded_internal(const AbstractState& src, UpdateOpID_type update_op_id, const AbstractState& excluded_successor) override;

        [[nodiscard]] bool is_excluded_label_internal(const AbstractState& source, ActionLabel_type action_label) const override;

        [[nodiscard]] bool is_excluded_op_internal(const AbstractState& source, ActionOpID_type action_op) const override;

        [[nodiscard]] PredicatesNumber_type load_entailments_internal(PredicateState& target, const AbstractState& source, UpdateOpID_type update_op, const PredicateRelations* predicate_relations) const override;

        [[nodiscard]] std::unique_ptr<StateID_type> transition_exists_internal(const AbstractState& source, UpdateOpID_type update_op_id, const AbstractState& successor) const override;

        /* */

        [[nodiscard]] SourceNode* get_source_node(const AbstractState& src) override;

        [[nodiscard]] UpdateOpNode* get_update_op(const AbstractState& src, UpdateOpID_type update_op_id) override;

        [[nodiscard]] SourceNode& add_source_node(const AbstractState& src) override;

        [[nodiscard]] UpdateOpNode& add_update_op(const AbstractState& src, UpdateOpID_type update_op_id) override;

    public:
        explicit IncStateSpaceMt(const PLAJA::Configuration& config, std::shared_ptr<StateRegistryPa> state_reg_pa);
        ~IncStateSpaceMt() final;
        DELETE_CONSTRUCTOR(IncStateSpaceMt)

        void update_stats() const override;

        void increment() override;

        /* */

        void refine_for(const AbstractState& source) override;

    };

}

#endif //PLAJA_INC_STATE_SPACE_MT_H
