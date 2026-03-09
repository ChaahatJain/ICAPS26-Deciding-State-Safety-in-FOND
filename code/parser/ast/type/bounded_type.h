//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_BOUNDED_TYPE_H
#define PLAJA_BOUNDED_TYPE_H

#include <memory>
#include "../expression/forward_expression.h"
#include "declaration_type.h"
#include "forward_type.h"

class BoundedType: public DeclarationType {

private:
    std::unique_ptr<BasicType> base; // int or real
    // expressions must be constant expressions
    std::unique_ptr<Expression> lowerBound;
    std::unique_ptr<Expression> upperBound;

public:
    BoundedType();
    ~BoundedType() override;
    DELETE_CONSTRUCTOR(BoundedType)

    /* Setter. */

    void set_base(std::unique_ptr<BasicType>&& base_);

    void set_lower_bound(std::unique_ptr<Expression>&& lower_bound);

    void set_upper_bound(std::unique_ptr<Expression>&& upper_bound);

    /* Getter. */

    [[nodiscard]] inline BasicType* get_base() { return base.get(); }

    [[nodiscard]] inline const BasicType* get_base() const { return base.get(); }

    [[nodiscard]] inline Expression* get_lower_bound() { return lowerBound.get(); }

    [[nodiscard]] inline const Expression* get_lower_bound() const { return lowerBound.get(); }

    [[nodiscard]] inline Expression* get_upper_bound() { return upperBound.get(); }

    [[nodiscard]] inline const Expression* get_upper_bound() const { return upperBound.get(); }

    /* Override. */

    [[nodiscard]] bool is_integer_type() const override;
    [[nodiscard]] bool is_floating_type() const override;
    [[nodiscard]] bool is_numeric_type() const override;
    [[nodiscard]] bool is_trivial_type() const override;
    [[nodiscard]] bool is_bounded_type() const override;

    [[nodiscard]] TypeKind get_kind() const override;
    [[nodiscard]] bool is_assignable_from(const DeclarationType& assigned_type) const override;
    [[nodiscard]] std::unique_ptr<DeclarationType> infer_common(const DeclarationType& ref_type) const override;

    // bool operator==(const DeclarationType& ref_type) const override; // deprecated

    [[nodiscard]] std::unique_ptr<DeclarationType> deep_copy_decl_type() const override;

    void accept(AstVisitor* ast_visitor) override;
    void accept(AstVisitorConst* ast_visitor) const override;

};

#endif //PLAJA_BOUNDED_TYPE_H
