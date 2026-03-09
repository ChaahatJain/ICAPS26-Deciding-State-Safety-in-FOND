//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "special_case_expression.h"
#include "../../type/declaration_type.h"

SpecialCaseExpression::SpecialCaseExpression() = default;

SpecialCaseExpression::~SpecialCaseExpression() = default;

/* */

void SpecialCaseExpression::deep_copy(SpecialCaseExpression& target) const {
    if (resultType) { target.resultType = resultType->deep_copy_decl_type(); }
}

void SpecialCaseExpression::move(SpecialCaseExpression& target) {
    target.resultType = std::move(resultType);
}
