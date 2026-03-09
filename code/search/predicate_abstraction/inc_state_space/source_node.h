//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SOURCE_NODE_H
#define PLAJA_SOURCE_NODE_H

#include <list>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "../../../utils/default_constructors.h"
#include "../../../utils/utils.h"
#include "../../using_search.h"
#include "../optimization/forward_optimization_pa.h"
#include "../pa_states/forward_pa_states.h"
#include "../using_predicate_abstraction.h"

namespace SEARCH_SPACE_PA {

    struct TransitionWitness final {
    private:
        StateID_type source;
        StateID_type successor;
    public:
        TransitionWitness(StateID_type src, StateID_type suc): // NOLINT(*-easily-swappable-parameters)
            source(src)
            , successor(suc) {
        }

        ~TransitionWitness() = default;
        DEFAULT_CONSTRUCTOR_ONLY(TransitionWitness)
        DELETE_ASSIGNMENT(TransitionWitness)

        [[nodiscard]] inline StateID_type get_source() const { return source; }

        [[nodiscard]] inline StateID_type get_successor() const { return successor; }
    };

    /******************************************************************************************************************/

    struct UpdateOpNode {

        using EntailmentInformation = int32_t;

    private:
        /* Entailments. */
        std::unique_ptr<EntailmentInformation[]> entailments; // Truth values entailed for source state and update (should not contain static entailments and binary relations).
        int entailmentsSize; // Negative size indicates that previous checked derived no entailments.
        PredicatesNumber_type lastPredsSize;

        /* Transitions. */
        std::list<TransitionWitness> transitionWitnesses;

        /* Convert entailment information. */
        [[nodiscard]] static EntailmentInformation to_info(PredicateIndex_type pred, bool val);
        [[nodiscard]] static std::pair<PredicateIndex_type, bool> to_info(EntailmentInformation info);
        FCT_IF_DEBUG([[nodiscard]] static bool check_sanity(PredicateIndex_type pred, bool val);)

    public:
        UpdateOpNode();
        virtual ~UpdateOpNode() = 0;
        UpdateOpNode(const UpdateOpNode& other);
        UpdateOpNode(UpdateOpNode&& other) noexcept;
        DELETE_ASSIGNMENT(UpdateOpNode)

        /* Setter. */

        void store_entailments(const std::unordered_map<PredicateIndex_type, bool>& entailments_map, PredicatesNumber_type preds_size);

        inline void add_transition_witness(StateID_type src_witness, StateID_type suc_witness) { transitionWitnesses.emplace_back(src_witness, suc_witness); }

        /* Getter. */

        [[nodiscard]] inline bool has_entailment() const { return entailmentsSize != 0; }

        /** The size of predicate set considered during the computation of the stored entailment information. */
        [[nodiscard]] inline PredicatesNumber_type retrieve_last_predicates_size() const { return lastPredsSize; }

        [[nodiscard]] inline bool has_non_empty_entailment() const { return entailmentsSize > 0; }

        void load_entailments(PredicateState& predicate_state, const PredicateRelations* predicate_relations) const;

        [[nodiscard]] inline const std::list<TransitionWitness>& get_transition_witnesses() const { return transitionWitnesses; }

        [[nodiscard]] inline std::list<TransitionWitness>& get_transition_witnesses() { return transitionWitnesses; }

    };

    struct SourceNode {
    private:
        std::unordered_set<ActionLabel_type> excludedLabels;
        std::unordered_set<ActionOpID_type> excludedOps;

    public:
        SourceNode();
        virtual ~SourceNode() = 0;
        DEFAULT_CONSTRUCTOR_ONLY(SourceNode)
        DELETE_ASSIGNMENT(SourceNode)

        /* Setter. */

        inline void set_excluded_label(ActionLabel_type action_label) { excludedLabels.insert(action_label); }

        inline void set_excluded_op(ActionOpID_type action_op) { excludedOps.insert(action_op); }

        /* Getter. */

        [[nodiscard]] inline bool is_excluded_label(ActionLabel_type action_label) const { return excludedLabels.count(action_label); }

        [[nodiscard]] inline bool is_excluded_op(ActionOpID_type action_op) const { return excludedOps.count(action_op); }

        [[nodiscard]] virtual UpdateOpNode* get_update_op(UpdateOpID_type update_op_id) = 0;

        [[nodiscard]] virtual const UpdateOpNode* get_update_op(UpdateOpID_type update_op_id) const = 0;

        [[nodiscard]] virtual UpdateOpNode& add_update_op(UpdateOpID_type update_op_id) = 0;

        inline void add_transition_witness(UpdateOpID_type update_op_id, StateID_type src_witness, StateID_type suc_witness) { add_update_op(update_op_id).add_transition_witness(src_witness, suc_witness); }

    };

    /******************************************************************************************************************/


    template<typename ExclusionInfo_t>
    struct UpdateOpNodeP final: public UpdateOpNode {
    private:
        std::unordered_set<ExclusionInfo_t> excludedSuccessors;

    public:
        UpdateOpNodeP() = default;
        ~UpdateOpNodeP() override = default;
        DEFAULT_CONSTRUCTOR_ONLY(UpdateOpNodeP)
        DELETE_ASSIGNMENT(UpdateOpNodeP)

        inline void set_excluded(ExclusionInfo_t excluded_successor) { excludedSuccessors.insert(excluded_successor); }

        [[nodiscard]] inline bool is_excluded(ExclusionInfo_t successor) const { return excludedSuccessors.count(successor); }

        [[nodiscard]] inline const std::unordered_set<ExclusionInfo_t>& get_excluded_successors() const { return excludedSuccessors; }

    };

    template<typename ExclusionInfo_t>
    struct SourceNodeP final: public SourceNode {
    private:
        std::unordered_map<UpdateOpID_type, UpdateOpNodeP<ExclusionInfo_t>> transitionsPerOp;

    public:
        SourceNodeP() = default;
        ~SourceNodeP() override = default;
        DEFAULT_CONSTRUCTOR_ONLY(SourceNodeP)
        DELETE_ASSIGNMENT(SourceNodeP)

        [[nodiscard]] UpdateOpNode* get_update_op(UpdateOpID_type update_op_id) override {
            auto rlt = transitionsPerOp.find(update_op_id);
            return rlt == transitionsPerOp.end() ? nullptr : &rlt->second;
        }

        [[nodiscard]] const UpdateOpNode* get_update_op(UpdateOpID_type update_op_id) const override {
            auto rlt = transitionsPerOp.find(update_op_id);
            return rlt == transitionsPerOp.end() ? nullptr : &rlt->second;
        }

        [[nodiscard]] UpdateOpNode& add_update_op(UpdateOpID_type update_op_id) override { return transitionsPerOp.emplace(update_op_id, UpdateOpNodeP<ExclusionInfo_t>()).first->second; }

        [[nodiscard]] inline std::unordered_map<UpdateOpID_type, UpdateOpNodeP<ExclusionInfo_t>>& get_transitions_per_op() { return transitionsPerOp; }

        inline void set_excluded(UpdateOpID_type update_op_id, ExclusionInfo_t excluded_successor) { PLAJA_UTILS::cast_ref<UpdateOpNodeP<ExclusionInfo_t>>(add_update_op(update_op_id)).set_excluded(excluded_successor); }
    };

}

#endif //PLAJA_SOURCE_NODE_H
