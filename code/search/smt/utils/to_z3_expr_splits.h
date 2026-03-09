//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_TO_Z3_EXPR_SPLITS_H
#define PLAJA_TO_Z3_EXPR_SPLITS_H

#include <list>
#include <z3++.h>
#include "../../../parser/ast/expression/forward_expression.h"
#include "../../../assertions.h"
#include "../forward_smt_z3.h"

struct ToZ3ExprSplits {
    friend class ToZ3Visitor; //
    friend class ActionOpToZ3;

private:
    Z3_IN_PLAJA::Context* context;
    std::list<std::unique_ptr<Expression>> splits;
    z3::expr_vector splitsInZ3;

public:
    explicit ToZ3ExprSplits(Z3_IN_PLAJA::Context& c);
    ToZ3ExprSplits(std::list<std::unique_ptr<Expression>>&& splits_, Z3_IN_PLAJA::Context& c);
    ~ToZ3ExprSplits();
    DELETE_CONSTRUCTOR(ToZ3ExprSplits)

    [[nodiscard]] const z3::expr_vector& _splits_in_z3() const { return splitsInZ3; }

    class Iterator {
    private:
        std::list<std::unique_ptr<Expression>>::iterator it;
        const std::list<std::unique_ptr<Expression>>::iterator itEnd;
        z3::expr_vector::iterator itZ3;

    public:
        explicit Iterator(ToZ3ExprSplits& splits):
            it(splits.splits.begin())
            , itEnd(splits.splits.end())
            , itZ3(splits.splitsInZ3.begin()) {
            PLAJA_ASSERT(splits.splits.size() == splits.splitsInZ3.size())
        }

        inline void operator++() {
            ++it;
            ++itZ3;
        }

        [[nodiscard]] inline bool end() const { return it == itEnd; }

        [[nodiscard]] inline const Expression& split() const { return **it; }

        [[nodiscard]] inline z3::expr split_z3() { return *itZ3; }

        [[nodiscard]] inline std::unique_ptr<Expression> move_split() { return std::move(*it); }
    };

    [[nodiscard]] Iterator iterator() { return Iterator(*this); }

};

#endif //PLAJA_TO_Z3_EXPR_SPLITS_H
