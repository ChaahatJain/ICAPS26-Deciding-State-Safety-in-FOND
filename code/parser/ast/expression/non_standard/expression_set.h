//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_EXPRESSION_SET_H
#define PLAJA_EXPRESSION_SET_H

#include <unordered_set>
#include "../expression.h"

namespace EXPRESSION_SET {

    struct ExpressionHash {
        std::size_t operator()(const Expression* exp) const {
            PLAJA_ASSERT(exp)
            return exp->hash();
        }
    };

    struct ExpressionEqual {
        bool operator()(const Expression* exp1, const Expression* exp2) const {
            PLAJA_ASSERT(exp1)
            return exp1->equals(exp2);
        }
    };

}

typedef std::unordered_set<const Expression*,
        EXPRESSION_SET::ExpressionHash,
        EXPRESSION_SET::ExpressionEqual> ExpressionSet;

#endif //PLAJA_EXPRESSION_SET_H
