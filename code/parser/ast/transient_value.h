//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_TRANSIENT_VALUE_H
#define PLAJA_TRANSIENT_VALUE_H


#include <memory>
#include "ast_element.h"
#include "commentable.h"

// forward declaration:
class Expression;
class LValueExpression;
class VariableExpression;
class StateBase;
class StateValues;

/**
 * Location substructure.
 */
class TransientValue: public AstElement, public Commentable {
private:
    std::unique_ptr<LValueExpression> ref;
    std::unique_ptr<Expression> value;
public:
    TransientValue();
    ~TransientValue() override;

    // setter:
    void set_ref(std::unique_ptr<LValueExpression>&& ref_);
    void set_value(std::unique_ptr<Expression>&& val);

    // getter:
    inline LValueExpression* get_ref() { return ref.get(); }
    [[nodiscard]] inline const LValueExpression* get_ref() const { return ref.get(); }
    inline Expression* get_value() { return value.get(); }
    [[nodiscard]] inline const Expression* get_value() const { return value.get(); }

    /**
     * Deep copy of an transient-value.
     * @return
     */
    [[nodiscard]] std::unique_ptr<TransientValue> deepCopy() const;

    void evaluate(const StateBase* source, StateBase* target) const;

    // override:
    void accept(AstVisitor* astVisitor) override;
	void accept(AstVisitorConst* astVisitor) const override;
};


#endif //PLAJA_TRANSIENT_VALUE_H
