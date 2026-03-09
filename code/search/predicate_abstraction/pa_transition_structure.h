//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PA_TRANSITION_STRUCTURE_H
#define PLAJA_PA_TRANSITION_STRUCTURE_H

#include <memory>
#include "pa_states/forward_pa_states.h"
#include "successor_generation/forward_succ_gen_pa.h"
#include "../using_search.h"

struct PATransitionStructure final {

private:
    /* Basic data. */
    std::unique_ptr<AbstractState> source;
    std::unique_ptr<AbstractState> target;

    ActionLabel_type actionLabel;
    ActionOpID_type actionOpId;
    UpdateIndex_type updateIndex;
    UpdateOpID_type updateOpId;

    /* More fancy data. */
    const ActionOpPA* actionOp;
    const UpdatePA* update;

public:
    PATransitionStructure();
    PATransitionStructure(const PATransitionStructure& other);
    ~PATransitionStructure();

    PATransitionStructure& operator=(const PATransitionStructure& other);

    void set_source(std::unique_ptr<AbstractState>&& pa_src);
    void unset_source();
    void set_target(std::unique_ptr<AbstractState>&& pa_target);
    void unset_target();

    inline void set_action_label(ActionLabel_type action_label) { actionLabel = action_label; }

    inline void unset_action_label() { actionLabel = ACTION::noneLabel; }

    inline void set_action_op(ActionOpID_type action_op_id) { actionOpId = action_op_id; }

    void set_action_op(const ActionOpPA& action_op);

    inline void unset_action_op() {
        actionOpId = ACTION::noneOp;
        actionOp = nullptr;
    }

    inline void set_update(UpdateIndex_type update_index) { updateIndex = update_index; }

    inline void set_update_op_id(UpdateOpID_type update_op) { updateOpId = update_op; }

    void set_update(UpdateIndex_type update_index, const UpdatePA& update);

    inline void unset_update() {
        updateIndex = ACTION::noneUpdate;
        updateOpId = ACTION::noneUpdateOp;
        update = nullptr;
    }

    [[nodiscard]] inline const AbstractState* _source() const { return source.get(); }

    [[nodiscard]] StateID_type _source_id() const;

    [[nodiscard]] inline const AbstractState* _target() const { return target.get(); }

    [[nodiscard]] StateID_type _target_id() const;

    [[nodiscard]] inline ActionLabel_type _action_label() const { return actionLabel; }

    [[nodiscard]] inline ActionOpID_type _action_op_id() const { return actionOpId; }

    [[nodiscard]] inline UpdateIndex_type _update_index() const { return updateIndex; }

    [[nodiscard]] inline const ActionOpPA* _action_op() const { return actionOp; }

    [[nodiscard]] inline const UpdatePA* _update() const { return update; }

    [[nodiscard]] inline UpdateOpID_type get_update_op_id() const { return updateOpId; }

    [[nodiscard]] inline bool has_action_label() const { return actionLabel != ACTION::noneLabel; }

    [[nodiscard]] inline bool has_action_op() const { return actionOpId != ACTION::noneOp; }

    [[nodiscard]] inline bool has_update() const { return updateIndex != ACTION::noneUpdate; }

};

#endif //PLAJA_PA_TRANSITION_STRUCTURE_H
