//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <unordered_map>
#include "qfied_expression.h"
#include "../../visitor/ast_visitor.h"
#include "../../visitor/ast_visitor_const.h"
#include "../type/bool_type.h"
#include "../type/real_type.h"

QfiedExpression::QfiedExpression(Qfier qfier_op):
    op(qfier_op)
    , pathFormula() {
}

QfiedExpression::~QfiedExpression() = default;

// static:

namespace QFIED_EXPRESSION {
    const std::string qfierToStr[] { "∀", "∃", "Pmin", "Pmax", "Smin", "Smax" }; // NOLINT(cert-err58-cpp)
    const std::unordered_map<std::string, QfiedExpression::Qfier> strToQfier { { "∀",    QfiedExpression::FORALL },
                                                                               { "∃",    QfiedExpression::EXISTS },
                                                                               { "Pmin", QfiedExpression::PMIN },
                                                                               { "Pmax", QfiedExpression::PMAX },
                                                                               { "Smin", QfiedExpression::SMIN },
                                                                               { "Smax", QfiedExpression::SMAX } }; // NOLINT(cert-err58-cpp)
}

const std::string& QfiedExpression::qfier_to_str(QfiedExpression::Qfier qfier) { return QFIED_EXPRESSION::qfierToStr[qfier]; }

std::unique_ptr<QfiedExpression::Qfier> QfiedExpression::str_to_qfier(const std::string& qfier_str) {
    auto it = QFIED_EXPRESSION::strToQfier.find(qfier_str);
    if (it == QFIED_EXPRESSION::strToQfier.end()) { return nullptr; }
    else { return std::make_unique<Qfier>(it->second); }
}

// override:

const DeclarationType* QfiedExpression::determine_type() {
    if (resultType) { return resultType.get(); }
    else {
        if (op == FORALL or op == EXISTS) { return (resultType = std::make_unique<BoolType>()).get(); }
        else { return (resultType = std::make_unique<RealType>()).get(); }
    }
}

void QfiedExpression::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void QfiedExpression::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }

std::unique_ptr<PropertyExpression> QfiedExpression::deepCopy_PropExp() const {
    std::unique_ptr<QfiedExpression> copy(new QfiedExpression(op));
    if (resultType) { copy->resultType = resultType->deep_copy_decl_type(); }
    if (pathFormula) { copy->set_path_formula(pathFormula->deepCopy_PropExp()); }
    return copy;
}
