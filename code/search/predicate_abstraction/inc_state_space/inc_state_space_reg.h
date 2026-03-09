//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_INC_STATE_SPACE_REG_H
#define PLAJA_INC_STATE_SPACE_REG_H

#include <map>
#include "../pa_states/state_registry_pa.h"
#include "incremental_state_space.h"
#include "source_node.h"

namespace SEARCH_SPACE_PA {

    struct SourceNodeRegistry final {
    private:
        segmented_vector::SegmentedVector<SourceNodeP<StateID_type>> sources;

    public:
        SourceNodeRegistry() = default;
        ~SourceNodeRegistry() = default;
        DELETE_CONSTRUCTOR(SourceNodeRegistry)

        [[nodiscard]] inline const SourceNodeP<StateID_type>* get_source(StateID_type src_id) const { if (src_id < sources.size()) { return PLAJA_UTILS::cast_ptr<SourceNodeP<StateID_type>>(&sources[src_id]); } else { return nullptr; } }

        [[nodiscard]] inline SourceNodeP<StateID_type>* get_source(StateID_type src_id) { if (src_id < sources.size()) { return PLAJA_UTILS::cast_ptr<SourceNodeP<StateID_type>>(&sources[src_id]); } else { return nullptr; } }

        [[nodiscard]] inline SourceNodeP<StateID_type>& add_source(StateID_type src_id) {
            if (src_id >= sources.size()) { sources.resize(src_id + 1); }
            return PLAJA_UTILS::cast_ref<SourceNodeP<StateID_type>>(sources[src_id]);
        }

        /* Label. */

        [[nodiscard]] inline bool is_excluded_label(StateID_type src_id, ActionLabel_type action_label) const {
            auto* src_node = get_source(src_id);
            return src_node ? src_node->is_excluded_label(action_label) : false;
        }

        /* Op. */

        [[nodiscard]] inline bool is_excluded_op(StateID_type src_id, ActionOpID_type action_op) const {
            auto* src_node = get_source(src_id);
            return src_node ? src_node->is_excluded_op(action_op) : false;
        }

        /* Update node / transition. */

        [[nodiscard]] inline const UpdateOpNodeP<StateID_type>* get_update_node(StateID_type src_id, UpdateOpID_type update_op_id) const {
            const auto* src_node = get_source(src_id);
            return src_node ? PLAJA_UTILS::cast_ptr<UpdateOpNodeP<StateID_type>>(src_node->get_update_op(update_op_id)) : nullptr;
        }

        [[nodiscard]] inline UpdateOpNodeP<StateID_type>* get_update_node(StateID_type src_id, UpdateOpID_type update_op_id) {
            auto* src_node = get_source(src_id);
            return src_node ? PLAJA_UTILS::cast_ptr<UpdateOpNodeP<StateID_type>>(src_node->get_update_op(update_op_id)) : nullptr;
        }

        inline UpdateOpNodeP<StateID_type>& add_update_node(StateID_type src_id, UpdateOpID_type update_op_id) { return PLAJA_UTILS::cast_ref<UpdateOpNodeP<StateID_type>>(add_source(src_id).add_update_op(update_op_id)); }

        inline void set_excluded(StateID_type src_id, UpdateOpID_type update_op_id, StateID_type excluded_successor) { add_source(src_id).set_excluded(update_op_id, excluded_successor); }

    };

/**********************************************************************************************************************/

    struct IncStateSpaceReg final: public IncStateSpace {
        friend IncStateSpace;

    private:
        std::map<PredicatesNumber_type, std::unique_ptr<SourceNodeRegistry>> nodeRegistries;

        [[nodiscard]] inline SourceNodeRegistry& get_node_reg(PredicatesNumber_type num_preds) {
            PLAJA_ASSERT(nodeRegistries.count(num_preds))
            return *nodeRegistries.at(num_preds);
        };

        [[nodiscard]] inline const SourceNodeRegistry& get_node_reg(PredicatesNumber_type num_preds) const {
            PLAJA_ASSERT(nodeRegistries.count(num_preds))
            return *nodeRegistries.at(num_preds);
        };

        [[nodiscard]] inline StateRegistry& get_state_reg(PredicatesNumber_type num_preds) { return stateRegistryPa->get_registry(num_preds); };

        [[nodiscard]] inline const StateRegistry& get_state_reg(PredicatesNumber_type num_preds) const { return stateRegistryPa->get_registry(num_preds); };

        void set_registry(PredicatesNumber_type num_preds);

        [[nodiscard]] inline std::size_t number_of_registries() const { return nodeRegistries.size(); };

        /* */

        [[nodiscard]] SourceNode* get_source_node(const AbstractState& src) override;

        [[nodiscard]] UpdateOpNode* get_update_op(const AbstractState& src, UpdateOpID_type update_op_id) override;

        [[nodiscard]] SourceNode& add_source_node(const AbstractState& src) override;

        [[nodiscard]] UpdateOpNode& add_update_op(const AbstractState& src, UpdateOpID_type update_op_id) override;

        /* */

        void set_excluded_internal(const AbstractState& src, UpdateOpID_type update_op_id, const AbstractState& excluded_successor) override;

        [[nodiscard]] bool is_excluded_label_internal(const AbstractState& source, ActionLabel_type action_label) const override;

        [[nodiscard]] bool is_excluded_op_internal(const AbstractState& source, ActionOpID_type action_op) const override;

        [[nodiscard]] PredicatesNumber_type load_entailments_internal(PredicateState& target, const AbstractState& source, UpdateOpID_type update_op, const PredicateRelations* predicate_relations) const override;

        [[nodiscard]] std::unique_ptr<StateID_type> transition_exists_internal(const AbstractState& source, UpdateOpID_type update_op_id, const AbstractState& successor) const override;

    public:
        explicit IncStateSpaceReg(const PLAJA::Configuration& config, std::shared_ptr<StateRegistryPa> state_reg_pa);
        ~IncStateSpaceReg() final;
        DELETE_CONSTRUCTOR(IncStateSpaceReg)

        void increment() override;

        void clear_coarser() override;

        void refine_for(const AbstractState& source) override;

        /**************************************************************************************************************/

        class RegistryIteratorBase {
        protected:
            IncStateSpaceReg& stateSpace;
            std::map<PredicatesNumber_type, std::unique_ptr<SourceNodeRegistry>>::const_iterator it;
            std::map<PredicatesNumber_type, std::unique_ptr<SourceNodeRegistry>>::const_iterator endIt;
            PredicatesNumber_type maxIndex;

            RegistryIteratorBase(IncStateSpaceReg& state_space, PredicatesNumber_type max_num_preds):
                stateSpace(state_space)
                , it(state_space.nodeRegistries.cbegin())
                , endIt(state_space.nodeRegistries.cend())
                , maxIndex(max_num_preds) {}

        public:
            virtual ~RegistryIteratorBase() = default;
            DELETE_CONSTRUCTOR(RegistryIteratorBase)

            inline void operator++() { ++it; }

            [[nodiscard]] inline bool end() const { return it == endIt or it->first > maxIndex; }

            [[nodiscard]] inline const SourceNodeRegistry& node_reg() const { return *it->second; }

            [[nodiscard]] inline PredicatesNumber_type number_of_preds() const { return it->first; }

            [[nodiscard]] inline const UpdateOpNodeP<StateID_type>* get_update_node(const AbstractState& source, UpdateOpID_type update_op) const {
                auto coarser_id = stateSpace.stateRegistryPa->is_contained_in(source, number_of_preds());
                if (coarser_id == STATE::none) { return nullptr; } // does not contain any information concerning source
                return node_reg().get_update_node(coarser_id, update_op);
            }

        };

        class RegistryIterator final: public RegistryIteratorBase {
            friend IncStateSpaceReg;
        private:
            RegistryIterator(IncStateSpaceReg& state_space, PredicatesNumber_type max_num_preds):
                RegistryIteratorBase(state_space, max_num_preds) {}

        public:
            ~RegistryIterator() final = default;
            DELETE_CONSTRUCTOR(RegistryIterator)

            [[nodiscard]] inline SourceNodeRegistry& node_reg() { return *it->second; }

            [[nodiscard]] inline UpdateOpNodeP<StateID_type>* get_update_node(const AbstractState& source, UpdateOpID_type update_op) {
                auto coarser_id = stateSpace.stateRegistryPa->is_contained_in(source, number_of_preds());
                if (coarser_id == STATE::none) { return nullptr; } // does not contain any information concerning source
                return node_reg().get_update_node(coarser_id, update_op);
            }
        };

        class RegistryConstIterator final: public RegistryIteratorBase {
            friend IncStateSpaceReg;
        private:
            RegistryConstIterator(const IncStateSpaceReg& state_space, PredicatesNumber_type max_num_preds):
                RegistryIteratorBase(const_cast<IncStateSpaceReg&>(state_space), max_num_preds) {}

        public:
            ~RegistryConstIterator() final = default;
            DELETE_CONSTRUCTOR(RegistryConstIterator)
        };

        [[nodiscard]] inline RegistryConstIterator registryIterator() const { return { *this, (--nodeRegistries.end())->first }; }

        [[nodiscard]] inline RegistryConstIterator registryIterator(PredicatesNumber_type max_num_preds) const { return { *this, max_num_preds }; }

        [[nodiscard]] inline RegistryIterator registryIterator() { return { *this, (--nodeRegistries.end())->first }; }

        [[nodiscard]] inline RegistryIterator registryIterator(PredicatesNumber_type max_num_preds) { return { *this, max_num_preds }; }

    };

}

#endif //PLAJA_INC_STATE_SPACE_REG_H
