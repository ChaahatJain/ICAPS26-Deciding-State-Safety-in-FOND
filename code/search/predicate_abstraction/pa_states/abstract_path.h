//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_ABSTRACT_PATH_H
#define PLAJA_ABSTRACT_PATH_H

#include <list>
#include <memory>
#include <vector>
#include "../../../parser/ast/expression/forward_expression.h"
#include "../../../utils/structs_utils/update_op_structure.h"
#include "../../../assertions.h"
#include "../../states/forward_states.h"
#include "../../successor_generation/forward_successor_generation.h"
#include "../search_space/forward_search_space.h"
#include "forward_pa_states.h"
#include "../../../include/ct_config_const.h"
#ifdef USE_VERITAS
#include "interval.hpp"
#endif

struct AbstractPath {
private:
    const PredicatesExpression* predicates;
    const SearchSpacePABase* searchSpacePa;

    std::vector<UpdateOpStructure> opPath;
    std::vector<StateID_type> witnessPath;
    std::vector<std::unique_ptr<AbstractState>> paStatePath;

public:
    explicit AbstractPath(const PredicatesExpression& predicates, const SearchSpacePABase& ss_pa);
    ~AbstractPath();
    DEFAULT_CONSTRUCTOR(AbstractPath)

    void set_op_path(const std::list<UpdateOpStructure>& op_path);

    void set_witness_path(const std::list<StateID_type>& witness_path);

    void set_pa_path(const std::list<StateID_type>& pa_state_path);

    void append(AbstractPath&& path);

    inline void append_op(ActionOpID_type op_id, UpdateIndex_type update_index) { opPath.emplace_back(op_id, update_index); }

    inline void append_witness(StateID_type id) {
        PLAJA_ASSERT(searchSpacePa)
        witnessPath.emplace_back(id);
    }

    void append_abstract_state(StateID_type id);

    /******************************************************************************************************************/

    [[nodiscard]] const PredicatesExpression& get_predicates() const { return *predicates; }

    [[nodiscard]] const ActionOp& retrieve_op(ActionOpID_type op_id) const;

    [[nodiscard]] ActionLabel_type retrieve_label(ActionOpID_type op_id) const;

    [[nodiscard]] inline bool has_witnesses() const {
        PLAJA_ASSERT(witnessPath.empty() or (searchSpacePa and witnessPath.size() == opPath.size()))
        return not witnessPath.empty();
    }

    [[nodiscard]] inline bool has_pa_state_path() const {
        PLAJA_ASSERT(paStatePath.empty() or (searchSpacePa and paStatePath.size() == opPath.size() + 1))
        return not paStatePath.empty();
    }

    [[nodiscard]] inline std::size_t get_size() const { return opPath.size(); }

    [[nodiscard]] inline std::size_t get_state_path_size() const { return get_size() + 1; }

    [[nodiscard]] inline StepIndex_type get_start_step() const { return 0; }

    [[nodiscard]] inline StepIndex_type get_target_step() const { return get_size(); }

    [[nodiscard]] inline ActionOpID_type get_op_id(StepIndex_type step) const {
        PLAJA_ASSERT(step < opPath.size())
        return opPath[step].actionOpID;
    }

    [[nodiscard]] inline UpdateIndex_type get_update_index(StepIndex_type step) const {
        PLAJA_ASSERT(step < opPath.size())
        return opPath[step].updateIndex;
    }

    [[nodiscard]] inline const ActionOp& retrieve_op(StepIndex_type step) const { return retrieve_op(get_op_id(step)); }

    [[nodiscard]] inline ActionOpID_type last_op_id() const { return opPath.back().actionOpID; }

    [[nodiscard]] inline ActionOpID_type last_update_index() const { return opPath.back().updateIndex; }

    [[nodiscard]] inline const ActionOp& retrieve_last_op() const { return retrieve_op(last_op_id()); }

    [[nodiscard]] inline ActionLabel_type retrieve_last_label() const { return retrieve_label(last_op_id()); }

    [[nodiscard]] bool is_non_deterministic() const;

    [[nodiscard]] StateID_type get_witness_id(StepIndex_type step) const;

    [[nodiscard]] inline bool has_witness(StepIndex_type step) const { return STATE::none != get_witness_id(step); }

    [[nodiscard]] std::unique_ptr<State> get_witness(StepIndex_type step) const;

    #ifdef USE_VERITAS
    [[nodiscard]] std::vector<veritas::Interval> get_box_witness(StepIndex_type step) const;
    #endif

    [[nodiscard]] inline bool has_last_witness() const {
        if (has_witnesses()) {
            PLAJA_ASSERT(witnessPath.size() == get_size())
            return witnessPath.back() != STATE::none;
        } else { return false; };
    }

    [[nodiscard]] inline StateID_type last_witness_id() const {
        PLAJA_ASSERT(has_last_witness())
        return witnessPath.back();
    }

    [[nodiscard]] std::unique_ptr<State> last_witness() const;

    [[nodiscard]] StateID_type get_pa_state_id(StepIndex_type step) const;

    [[nodiscard]] const AbstractState& get_pa_state(StepIndex_type step) const;

    [[nodiscard]] inline StateID_type get_pa_source_id() const { return get_pa_state_id(get_start_step()); }

    [[nodiscard]] inline const AbstractState& get_pa_source_state() const { return get_pa_state(get_start_step()); }

    [[nodiscard]] inline StateID_type get_pa_target_id() const { return get_pa_state_id(get_target_step()); }

    [[nodiscard]] inline const AbstractState& get_pa_target_state() const { return get_pa_state(get_target_step()); }

    void dump() const;

/**********************************************************************************************************************/

    class PathIterator {
        friend AbstractPath;
    protected:
        const AbstractPath* path;
        StepIndex_type step;

        explicit PathIterator(const AbstractPath& path);
    public:
        virtual ~PathIterator() = default;
        DELETE_CONSTRUCTOR(PathIterator)

        [[nodiscard]] inline ActionOpID_type get_op_id() const { return path->get_op_id(step); }

        [[nodiscard]] inline const ActionOp& retrieve_op() const { return path->retrieve_op(get_op_id()); }

        [[nodiscard]] inline ActionLabel_type retrieve_label() const { return path->retrieve_label(get_op_id()); }

        [[nodiscard]] inline UpdateIndex_type get_update_index() const { return path->get_update_index(step); }

        [[nodiscard]] inline bool has_witness() const { return path->has_witness(step); }

        [[nodiscard]] inline StateID_type get_witness_id() const { return path->get_witness_id(step); }

        [[nodiscard]] std::unique_ptr<State> get_witness() const;

        #ifdef USE_VERITAS
        [[nodiscard]] std::vector<veritas::Interval> get_box_witness() const;
        #endif

        [[nodiscard]] inline StateID_type get_pa_state_id() const { return path->get_pa_state_id(step); }

        [[nodiscard]] inline const AbstractState& get_pa_state() const { return path->get_pa_state(step); }

    };

    class PathIteratorForward final: public PathIterator {
        friend AbstractPath;
    private:
        StepIndex_type targetStep;

        explicit PathIteratorForward(const AbstractPath& path, StepIndex_type target_step);
    public:
        ~PathIteratorForward() override = default;
        DELETE_CONSTRUCTOR(PathIteratorForward)

        inline void operator++() { ++step; }

        [[nodiscard]] inline bool end() const { return step >= targetStep; }

        [[nodiscard]] inline StepIndex_type get_step() const { return step; }

        [[nodiscard]] inline StepIndex_type get_successor_step() const { return step + 1; }

        [[nodiscard]] inline StateID_type get_pa_target_id() const { return path->get_pa_target_id(); }

        [[nodiscard]] inline const AbstractState& get_abstract_target_state() const { return path->get_pa_target_state(); }

    };

    [[nodiscard]] inline PathIteratorForward init_path_iterator() const { return PathIteratorForward(*this, get_target_step()); } // NOLINT(modernize-return-braced-init-list)

    [[nodiscard]] inline PathIteratorForward init_prefix_iterator(StepIndex_type max_step) const { return PathIteratorForward(*this, max_step); } // NOLINT(modernize-return-braced-init-list)


    class PathIteratorBackwards final: public PathIterator {
        friend AbstractPath;
    private:
        int step; // signed on purpose

        explicit PathIteratorBackwards(const AbstractPath& path, StepIndex_type target_step);
    public:
        ~PathIteratorBackwards() override = default;
        DELETE_CONSTRUCTOR(PathIteratorBackwards)

        inline void operator++() { --step; }

        [[nodiscard]] inline bool end() const { return step < 0; }

        [[nodiscard]] inline StepIndex_type get_step() const { return step; }

        [[nodiscard]] inline StepIndex_type get_successor_step() const { return step + 1; }

    };

    [[nodiscard]] inline PathIteratorForward init_path_iterator_backwards() const { return PathIteratorForward(*this, get_target_step()); } // NOLINT(modernize-return-braced-init-list)

    [[nodiscard]] inline PathIteratorBackwards init_prefix_iterator_backwards(StepIndex_type target_step) const { return PathIteratorBackwards(*this, target_step); } // NOLINT(modernize-return-braced-init-list)

};

#endif //PLAJA_ABSTRACT_PATH_H
