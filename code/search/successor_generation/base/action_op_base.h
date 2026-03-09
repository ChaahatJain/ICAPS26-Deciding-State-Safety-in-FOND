//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_ACTION_OP_BASE_H
#define PLAJA_ACTION_OP_BASE_H

#include <memory>
#include <vector>
#include <string>
#include "../../../parser/ast/forward_ast.h"
#include "../../../utils/default_constructors.h"
#include "../../../assertions.h"
#include "../../states/forward_states.h"
#include "../../using_search.h"
#include "../forward_successor_generation.h"
#include "../update.h"
#include "forward_successor_generation_base.h"

class ActionOpBase {

protected:
    ActionLabel_type actionLabel;
    ActionOpID_type opId;  // Unique ID for the respective edge/sync combination.
    std::vector<const Edge*> edges;

    std::vector<Update> updates;

    ActionOpBase(ActionLabel_type action_label, ActionOpID_type op_id);

public:
    virtual ~ActionOpBase() = 0;
    DEFAULT_CONSTRUCTOR_ONLY(ActionOpBase)
    DELETE_ASSIGNMENT(ActionOpBase)

    [[nodiscard]] inline ActionLabel_type _action_label() const { return actionLabel; }

    [[nodiscard]] inline ActionOpID_type _op_id() const { return opId; }

    [[nodiscard]] inline std::size_t size() const { return updates.size(); }

    [[nodiscard]] inline const Update& get_update(UpdateIndex_type index) const {
        PLAJA_ASSERT(index < updates.size())
        return updates[index];
    }

    /* Auxiliaries. */

    [[nodiscard]] std::unordered_set<ConstantIdType> collect_op_constants() const;

    [[nodiscard]] std::unordered_set<VariableIndex_type> collect_guard_locs() const;

    [[nodiscard]] std::unordered_set<VariableID_type> collect_guard_vars() const;

    [[nodiscard]] std::unordered_set<VariableIndex_type> collect_guard_state_indexes(bool include_locs = false) const;

    [[nodiscard]] std::unique_ptr<Expression> guard_to_conjunction() const;

    [[nodiscard]] std::list<std::unique_ptr<Expression>> guard_to_splits() const;

    [[nodiscard]] std::unordered_set<VariableID_type> collect_updates_vars() const;

    [[nodiscard]] std::unordered_set<VariableID_type> infer_non_updates_vars() const;

    /* Outputs. */

    [[nodiscard]] static std::string label_to_str(ActionLabel_type action_label, const Model* model = nullptr);

    [[nodiscard]] std::string label_to_str(const Model* model = nullptr) const;

    [[nodiscard]] std::string to_str(const Model* model, bool readable) const;

    void dump(const Model* model, bool readable) const;

    [[nodiscard]] static std::string update_to_str(UpdateIndex_type update);

    /** iterators *****************************************************************************************************/

    class LocationIterator final {
        friend ActionOpBase;

    private:
        std::vector<const Edge*>::const_iterator it;
        std::vector<const Edge*>::const_iterator itEnd;

        explicit LocationIterator(const std::vector<const Edge*>& edges);
    public:
        ~LocationIterator() = default;
        DELETE_CONSTRUCTOR(LocationIterator)

        inline void operator++() { ++it; }

        [[nodiscard]] inline bool end() const { return it == itEnd; }

        [[nodiscard]] VariableIndex_type loc() const;
        [[nodiscard]] PLAJA::integer src() const;
    };

    [[nodiscard]] inline LocationIterator locationIterator() const { return LocationIterator(edges); }

    /**/

    class GuardIterator final {
        friend ActionOpBase;

    private:
        std::vector<const Edge*>::const_iterator it;
        const std::vector<const Edge*>::const_iterator itEnd; // NOLINT(*-avoid-const-or-ref-data-members)
        explicit GuardIterator(const std::vector<const Edge*>& edges);
    public:
        ~GuardIterator() = default;
        DELETE_CONSTRUCTOR(GuardIterator)

        [[nodiscard]] inline bool end() const { return it == itEnd; }

        void operator++();
        [[nodiscard]] const Expression* operator()() const;
        [[nodiscard]] const Expression* operator->() const;
        [[nodiscard]] const Expression& operator*() const;
    };

    [[nodiscard]] inline GuardIterator guardIterator() const { return GuardIterator(edges); }

    /**/

    class UpdateIteratorBase {

    private:
        const std::vector<Update>* updates;

    protected:
        UpdateIndex_type refIndex;

        explicit UpdateIteratorBase(const std::vector<Update>& updates_ref):
            updates(&updates_ref)
            , refIndex(0) {
        }

    public:
        ~UpdateIteratorBase() = default;
        DELETE_CONSTRUCTOR(UpdateIteratorBase)

        [[nodiscard]] inline bool end() const { return refIndex >= updates->size(); }

        [[nodiscard]] inline const Update& update() const { return (*updates)[refIndex]; }

        [[nodiscard]] inline UpdateIndex_type update_index() const { return refIndex; }
    };

};

#endif //PLAJA_ACTION_OP_BASE_H
