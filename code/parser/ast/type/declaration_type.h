//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_DECLARATION_TYPE_H
#define PLAJA_DECLARATION_TYPE_H

#include <memory>
#include "../ast_element.h"

class DeclarationType: public AstElement {

public:
    enum TypeKind { Bool, Int, Real, Clock, Continuous, Array, Bounded, Location };

    DeclarationType();
    ~DeclarationType() override = 0;
    DELETE_CONSTRUCTOR(DeclarationType)

    // static:
    static const std::string& kind_to_str(TypeKind kind);
    static std::unique_ptr<TypeKind> str_to_kind(const std::string& kind_str);

    // integer types: BOOL
    [[nodiscard]] virtual bool is_boolean_type() const;

    // integer types: INT, BOUNDED (if base is)
    [[nodiscard]] virtual bool is_integer_type() const;

    // floating types: REAL, CLOCK, CONTINUOUS, BOUNDED (if base is)
    [[nodiscard]] virtual bool is_floating_type() const;

    // numeric types: INT, REAL, CLOCK, CONTINUOUS, BOUNDED (if base is)
    [[nodiscard]] virtual bool is_numeric_type() const;

    // basic types: BOOL, INT, REAL
    [[nodiscard]] virtual bool is_basic_type() const;

    // trivial types: BOOL, INT, REAL, CLOCK, CONTINUOUS, BOUNDED (if base is)
    [[nodiscard]] virtual bool is_trivial_type() const;

    // array types: ARRAY
    [[nodiscard]] virtual bool is_array_type() const;

    [[nodiscard]] inline bool is_scalar_type() const { return not is_array_type(); }

    // bounded types: Bound
    [[nodiscard]] virtual bool is_bounded_type() const;

    // Is boolean/integer/floating or array thereof?
    [[nodiscard]] virtual bool has_boolean_base() const;
    [[nodiscard]] virtual bool has_integer_base() const;
    [[nodiscard]] virtual bool has_floating_base() const;

    [[nodiscard]] virtual TypeKind get_kind() const = 0;

    [[nodiscard]] inline const std::string& get_type_string() const { return kind_to_str(get_kind()); }

    /** True iff a variable of *this* type can be assigned from *assigned_type* (cf. PRISM). */
    virtual bool is_assignable_from(const DeclarationType& assigned_type) const;

    /**
     * Infer smallest common type *this* type and ref_type can be both assigned to.
     * If A,B and C can be assigned to a common type, then the type inferred for A and B can too.
     * @return common type, *nullptr* if none can be derived.
     */
    virtual std::unique_ptr<DeclarationType> infer_common(const DeclarationType& ref_type) const;

    // virtual bool operator==(const DeclarationType& refType) const = 0; // deprecated

    /** Deep copy of a declaration type. */
    [[nodiscard]] virtual std::unique_ptr<DeclarationType> deep_copy_decl_type() const;

};

#endif //PLAJA_DECLARATION_TYPE_H
