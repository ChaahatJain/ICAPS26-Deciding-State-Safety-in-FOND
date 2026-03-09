//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_INCREMENTAL_STATE_SPACE_H
#define PLAJA_INCREMENTAL_STATE_SPACE_H

#include <memory>
#include <vector>
#include <unordered_map>
#include "../../../include/ct_config_const.h"
#include "../../../assertions.h"
#include "../../factories/forward_factories.h"
#include "../optimization/forward_optimization_pa.h"
#include "../pa_states/forward_pa_states.h"
#include "../using_predicate_abstraction.h"
#include "forward_inc_state_space_pa.h"

namespace SEARCH_SPACE_PA {

    class IncStateSpace {

    protected:
        std::shared_ptr <StateRegistryPa> stateRegistryPa;

    private:
        StateID_type cachedSrcId;
        UpdateOpID_type cachedUpdateOpId;
        SourceNode* cachedSrc;
        UpdateOpNode* cachedUpdateOp;
        FIELD_IF_DEBUG(PredicatesNumber_type predicatesSize;)

        bool useIncSearch;
        FIELD_IF_DEBUG(bool cacheWitnesses;)

        FIELD_IF_DEBUG(std::unique_ptr <IncStateSpace> incStateSpaceRef;)

    protected:
        IncStateSpace(const PLAJA::Configuration& config, std::shared_ptr <StateRegistryPa> state_reg_pa);

        /* */

        [[nodiscard]] SourceNode* get_and_cache_source_node(const AbstractState& src);

        [[nodiscard]] UpdateOpNode* get_and_cache_update_op(const AbstractState& src, UpdateOpID_type update_op_id);

        [[nodiscard]] virtual SourceNode* get_source_node(const AbstractState& src) = 0;

        [[nodiscard]] virtual UpdateOpNode* get_update_op(const AbstractState& src, UpdateOpID_type update_op_id) = 0;

        [[nodiscard]] SourceNode& add_and_cache_source_node(const AbstractState& src);

        [[nodiscard]] UpdateOpNode& add_and_cache_update_op(const AbstractState& src, UpdateOpID_type update_op_id);

        [[nodiscard]] virtual SourceNode& add_source_node(const AbstractState& src) = 0;

        [[nodiscard]] virtual UpdateOpNode& add_update_op(const AbstractState& src, UpdateOpID_type update_op_id) = 0;

        /* */

        virtual void set_excluded_internal(const AbstractState& src, UpdateOpID_type update_op_id, const AbstractState& excluded_successor) = 0;

        [[nodiscard]] virtual bool is_excluded_label_internal(const AbstractState& source, ActionLabel_type action_label) const = 0;

        [[nodiscard]] virtual bool is_excluded_op_internal(const AbstractState& source, ActionOpID_type action_op) const = 0;

        [[nodiscard]] virtual PredicatesNumber_type load_entailments_internal(PredicateState& target, const AbstractState& source, UpdateOpID_type update_op_id, const PredicateRelations* predicate_relations) const = 0;

        [[nodiscard]] virtual std::unique_ptr <StateID_type> transition_exists_internal(const AbstractState& source, UpdateOpID_type update_op_id, const AbstractState& successor) const = 0;

        /* */

        FCT_IF_DEBUG([[nodiscard]] inline bool has_ref() const { return incStateSpaceRef.get(); })

        FCT_IF_DEBUG([[nodiscard]] inline IncStateSpace& access_ref() { return *incStateSpaceRef; })

    public:
        virtual ~IncStateSpace() = 0;
        DELETE_CONSTRUCTOR(IncStateSpace)

        /**************************************************************************************************************/

        virtual void update_stats() const;

        virtual void increment();

        virtual void clear_coarser();

        /**************************************************************************************************************/

        void set_excluded_label(const AbstractState& src, ActionLabel_type action_label);

        void set_excluded_op(const AbstractState& src, ActionOpID_type action_op);

        void store_entailments(const AbstractState& src, UpdateOpID_type update_op_id, const std::unordered_map<PredicateIndex_type, bool>& entailments, PredicatesNumber_type preds_size);

        void add_transition_witness(const AbstractState& src, UpdateOpID_type update_op_id, StateID_type src_witness, StateID_type suc_witness);

        void set_excluded(const AbstractState& src, UpdateOpID_type update_op_id, const AbstractState& excluded_successor);

        virtual void refine_for(const AbstractState& source) = 0;

        [[nodiscard]] bool is_excluded_label(const AbstractState& source, ActionLabel_type action_label) const;

        [[nodiscard]] bool is_excluded_op(const AbstractState& source, ActionOpID_type action_op) const;

        [[nodiscard]] PredicatesNumber_type load_entailments(PredicateState& target, const AbstractState& source, UpdateOpID_type update_op_id, const PredicateRelations* predicate_relations) const;

        [[nodiscard]] std::unique_ptr <StateID_type> transition_exists(const AbstractState& source, UpdateOpID_type update_op_id, const AbstractState& successor) const;

        /** Workaround to allow witness extraction while not using incremental computation. */
        [[nodiscard]] std::unique_ptr <StateID_type> retrieve_witness(const AbstractState& source, UpdateOpID_type update_op_id, const AbstractState& successor) const;

    };

}

#endif //PLAJA_INCREMENTAL_STATE_SPACE_H
