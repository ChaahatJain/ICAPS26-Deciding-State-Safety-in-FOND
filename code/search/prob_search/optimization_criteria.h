//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_OPTIMIZATION_CRITERIA_H
#define PLAJA_OPTIMIZATION_CRITERIA_H

#include <cmath>
#include <iostream>
#include <vector>

#include "../../utils/structs_utils/qval_local_op.h"
#include "../../utils/structs_utils/qval_local_ops.h"
#include "../../utils/floating_utils.h"
#include "state_space.h"

/***
 * Purpose: Implementation of several optimization criteria in a parameterized fashion.
 * How to evaluate & update a state node,
 * how to compute the Q-value and the greedy action operator,
 * with respect to different objectives.
 */

/**********************************************************************************************************************/
/// GET THE VALUE (and cost) of a state:

/**
 * Cost on EXIT, before taking a transition;
 * in contrast to STEP.
 */
struct OnExitCost {
    template<typename StateNode_t>
    inline PLAJA::Cost_type get_exit_cost(const StateNode_t& state_node) const {
        return state_node.get_cost();
    }
};

struct OnExitZeroCost {
    template<typename StateNode_t>
    inline PLAJA::Cost_type get_exit_cost(const StateNode_t& /*state_node*/) const {
        return 0;
    }
};

/* Not supported, not implemented
struct _OnStepsCost {

};

struct _OnStepsZeroCost {

};
*/

// For the following cf. v_current and v_alternative in prob_search_node_info.h and prob_search_space.h

struct CurrentVal_ {
    template<typename StateNode_t>
    inline QValue_type operator()(const StateNode_t& state_node) const {
        return state_node.get_v_current();
    }

    template<typename StateNode_t>
    inline QValue_type update(StateNode_t& state_node, QValLocalOp& qValOp) const {
        QValue_type old = state_node.get_v_current();
        state_node.set_v_current(qValOp.qValue, qValOp.localOpIndex);
        return std::fabs(old - qValOp.qValue);
    }
};

struct AlternativeVal_ {
    template<typename StateNode_t>
    inline QValue_type operator()(const StateNode_t& state_node) const {
        return state_node.get_v_alternative();
    }

    template<typename StateNode_t>
    inline QValue_type update(StateNode_t& state_node, QValLocalOp& qValOp) const {
        QValue_type old = state_node.get_v_alternative();
        state_node.set_v_alternative(qValOp.qValue, qValOp.localOpIndex);
        return std::fabs(old - qValOp.qValue);
    }
};

// Combinations (currently used)

struct CurrentValOnExitCost: public OnExitCost, /*public _OnStepsZeroCost,*/ public CurrentVal_ {};
struct AlternativeValOnExitCost: public OnExitCost, /*public _OnStepsZeroCost,*/ public AlternativeVal_ {};

/**********************************************************************************************************************/
/// Actual OPTIMIZATION:

// Q-value base class for common computations
// (at cost of more virtual functions more function from QValMax_ and QValMin_ could be added):

template<typename PSearchSpace_t, typename StateNode_t, typename StateVal_t>
struct QValShared_ {
protected:
    const StateVal_t stateVal; // How to evaluate a state (see above).
    PSearchSpace_t& searchSpace; // note changes to the searchSpace does to affect the constant-nes of QValShared, not technically (when we modify function-argument state-nodes from that very search space) but also not formally as the reference does not change
    const StateSpaceProb& stateSpace;

    /**
     * Get all local action operator indices that are greedy w.r.t to a given q-value within an interval delta.
     * @param state_node
     * @param q_val
     * @param delta
     */
    std::vector<LocalOpIndex_type> extract_greedy_actions(const StateNode_t& state_node, const QValue_type q_val, const QValue_type delta = 0) const {
        std::vector<LocalOpIndex_type> ops;
        //
        QValue_type val;
        for (auto app_op_it = stateSpace.applicableOpIterator(state_node.get_state_id()); !app_op_it.end(); ++app_op_it) {
            val = compute_value(state_node, app_op_it.local_op_index());
            if (std::fabs(val - q_val) <= delta) {
                ops.push_back(app_op_it.local_op_index());
            }
        }
        //
        return ops;
    }

public:
    explicit QValShared_(PSearchSpace_t& search_space): stateVal(), searchSpace(search_space), stateSpace(searchSpace.get_underlying_state_space()) {}
    virtual ~QValShared_() = 0;

    inline QValue_type get_value(const StateNode_t& state_node) const { return this->stateVal(state_node); }

    /**
    * Compute QValue of state and respective action operator.
    * @param state_node
    * @param local_op (local w.r.t. to state)
    * @return
    */
    inline QValue_type compute_value(const StateNode_t& state_node, LocalOpIndex_type local_op) const {
        QValue_type v = stateVal.get_exit_cost(state_node);
        for (auto per_op_it = stateSpace.perOpTransitionIterator(state_node.get_state_id(), local_op); !per_op_it.end(); ++per_op_it) {
            v += per_op_it.prob() * stateVal(searchSpace.get_node(per_op_it.id()));
        }
        return v;
    }

};

template<typename PSearchSpace_t, typename StateNode_t, typename StateVal_t>
QValShared_<PSearchSpace_t, StateNode_t, StateVal_t>::~QValShared_<PSearchSpace_t, StateNode_t, StateVal_t>() = default;

/**********************************************************************************************************************/

template<typename PSearchSpace_t, typename StateNode_t, typename StateVal_t>
struct QValMax_ : public QValShared_<PSearchSpace_t, StateNode_t, StateVal_t> {

    explicit QValMax_(PSearchSpace_t& search_space): QValShared_<PSearchSpace_t, StateNode_t, StateVal_t>(search_space) {}
    ~QValMax_() override = default;

    /**
     * Compute greedy action operator and respective q-value.
     * However, may not be called on absorbing state
     * @param state_node
     * @return
     */
    QValLocalOp compute_greedy_action(const StateNode_t& state_node) const {
        auto app_op_it = this->stateSpace.applicableOpIterator(state_node.get_state_id());
        PLAJA_ASSERT(!app_op_it.end()) // must be non-absorbing
        // init:
        QValLocalOp q_val_op(this->compute_value(state_node, app_op_it.local_op_index()), app_op_it.local_op_index());
        ++app_op_it;
        // compute max:
        QValue_type val;
        for (; !app_op_it.end(); ++app_op_it) {
            val = this->compute_value(state_node, app_op_it.local_op_index());
            if (val > q_val_op.qValue) {
                q_val_op.qValue = val;
                q_val_op.localOpIndex = app_op_it.local_op_index();
            }
        }
        //
        return q_val_op;
    }

    /**
     * Test whether a new value is *worse*.
     * @param state_node
     * @param val
     * @param delta, tolerance in favour of the new value
     * @return
     */
    bool compare(const StateNode_t& state_node, QValue_type val, QValue_type delta = 0) const {
        return PLAJA_FLOATS::lt(val, this->stateVal(state_node), delta);
    }

    // The following functions are the same as for QValMin_, but call compute_greedy_action would have to be virtual if the following classes were in QValShared_:

    // see extract_greedy_actions in QValShared_
    QValLocalOps compute_greedy_actions(const StateNode_t& state_node, QValue_type delta = 0) const {
        QValLocalOp q_val_op = this->compute_greedy_action(state_node);
        return QValLocalOps(q_val_op, this->extract_greedy_actions(state_node, q_val_op.qValue, delta));
    }

    /**
     * Compute Q-value and update the state value respectively.
     * However, may not be called on absorbing state.
     * @param state_node
     * @return
     */
    inline std::pair<QValue_type, QValLocalOp> update(StateNode_t& state_node) const {
        QValLocalOp q_val_op = this->compute_greedy_action(state_node);
        return std::pair<QValue_type, QValLocalOp>(this->stateVal.update(state_node, q_val_op), q_val_op);
    }

    /**
     * Apply update and extract action operators greedy with respect to Q-value.
     * @param state_node
     * @param delta
     * @return
     */
    inline std::pair<QValue_type, QValLocalOps> update_greedy(StateNode_t& state_node, QValue_type delta = 0) { // see extract_greedy_actions above QValShared_
        const auto update_diff_q_val_op = this->update(state_node);
        return std::pair<QValue_type, QValLocalOps>(update_diff_q_val_op.first, QValLocalOps(update_diff_q_val_op.second, this->extract_greedy_actions(state_node, update_diff_q_val_op.second.qValue, delta)));
    }

};

/**********************************************************************************************************************/

template<typename PSearchSpace_t, typename StateNode_t, typename StateVal_t>
struct QValMin_ : public QValShared_<PSearchSpace_t, StateNode_t, StateVal_t> {

    explicit QValMin_(PSearchSpace_t& search_space): QValShared_<PSearchSpace_t, StateNode_t, StateVal_t>(search_space) {}
    ~QValMin_() override = default;

    QValLocalOp compute_greedy_action(const StateNode_t& state_node) const {
        auto app_op_it = this->stateSpace.applicableOpIterator(state_node.get_state_id());
        PLAJA_ASSERT(!app_op_it.end()) // must be non-absorbing
        // init:
        QValLocalOp q_val_op(this->compute_value(state_node, app_op_it.local_op_index()), app_op_it.local_op_index());
        ++app_op_it;
        // compute max:
        QValue_type val;
        for (; !app_op_it.end(); ++app_op_it) {
            val = this->compute_value(state_node, app_op_it.local_op_index());
            if (val < q_val_op.qValue) {
                q_val_op.qValue = val;
                q_val_op.localOpIndex = app_op_it.local_op_index();
            }
        }
        //
        return q_val_op;
    }

    bool compare(const StateNode_t& state_node, QValue_type val, QValue_type delta = 0) const {
        return PLAJA_FLOATS::gt(val, this->stateVal(state_node), delta);
    }

    // The following functions are the same as for QValMax_, but call compute_greedy_action would have to be virtual if the following classes were in QValShared_:

    // see extract_greedy_actions in QValShared_
    QValLocalOps compute_greedy_actions(const StateNode_t& state_node, QValue_type delta = 0) const {
        QValLocalOp q_val_op = this->compute_greedy_action(state_node);
        return QValLocalOps(q_val_op, this->extract_greedy_actions(state_node, q_val_op.qValue, delta));
    }

    inline std::pair<QValue_type, QValLocalOp> update(StateNode_t& state_node) const {
        QValLocalOp q_val_op = this->compute_greedy_action(state_node);
        return std::pair<QValue_type, QValLocalOp>(this->stateVal.update(state_node, q_val_op), q_val_op);
    }

    // see extract_greedy_actions above QValShared_
    inline std::pair<QValue_type, QValLocalOps> update_greedy(StateNode_t& state_node, QValue_type delta = 0) {
        const auto update_diff_q_val_op = this->update(state_node);
        return std::pair<QValue_type, QValLocalOps>(update_diff_q_val_op.first, QValLocalOps(update_diff_q_val_op.second, this->extract_greedy_actions(state_node, update_diff_q_val_op.second.qValue, delta)));
    }

};

/**********************************************************************************************************************/

/**
 * Maintain two state values, typically v_current and v_alternative.
 * @tparam PSearchSpace_t
 * @tparam StateNode_t
 * @tparam QVal_t
 * @tparam StateVal_t1
 * @tparam StateVal_t2
 */
template<typename PSearchSpace_t, typename StateNode_t, template<typename, typename, typename> class QVal_t, typename StateVal_t1, typename StateVal_t2>
struct DualQVal {
public:
    const QVal_t<PSearchSpace_t, StateNode_t, StateVal_t1> q;
    const QVal_t<PSearchSpace_t, StateNode_t, StateVal_t2> q_alt;
    explicit DualQVal(PSearchSpace_t& search_space): q(search_space), q_alt(search_space) {}
    virtual ~DualQVal() = 0;

    // analog to QValMax_ and QValMin_, difference: functions are called for both state values, where the *non-alternative* is the one returned:

    inline QValue_type get_value(const StateNode_t& state_node) const {
        return q.get_value(state_node);
    }

    inline QValue_type compute_value(const StateNode_t& state_node, LocalOpIndex_type local_op) const {
        return q.compute_value(state_node, local_op);
    }

    inline QValLocalOp compute_greedy_action(const StateNode_t& state_node) const {
        return q.compute_greedy_action(state_node);
    }

    inline bool compare(const StateNode_t& state_node, QValue_type val, QValue_type delta = 0) const {
        return q.compare(state_node, val, delta);
    }

    inline QValLocalOps compute_greedy_actions(const StateNode_t& state_node, QValue_type delta = 0) const {
        return q.compute_greedy_actions(state_node, delta);
    }

    inline std::pair<QValue_type, QValLocalOp> update(StateNode_t& state_node) const {
        q_alt.update(state_node);
        return q.update(state_node);
    }

    inline std::pair<QValue_type, QValLocalOps> update_greedy(StateNode_t& state_node, QValue_type delta = 0) const {
        q_alt.update(state_node); // *not* update_greedy since the greedy actions are not used
        return q.update_greedy(state_node, delta);
    }

};

template<typename PSearchSpace_t, typename StateNode_t, template<typename, typename, typename> class QVal_t, typename StateVal_t1, typename StateVal_t2>
DualQVal<PSearchSpace_t, StateNode_t, QVal_t, StateVal_t1, StateVal_t2>::~DualQVal<PSearchSpace_t, StateNode_t, QVal_t, StateVal_t1, StateVal_t2>() = default;

/**********************************************************************************************************************/

// Currently used combinations:

template<typename PSearchSpace_t, typename StateNode_t>
struct QValMin : public QValMin_<PSearchSpace_t, StateNode_t, CurrentValOnExitCost>
{
    explicit QValMin(PSearchSpace_t& search_space)
        : QValMin_<PSearchSpace_t, StateNode_t, CurrentValOnExitCost>(search_space) {}
};

template<typename PSearchSpace_t, typename StateNode_t>
struct QValMax : public QValMax_<PSearchSpace_t, StateNode_t, CurrentValOnExitCost>
{
    explicit QValMax(PSearchSpace_t& search_space)
        : QValMax_<PSearchSpace_t, StateNode_t, CurrentValOnExitCost>(search_space) {}
};

template<typename PSearchSpace_t, typename StateNode_t>
struct DualQValMin : public DualQVal<PSearchSpace_t, StateNode_t, QValMin_, CurrentValOnExitCost, AlternativeValOnExitCost>
{
    explicit DualQValMin(PSearchSpace_t& search_space)
        : DualQVal<PSearchSpace_t, StateNode_t, QValMin_, CurrentValOnExitCost, AlternativeValOnExitCost>(search_space) {}
};

template<typename PSearchSpace_t, typename StateNode_t>
struct DualQValMax : public DualQVal<PSearchSpace_t, StateNode_t, QValMax_, CurrentValOnExitCost, AlternativeValOnExitCost>
{
    explicit DualQValMax(PSearchSpace_t& search_space)
        : DualQVal<PSearchSpace_t, StateNode_t, QValMax_, CurrentValOnExitCost, AlternativeValOnExitCost>(search_space) {}
};

#endif //PLAJA_OPTIMIZATION_CRITERIA_H
