//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PREDICATES_EXPRESSION_H
#define PLAJA_PREDICATES_EXPRESSION_H

#include "../../iterators/ast_iterator.h"
#include "../expression.h"

/** Auxiliary class for predicates array.*/
class PredicatesExpression final: public Expression {

private:
    std::vector<std::unique_ptr<Expression>> predicates; // for cegar this is the initial set

public:
    PredicatesExpression();
    ~PredicatesExpression() override;
    DELETE_CONSTRUCTOR(PredicatesExpression)

    /* construction */

    void reserve(std::size_t predicates_cap);

    void add_predicate(std::unique_ptr<Expression>&& predicate);

    /* setter */

    inline void set_predicates(std::unique_ptr<Expression>&& predicate, std::size_t index) {
        PLAJA_ASSERT(index < predicates.size())
        predicates[index] = std::move(predicate);
    }

    /* getter */

    [[nodiscard]] inline std::size_t get_number_predicates() const { return predicates.size(); }

    [[nodiscard]] inline Expression* get_predicate(std::size_t index) {
        PLAJA_ASSERT(index < predicates.size())
        return predicates[index].get();
    }

    [[nodiscard]] inline const Expression* get_predicate(std::size_t index) const {
        PLAJA_ASSERT(index < predicates.size())
        return predicates[index].get();
    }

    [[nodiscard]] inline AstConstIterator<Expression> predicatesIterator() const { return AstConstIterator(predicates); }

    [[nodiscard]] inline AstIterator<Expression> predicatesIterator() { return AstIterator(predicates); }

    /* override */

    bool equals(const Expression* exp) const override;
    void accept(AstVisitor* ast_visitor) override;
    void accept(AstVisitorConst* ast_visitor) const override;
    [[nodiscard]] std::unique_ptr<Expression> deepCopy_Exp() const override;
    [[nodiscard]] std::unique_ptr<PredicatesExpression> deepCopy() const;

    /**
     * Is the predicate (at the respective index) a bound on a variable value.
     * Currently: x {<=,<,==,>,>=} c, b or !b.
     * @param index
     * @return
     */
    [[nodiscard]] static bool predicate_is_bound(const Expression& predicate);

    [[nodiscard]] inline bool predicate_is_bound(std::size_t index) const { return PredicatesExpression::predicate_is_bound(*get_predicate(index)); }

};

#endif //PLAJA_PREDICATES_EXPRESSION_H
