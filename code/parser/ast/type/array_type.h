//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_ARRAY_TYPE_H
#define PLAJA_ARRAY_TYPE_H

#include <memory>
#include "declaration_type.h"

class ArrayType final: public DeclarationType {

private:
    std::unique_ptr<DeclarationType> elementType;

public:
    ArrayType();
    ~ArrayType() override;
    DELETE_CONSTRUCTOR(ArrayType)

    /* Setter. */
    inline void set_element_type(std::unique_ptr<DeclarationType>&& element_type) { elementType = std::move(element_type); }

    /* Getter. */

    [[nodiscard]] inline DeclarationType* get_element_type() { return elementType.get(); }

    [[nodiscard]] inline const DeclarationType* get_element_type() const { return elementType.get(); }

    /* Override. */

    [[nodiscard]] bool is_array_type() const override;
    [[nodiscard]] bool has_boolean_base() const override;
    [[nodiscard]] bool has_integer_base() const override;
    [[nodiscard]] bool has_floating_base() const override;

    [[nodiscard]] inline bool is_boolean_array() const { return elementType->is_boolean_type(); }
    [[nodiscard]] inline bool is_integer_array() const { return elementType->is_integer_type(); }
    [[nodiscard]] inline bool is_floating_array() const { return elementType->is_floating_type(); }

    [[nodiscard]] TypeKind get_kind() const override;
    [[nodiscard]] bool is_assignable_from(const DeclarationType& assigned_type) const override;
    [[nodiscard]] std::unique_ptr<DeclarationType> infer_common(const DeclarationType& ref_type) const override;

    // bool operator==(const DeclarationType& ref_type) const override; // deprecated

    [[nodiscard]] std::unique_ptr<DeclarationType> deep_copy_decl_type() const override;

    void accept(AstVisitor* ast_visitor) override;
    void accept(AstVisitorConst* ast_visitor) const override;

};

#endif //PLAJA_ARRAY_TYPE_H
