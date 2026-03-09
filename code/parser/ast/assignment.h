//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_ASSIGNMENT_H
#define PLAJA_ASSIGNMENT_H

#include <memory>
#include "../using_parser.h"
#include "../../search/states/forward_states.h"
#include "expression/forward_expression.h"
#include "ast_element.h"
#include "commentable.h"

class Assignment final: public AstElement, public Commentable {
private:
    std::unique_ptr<LValueExpression> ref;

    /* If lowerBound or upperBound assignment is non-deterministic and value must be NULL. */
    std::unique_ptr<Expression> value;
    std::unique_ptr<Expression> lowerBound;
    std::unique_ptr<Expression> upperBound;

    SequenceIndex_type index; // Index for sequence of atomic expressions, defaults to 0.

public:
    Assignment();
    ~Assignment() override;
    DELETE_CONSTRUCTOR(Assignment)

    /* setter */

    void set_ref(std::unique_ptr<LValueExpression>&& ref_);

    void set_value(std::unique_ptr<Expression>&& val);

    void set_lower_bound(std::unique_ptr<Expression>&& lb);

    void set_upper_bound(std::unique_ptr<Expression>&& ub);

    inline void set_index(SequenceIndex_type seq_index) { index = seq_index; }

    /* getter */

    [[nodiscard]] inline LValueExpression* get_ref() { return ref.get(); }

    [[nodiscard]] inline const LValueExpression* get_ref() const { return ref.get(); }

    [[nodiscard]] inline Expression* get_value() { return value.get(); }

    [[nodiscard]] inline const Expression* get_value() const { return value.get(); }

    [[nodiscard]] inline const Expression* get_lower_bound() const { return lowerBound.get(); }

    [[nodiscard]] inline Expression* get_lower_bound() { return lowerBound.get(); }

    [[nodiscard]] inline const Expression* get_upper_bound() const { return upperBound.get(); }

    [[nodiscard]] inline Expression* get_upper_bound() { return upperBound.get(); }

    [[nodiscard]] inline SequenceIndex_type get_index() const { return index; }

    /* */
    [[nodiscard]] inline bool is_deterministic() const { return value.get(); }

    [[nodiscard]] inline bool is_non_deterministic() const { return not is_deterministic(); }

    /* LValueExpression shortcuts */
    [[nodiscard]] VariableID_type get_updated_var_id() const;
    [[nodiscard]] const Expression* get_array_index() const;
    [[nodiscard]] VariableIndex_type get_variable_index(const StateBase* source) const;

    /** Deep copy of an assignment. */
    [[nodiscard]] std::unique_ptr<Assignment> deep_copy() const;

    void evaluate(const StateBase* current, StateBase* target) const;

    /** Assignment agrees with the given source-target combination. */
    [[nodiscard]] bool agrees(const StateBase& source, const StateBase& target) const;

    /**
     * To be used for non-deterministic assignments.
     * If value in target is out-of-bounds, then clip at lower/upper bound.
     */
    void clip(const StateBase* source, StateBase& target) const;

    /* Override. */
    void accept(AstVisitor* ast_visitor) override;
    void accept(AstVisitorConst* ast_visitor) const override;
};

#endif //PLAJA_ASSIGNMENT_H
