//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_STATES_VALUES_EXPRESSION_H
#define PLAJA_STATES_VALUES_EXPRESSION_H

#include <unordered_set>
#include "../../iterators/ast_iterator.h"
#include "../expression.h"
#include "../forward_expression.h"

class StatesValuesExpression: public Expression {
private:
    std::vector<std::unique_ptr<StateValuesExpression>> statesValues;

public:
    StatesValuesExpression();
    ~StatesValuesExpression() override;
    DELETE_CONSTRUCTOR(StatesValuesExpression)

    /* Static. */
    [[nodiscard]] static const std::string& get_op_string();

    /* Construction. */
    void reserve(std::size_t states_values_cap);
    void add_state_values(std::unique_ptr<StateValuesExpression>&& state_values);

    /* Setter. */
    inline void set_state_values(std::unique_ptr<StateValuesExpression>&& state_values, std::size_t index);

    /* Getter. */

    [[nodiscard]] inline std::size_t get_size_states_values() const { return statesValues.size(); }

    [[nodiscard]] inline const StateValuesExpression* get_state_values(std::size_t index) const {
        PLAJA_ASSERT(index < statesValues.size())
        return statesValues[index].get();
    }

    [[nodiscard]] inline StateValuesExpression* get_state_values(std::size_t index) {
        PLAJA_ASSERT(index < statesValues.size())
        return statesValues[index].get();
    }

    // Iterators. */

    [[nodiscard]] inline AstConstIterator<StateValuesExpression> init_state_values_iterator() const { return AstConstIterator(statesValues); }

    [[nodiscard]] inline AstIterator<StateValuesExpression> init_state_values_iterator() { return AstIterator(statesValues); }

    // Auxiliary. */
    [[nodiscard]] std::unordered_set<VariableIndex_type> retrieve_state_indexes() const;

    // Override. */
    PLAJA::integer evaluateInteger(const StateBase* state) const override;
    [[nodiscard]] bool is_constant() const override;
    bool equals(const Expression* exp) const override;
    [[nodiscard]] std::size_t hash() const override;
    [[nodiscard]] std::unique_ptr<Expression> deepCopy_Exp() const override;
    void accept(AstVisitor* ast_visitor) override;
    void accept(AstVisitorConst* ast_visitor) const override;

};

#endif //PLAJA_STATES_VALUES_EXPRESSION_H
