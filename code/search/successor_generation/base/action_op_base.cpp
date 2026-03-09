//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <sstream>
#include "action_op_base.h"
#include "../../../parser/ast/expression/expression.h"
#include "../../../parser/ast/model.h"
#include "../../../parser/ast/edge.h"
#include "../../../parser/visitor/extern/variables_of.h"
#include "../../../parser/visitor/extern/to_normalform.h"
#include "../../../utils/utils.h"
#include "../../information/model_information.h"

ActionOpBase::ActionOpBase(ActionLabel_type action_label, ActionOpID_type op_id): // NOLINT(*-easily-swappable-parameters)
    actionLabel(action_label)
    , opId(op_id) {
}

ActionOpBase::~ActionOpBase() = default;

/**********************************************************************************************************************/

ActionOpBase::LocationIterator::LocationIterator(const std::vector<const Edge*>& edges):
    it(edges.cbegin())
    , itEnd(edges.cend()) {}

VariableIndex_type ActionOpBase::LocationIterator::loc() const { return (*it)->get_location_index(); }

PLAJA::integer ActionOpBase::LocationIterator::src() const { return (*it)->get_location_value(); }

ActionOpBase::GuardIterator::GuardIterator(const std::vector<const Edge*>& edges):
    it(edges.cbegin())
    , itEnd(edges.cend()) {
    for (; it != itEnd; ++it) { if ((*it)->get_guardExpression()) { break; } }
}

void ActionOpBase::GuardIterator::operator++() {
    for (++it; it != itEnd; ++it) { if ((*it)->get_guardExpression()) { break; } }
}

const Expression* ActionOpBase::GuardIterator::operator()() const { return (*it)->get_guardExpression(); }

const Expression* ActionOpBase::GuardIterator::operator->() const { return (*it)->get_guardExpression(); }

const Expression& ActionOpBase::GuardIterator::operator*() const { return *(*it)->get_guardExpression(); }

/**********************************************************************************************************************/

std::unordered_set<ConstantIdType> ActionOpBase::collect_op_constants() const {
    std::unordered_set<ConstantIdType> op_constants;
    for (const auto& edge: edges) { op_constants.merge(VARIABLES_OF::constants_of(*edge, true)); }
    return op_constants;
}

std::unordered_set<VariableIndex_type> ActionOpBase::collect_guard_locs() const {
    std::unordered_set<VariableIndex_type> guard_locs;
    for (auto it = locationIterator(); !it.end(); ++it) { guard_locs.insert(it.src()); }
    return guard_locs;
}

std::unordered_set<VariableID_type> ActionOpBase::collect_guard_vars() const {
    std::unordered_set<VariableID_type> guard_vars;
    for (auto it = guardIterator(); !it.end(); ++it) { guard_vars.merge(VARIABLES_OF::vars_of(it(), true)); }
    return guard_vars;
}

std::unordered_set<VariableIndex_type> ActionOpBase::collect_guard_state_indexes(bool include_locs) const {
    std::unordered_set<VariableIndex_type> guard_vars;
    for (auto it = guardIterator(); !it.end(); ++it) { guard_vars.merge(VARIABLES_OF::state_indexes_of(it(), true, not include_locs)); }
    return guard_vars;
}

std::unique_ptr<Expression> ActionOpBase::guard_to_conjunction() const {
    std::list<std::unique_ptr<Expression>> guard_conjunction;
    for (auto guard_it = guardIterator(); !guard_it.end(); ++guard_it) { guard_conjunction.push_back(guard_it->deepCopy_Exp()); }
    if (guard_conjunction.empty()) { return nullptr; } // Special case.
    auto rlt = TO_NORMALFORM::construct_conjunction(std::move(guard_conjunction));
    TO_NORMALFORM::to_nary(rlt);
    return rlt;
}

std::list<std::unique_ptr<Expression>> ActionOpBase::guard_to_splits() const {
    std::list<std::unique_ptr<Expression>> guard_conjunction;
    for (auto guard_it = guardIterator(); !guard_it.end(); ++guard_it) {
        guard_conjunction.splice(guard_conjunction.cend(), TO_NORMALFORM::normalize_and_split(*guard_it, false));
    }
    return guard_conjunction;
}

std::unordered_set<VariableID_type> ActionOpBase::collect_updates_vars() const {

    std::unordered_set<VariableID_type> updated_vars;

    for (const auto& update: updates) {
        PLAJA_ASSERT(update.get_num_sequences() == 1)
        updated_vars.merge(update.collect_updated_vars(0));
    }

    return updated_vars;

}

std::unordered_set<VariableID_type> ActionOpBase::infer_non_updates_vars() const {
    const auto updated_vars = collect_updates_vars();
    PLAJA_ASSERT(not updates.empty())
    return updates.front().infer_non_updated_vars(0, &updated_vars);
}

/**********************************************************************************************************************/

std::string ActionOpBase::label_to_str(ActionLabel_type action_label, const Model* model) {
    return model ? model->get_action_name(ModelInformation::action_label_to_id(action_label)) : std::to_string(action_label);
}

std::string ActionOpBase::label_to_str(const Model* model) const { return ActionOpBase::label_to_str(actionLabel, model); }

std::string ActionOpBase::to_str(const Model* model, bool readable) const {
    std::stringstream ss;
    ss << label_to_str(model);
    for (const auto* edge: edges) { ss << std::endl << edge->to_string(readable); }
    return ss.str();
}

void ActionOpBase::dump(const Model* model, bool readable) const {
    PLAJA_LOG(label_to_str(model));
    for (const auto* edge: edges) { edge->dump(readable); }
}

std::string ActionOpBase::update_to_str(UpdateIndex_type update) { return PLAJA_UTILS::string_f("update: %u", update); }
