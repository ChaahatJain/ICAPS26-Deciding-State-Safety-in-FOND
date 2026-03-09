//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_QFIED_EXPRESSION_H
#define PLAJA_QFIED_EXPRESSION_H

#include <memory>
#include "property_expression.h"

/**
 * Quantified/Qualified property expression.
 */
class QfiedExpression final: public PropertyExpression {
public:
    enum Qfier { FORALL, EXISTS, PMIN, PMAX, SMIN, SMAX };
private:
    Qfier op;
    std::unique_ptr<PropertyExpression> pathFormula;
public:
    explicit QfiedExpression(Qfier qfier_op);
    ~QfiedExpression() override;

    // static:
    static const std::string& qfier_to_str(Qfier qfier);
    static std::unique_ptr<Qfier> str_to_qfier(const std::string& qfier_str);

    // setter:
    [[maybe_unused]] inline void set_op(Qfier qfier_op) {op = qfier_op;}
    inline void set_path_formula(std::unique_ptr<PropertyExpression>&& path_formula) { pathFormula = std::move(path_formula); }

    // getter:
    [[nodiscard]] inline Qfier get_op() const { return op; }
    [[nodiscard]] inline const PropertyExpression* get_path_formula() const { return pathFormula.get(); }
    [[nodiscard]] inline PropertyExpression* get_path_formula() { return pathFormula.get(); }

    // override:
    const DeclarationType* determine_type() override;
    void accept(AstVisitor* astVisitor) override;
	void accept(AstVisitorConst* astVisitor) const override;
    [[nodiscard]] std::unique_ptr<PropertyExpression> deepCopy_PropExp() const override;
};


#endif //PLAJA_QFIED_EXPRESSION_H
