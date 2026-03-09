//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_STATE_CONDITION_EXPRESSION_H
#define PLAJA_STATE_CONDITION_EXPRESSION_H

#include "../../iterators/ast_iterator.h"
#include "../expression.h"
#include "../forward_expression.h"

class StateConditionExpression final: public Expression {
private:
    std::vector<std::unique_ptr<LocationValueExpression>> locValues;
    std::unique_ptr<Expression> constraint;

public:
    StateConditionExpression();
    ~StateConditionExpression() override;
    DELETE_CONSTRUCTOR(StateConditionExpression)

    /* Static. */

    [[nodiscard]] static const std::string& get_op_string();

    [[nodiscard]] static std::unique_ptr<StateConditionExpression> transform(std::unique_ptr<Expression>&& expr);

    /* Construction. */

    void reserve(std::size_t loc_values_cap);

    void add_loc_value(std::unique_ptr<LocationValueExpression>&& value);

    /* Setter. */

    void set_loc_value(std::unique_ptr<LocationValueExpression>&& value, std::size_t index);

    inline std::unique_ptr<Expression> set_constraint(std::unique_ptr<Expression>&& exp = nullptr) {
        std::swap(constraint, exp);
        return std::move(exp);
    }

    //* Getter. */

    [[nodiscard]] inline std::size_t get_size_loc_values() const { return locValues.size(); }

    [[nodiscard]] inline const LocationValueExpression* get_loc_value(std::size_t index) const {
        PLAJA_ASSERT(index < locValues.size())
        return locValues[index].get();
    }

    [[nodiscard]] inline LocationValueExpression* get_loc_value(std::size_t index) {
        PLAJA_ASSERT(index < locValues.size())
        return locValues[index].get();
    }

    [[nodiscard]] inline const Expression* get_constraint() const { return constraint.get(); }

    [[nodiscard]] inline Expression* get_constraint() { return constraint.get(); }

    /* Iterators. */

    [[nodiscard]] inline AstConstIterator<LocationValueExpression> init_loc_value_iterator() const { return AstConstIterator(locValues); }

    [[nodiscard]] inline AstIterator<LocationValueExpression> init_loc_value_iterator() { return AstIterator(locValues); }

    /**/

    [[nodiscard]] bool check_location_constraint(const StateBase* state) const;
    [[nodiscard]] bool check_state_variable_constraint(const StateBase* state) const;
    [[nodiscard]] bool check_location_constraint(const class AbstractState* state) const;

    /* Override. */

    PLAJA::integer evaluateInteger(const StateBase* state) const override;
    [[nodiscard]] bool is_constant() const override;

    bool equals(const Expression* exp) const override;
    [[nodiscard]] std::size_t hash() const override;

    [[nodiscard]] std::unique_ptr<Expression> deepCopy_Exp() const override;
    [[nodiscard]] std::unique_ptr<Expression> move_exp() override;

    void accept(AstVisitor* ast_visitor) override;
    void accept(AstVisitorConst* ast_visitor) const override;

    /* */

    [[nodiscard]] std::unique_ptr<StateConditionExpression> deep_copy() const;

    [[nodiscard]] std::unique_ptr<StateConditionExpression> move();

    void move_locs_from(StateConditionExpression& other);

    void move_constraint_from(StateConditionExpression& other);

};

#endif //PLAJA_STATE_CONDITION_EXPRESSION_H
