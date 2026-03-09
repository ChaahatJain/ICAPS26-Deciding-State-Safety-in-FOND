//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PROB_SEARCH_SPACE_H
#define PLAJA_PROB_SEARCH_SPACE_H

#include "../../utils/fd_imports/segmented_vector.h"
#include "../fd_adaptions/state.h"
#include "optimization_criteria.h"
#include "prob_search_node_info.h"

// forward declaration:
class ModelInformation;

/**
 * Basic probabilistic search node. To be used with basic probabilistic search node info, namely ProbSearchNodeInfo.
 * @tparam PSearchNodeInfo_t
 */
template<typename PSearchNodeInfo_t>
class ProbSearchNode {
protected:
    StateID_type stateID;
    PSearchNodeInfo_t& info;
    unsigned currentStep;

public:
    ProbSearchNode(StateID_type state_ID, PSearchNodeInfo_t& info_, unsigned current_step): stateID(state_ID), info(info_), currentStep(current_step) {};
    virtual ~ProbSearchNode() = default;

    [[nodiscard]] inline StateID_type get_state_id() const { return stateID; }

    // REMARK: the following set methods are *not const* in accordance with intuition. However, technically modifications to the info-reference do not modify a node.

    inline void set_v_current(QValue_type v, LocalOpIndex_type local_op) {
        info.v_current = v;
        info.policy_choice = local_op;
    }
    [[nodiscard]] inline QValue_type get_v_current() const { return info.v_current; }
    inline void set_policy_choice_index(LocalOpIndex_type local_op) { info.policy_choice = local_op; }
    [[nodiscard]] inline LocalOpIndex_type get_policy_choice_index() const { return info.policy_choice; }
    [[nodiscard]] inline PLAJA::Cost_type get_cost() const { return info.cost; }
    inline void set_cost(PLAJA::Cost_type cost) { RUNTIME_ASSERT(cost >= 0, "negative cost") info.cost = cost; }

    [[nodiscard]] inline bool is_new() const { return info.status == ProbSearchNodeInfo::NEW; }
    [[nodiscard]] inline bool is_opened() const { return info.status == ProbSearchNodeInfo::OPEN; }
    [[nodiscard]] inline bool is_closed() const { return info.status == ProbSearchNodeInfo::CLOSED; }
    [[nodiscard]] inline bool is_dead_end() const { return info.status == ProbSearchNodeInfo::DEAD_END; }
    [[nodiscard]] inline bool is_goal() const { return info.status == ProbSearchNodeInfo::GOAL; }
    [[nodiscard]] inline bool is_dont_care() const { return info.status == ProbSearchNodeInfo::DONT_CARE; }
    [[nodiscard]] inline bool is_fixed() const { return info.status >= ProbSearchNodeInfo::FIXED; }

    inline void set_open() { info.status = ProbSearchNodeInfo::OPEN; }
    inline void close() { info.status = ProbSearchNodeInfo::CLOSED; }
    inline void mark_as_dead_end() { info.status = ProbSearchNodeInfo::DEAD_END; }
    inline void set_goal() { info.status = ProbSearchNodeInfo::GOAL; }
    inline void set_dont_care() { info.status = ProbSearchNodeInfo::DONT_CARE; }
    inline void mark_fixed() { info.status = ProbSearchNodeInfo::FIXED; }

    // Solved in current sub-search, e.g. for FRET we have several calls to LRTDP.
    [[nodiscard]] inline bool is_solved() const { return info.solved >= currentStep; }
    inline void set_solved() { info.solved = currentStep; }

    // used on the caller-side to indicate membership to a stack, queue, list ...
    inline void mark() { info.marked = true; }
    inline void unmark() { info.marked = false; }
    [[nodiscard]] inline bool is_marked() const { return info.marked; }

    // Debugging
#ifndef NDEBUG
    void dump_status() const {
        std::cout << is_new() << is_opened() << is_closed() <<  is_fixed() << is_dead_end() << is_goal() << is_dont_care() << std::endl;
    }
#endif
};

template<typename PSearchNodeInfo_t>
struct UpperAndLowerPNode: public ProbSearchNode<PSearchNodeInfo_t> {
    UpperAndLowerPNode(StateID_type state_id, PSearchNodeInfo_t& info_, unsigned current_step):
        ProbSearchNode<PSearchNodeInfo_t>(state_id, info_, current_step) {
    }
    inline void set_v_alternative(QValue_type v, LocalOpIndex_type local_op) {
        this->info.v_alternative = v;
        this->info.policy_alternative = local_op;
    }
    [[nodiscard]] inline QValue_type get_v_alternative() const { return this->info.v_alternative; }
};

/**SEARCH SPACE********************************************************************************************************/

class ProbSearchSpaceBase {
protected:
    // explicit-state structures
    // const ModelInformation& modelInformation;
    std::unique_ptr<StateRegistry> stateRegistry;
    std::unique_ptr<State> initialState;
    StateSpaceProb stateSpace;
    std::size_t current_step; // id of current sub-search, e.g. for FRET

    // output & stats:
    void dump_policy_graph_aux(StateID_type root_state, const segmented_vector::SegmentedVector<ProbSearchNodeInfo>& data, std::ostream& out = std::cout) const;
    void dump_policy_graph_aux(StateID_type root_state, const segmented_vector::SegmentedVector<UpperAndLowerPNodeInfo>& data, std::ostream& out = std::cout) const;

    explicit ProbSearchSpaceBase(const ModelInformation& modelInformation);
public:
    virtual ~ProbSearchSpaceBase() = 0;

    // access methods:
    [[nodiscard]] inline const State& get_initialState() const { return *initialState; }
    inline StateSpaceProb& get_underlying_state_space() { return stateSpace; }
    [[nodiscard]] inline const StateSpaceProb& get_underlying_state_space() const { return stateSpace; }
    [[nodiscard]] virtual QValue_type get_initial_value() const = 0;
    //
    [[nodiscard]] inline std::size_t _current_step() const { return current_step; }
    inline void increase_current_step() { ++current_step; }
    //
    [[nodiscard]] inline State get_state(StateID_type state_id) const { return stateRegistry->lookup_state(state_id); }

    // output & stats:
    void print_statistics() const;
    void stats_to_csv(std::ofstream& file) const;
    static void stat_names_to_csv(std::ofstream& file);
};


template<template<typename, typename> class QVal_t, typename PSearchNodeInfo_t, template<typename> class PSearchNode_t>
class ProbSearchSpace: public ProbSearchSpaceBase {
    typedef PSearchNode_t<PSearchNodeInfo_t> PSearchNode;
private:
    // information structures
    QVal_t<ProbSearchSpace, PSearchNode> qVal;   // computation of Q-Value w.r.t. optimization criteria
    segmented_vector::SegmentedVector<PSearchNodeInfo_t> data;     // search information per state

public:
    explicit ProbSearchSpace(const ModelInformation& modelInformation): ProbSearchSpaceBase(modelInformation), qVal(*this), data() {}
    ~ProbSearchSpace() override = default;

    // access methods:
    inline std::size_t number_of_states() { return data.size(); }
    [[nodiscard]] QValue_type get_initial_value() const override { PLAJA_ASSERT(0 < data.size()) return data[0].v_current; }

    // output & stats:
    inline void dump_policy_graph(StateID_type root_state, std::ostream& out = std::cout) const { this->dump_policy_graph_aux(root_state, data, out); }

    // Get NODES & STATES:

    /**
     * Add node to search space if necessary.
     * Can also be used as normal get_node.
     * @param state
     * @return
     */
    PSearchNode add_node(const State& state) {
        StateID_type state_id = state.get_id_value();
        if (state_id >= data.size()) {
            data.resize(state_id + 1);
        }
        return PSearchNode(state_id, data[state_id], current_step);
    }
    inline PSearchNode get_node(StateID_type state_id) {
        PLAJA_ASSERT(state_id < data.size())
        return PSearchNode(state_id, data[state_id], current_step);
    }
    // inline PSearchNode get_node(const State& state) { return get_node(state.get_id_value()); } // if it becomes necessary one can also add add_node res. get_node for StateID_type res. State

    // Q-Value computation:
    /**
     * Get state value.
     * @param state_node
     * @return
     */
    inline QValue_type get_value(const PSearchNode& state_node) const { return qVal.get_value(state_node); }

    /**
     * Compute Q-value of state and respective action operator.
     * @param state_node
     * @param local_op (local w.r.t. to state)
     * @return
     */
    inline QValue_type compute_q_val(const PSearchNode& state_node, LocalOpIndex_type local_op) const {
        return qVal.compute_value(state_node, local_op);
    }

    /**
     * Compute greedy action and respective Q-value.
     * Must not be called on absorbing states.
     * @param state
     * @param q_val_op
     * @return pair of absolute difference between computed and current state value and greedy action.
     */
    inline std::pair<QValue_type, QValLocalOp> compute_greedy_action(const PSearchNode& state_node) const {
        QValLocalOp q_val_op = qVal.compute_greedy_action(state_node);
        return std::pair<QValue_type, QValLocalOp>(std::fabs(get_value(state_node) - q_val_op.qValue), q_val_op);
    }

    /**
     * Compute all greedy action operators up to a tolerance delta.
     * @param state_node
     * @param delta
    */
    inline QValLocalOps compute_greedy_actions(const PSearchNode& state_node, QValue_type delta = 0) const {
        return qVal.compute_greedy_actions(state_node, delta);
    }

    /**
     * Test whether a new value is worse than current state value.
     * @param state
     * @param new_val
     * @param epsilon, tolerance in favour of the new value
     * @return
    */
    inline bool qval_is_worse(const PSearchNode& state_node, QValue_type new_val, QValue_type epsilon = 0) const {
        return qVal.compare(state_node, new_val, epsilon);
    }

    /**
     * Compute greedy action & respective Q-value and update state node accordingly.
     * Must not be called on absorbing states.
     * @param state_node
     * @return pair of absolute difference between old and new state value and greedy action.
     */
    inline std::pair<QValue_type, QValLocalOp> update_q_val(PSearchNode& state_node) {
        return qVal.update(state_node);
    }

};

// Minimal expected cost for cost on EXIT
template<typename PSearchNodeInfo_t, template<typename> class PSearchNode_t>
struct ProbSearchSpaceCMin : public ProbSearchSpace<QValMin, PSearchNodeInfo_t, PSearchNode_t> {
    explicit ProbSearchSpaceCMin(const ModelInformation& modelInfo):
        ProbSearchSpace<QValMin, PSearchNodeInfo_t, PSearchNode_t>(modelInfo) {}
};

// Probability costs
template<typename PSearchNodeInfo_t, template<typename> class PSearchNode_t>
struct ProbSearchSpacePMin: public ProbSearchSpace<QValMin, PSearchNodeInfo_t, PSearchNode_t> {
    explicit ProbSearchSpacePMin(const ModelInformation& modelInfo):
        ProbSearchSpace<QValMin, PSearchNodeInfo_t, PSearchNode_t>(modelInfo) {}
};

template<typename PSearchNodeInfo_t, template<typename> class PSearchNode_t>
struct ProbSearchSpacePMax: public ProbSearchSpace<QValMax, PSearchNodeInfo_t, PSearchNode_t> {
    explicit ProbSearchSpacePMax(const ModelInformation& modelInfo):
        ProbSearchSpace<QValMax, PSearchNodeInfo_t, PSearchNode_t>(modelInfo) {}
};

// Duals

template<typename PSearchNodeInfo_t, template<typename> class PSearchNode_t>
struct ProbSearchSpaceDualCMin: public ProbSearchSpace<DualQValMin, PSearchNodeInfo_t, PSearchNode_t> {
    explicit ProbSearchSpaceDualCMin(const ModelInformation& modelInfo):
        ProbSearchSpace<DualQValMin, PSearchNodeInfo_t, PSearchNode_t>(modelInfo) {}
};

template<typename PSearchNodeInfo_t, template<typename> class PSearchNode_t>
struct ProbSearchSpaceDualPMin: public ProbSearchSpace<DualQValMin, PSearchNodeInfo_t, PSearchNode_t> {
    explicit ProbSearchSpaceDualPMin(const ModelInformation& modelInfo):
        ProbSearchSpace<DualQValMin, PSearchNodeInfo_t, PSearchNode_t>(modelInfo) {}
};

template<typename PSearchNodeInfo_t, template<typename> class PSearchNode_t>
struct ProbSearchSpaceDualPMax: public ProbSearchSpace<DualQValMax, PSearchNodeInfo_t, PSearchNode_t> {
    explicit ProbSearchSpaceDualPMax(const ModelInformation& modelInfo):
        ProbSearchSpace<DualQValMax, PSearchNodeInfo_t, PSearchNode_t>(modelInfo) {}
};

#endif //PLAJA_PROB_SEARCH_SPACE_H
