//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SPECIAL_CASE_EXPRESSION_H
#define PLAJA_SPECIAL_CASE_EXPRESSION_H

#include "../expression.h"

/** Parent for special case classes. */
class SpecialCaseExpression: public Expression {

protected:

    /* Auxiliary. */
    void deep_copy(SpecialCaseExpression& target) const;
    void move(SpecialCaseExpression& target);

public:
    SpecialCaseExpression();
    ~SpecialCaseExpression() override;
    DELETE_CONSTRUCTOR(SpecialCaseExpression)

    [[nodiscard]] virtual std::unique_ptr<Expression> to_standard() const = 0;
};

#endif //PLAJA_SPECIAL_CASE_EXPRESSION_H
