//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_VARIABLE_VALUE_LIST_H
#define PLAJA_VARIABLE_VALUE_LIST_H

#include <unordered_set>
#include "../../iterators/ast_iterator.h"
#include "../expression.h"
#include "../forward_expression.h"

class StateValues;

class StateValuesExpression: public Expression {
private:
    std::vector<std::unique_ptr<LocationValueExpression>> locValues;
    std::vector<std::unique_ptr<VariableValueExpression>> varValues;

public:
    StateValuesExpression();
    ~StateValuesExpression() override;
    DELETE_CONSTRUCTOR(StateValuesExpression)

    [[nodiscard]] std::unique_ptr<StateValuesExpression> deepCopy() const;

    /* Construction. */

    void reserve(std::size_t loc_values_cap, std::size_t var_values_cap);
    void add_loc_value(std::unique_ptr<LocationValueExpression>&& value);
    void add_var_value(std::unique_ptr<VariableValueExpression>&& value);

    /* Setter. */

    void set_loc_value(std::unique_ptr<LocationValueExpression>&& value, std::size_t index);
    void set_var_value(std::unique_ptr<VariableValueExpression>&& value, std::size_t index);

    /* Getter. */

    [[nodiscard]] inline std::size_t get_size_loc_values() const { return locValues.size(); }

    [[nodiscard]] inline const LocationValueExpression* get_loc_value(std::size_t index) const {
        PLAJA_ASSERT(index < locValues.size())
        return locValues[index].get();
    }

    inline LocationValueExpression* get_loc_value(std::size_t index) {
        PLAJA_ASSERT(index < locValues.size())
        return locValues[index].get();
    }

    [[nodiscard]] inline std::size_t get_size_var_values() const { return varValues.size(); }

    [[nodiscard]] inline const VariableValueExpression* get_var_value(std::size_t index) const {
        PLAJA_ASSERT(index < varValues.size())
        return varValues[index].get();
    }

    inline VariableValueExpression* get_var_value(std::size_t index) {
        PLAJA_ASSERT(index < varValues.size())
        return varValues[index].get();
    }

    /* Iterators. */

    [[nodiscard]] inline AstConstIterator<LocationValueExpression> init_loc_value_iterator() const { return AstConstIterator(locValues); }

    [[nodiscard]] inline AstIterator<LocationValueExpression> init_loc_value_iterator() { return AstIterator(locValues); }

    [[nodiscard]] inline AstConstIterator<VariableValueExpression> init_var_value_iterator() const { return AstConstIterator(varValues); }

    [[nodiscard]] inline AstIterator<VariableValueExpression> init_var_value_iterator() { return AstIterator(varValues); }

    // Auxiliary. */
    [[nodiscard]] std::unordered_set<VariableIndex_type> retrieve_state_indexes() const;
    [[nodiscard]] StateValues construct_state_values(const StateValues& constructor) const;

    /* Override. */
    PLAJA::integer evaluateInteger(const StateBase* state) const override;
    [[nodiscard]] bool is_constant() const override;
    bool equals(const Expression* exp) const override;
    [[nodiscard]] std::size_t hash() const override;
    [[nodiscard]] std::unique_ptr<Expression> deepCopy_Exp() const override;
    void accept(AstVisitor* ast_visitor) override;
    void accept(AstVisitorConst* ast_visitor) const override;

};

#endif //PLAJA_VARIABLE_VALUE_LIST_H
