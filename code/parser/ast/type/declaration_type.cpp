//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <unordered_map>
#include "declaration_type.h"
#include "../../../exception/not_implemented_exception.h"

// extern:

DeclarationType::DeclarationType() = default;

DeclarationType::~DeclarationType() = default;

// static:

const std::string& DeclarationType::kind_to_str(DeclarationType::TypeKind kind) {

    static const std::unordered_map<DeclarationType::TypeKind, std::string> type_kind_to_str { { DeclarationType::Bool,       "bool" },
                                                                                               { DeclarationType::Int,        "int" },
                                                                                               { DeclarationType::Real,       "real" },
                                                                                               { DeclarationType::Continuous, "continuous" },
                                                                                               { DeclarationType::Array,      "array" },
                                                                                               { DeclarationType::Bounded,    "bounded" },
                                                                                               { DeclarationType::Location,   "location" } };

    PLAJA_ASSERT(type_kind_to_str.count(kind))
    return type_kind_to_str.at(kind);
}

std::unique_ptr<DeclarationType::TypeKind> DeclarationType::str_to_kind(const std::string& kind_str) {

    static const std::unordered_map<std::string, DeclarationType::TypeKind> str_to_type_kind { { "bool",       DeclarationType::Bool },
                                                                                               { "int",        DeclarationType::Int },
                                                                                               { "real",       DeclarationType::Real },
                                                                                               { "continuous", DeclarationType::Continuous },
                                                                                               { "array",      DeclarationType::Array },
                                                                                               { "bounded",    DeclarationType::Bounded },
                                                                                               { "location",   DeclarationType::Location } };

    auto it = str_to_type_kind.find(kind_str);
    return it == str_to_type_kind.end() ? nullptr : std::make_unique<TypeKind>(it->second);
}

bool DeclarationType::is_boolean_type() const { return false; }

bool DeclarationType::is_integer_type() const { return false; }

bool DeclarationType::is_floating_type() const { return false; }

bool DeclarationType::is_numeric_type() const { return false; }

bool DeclarationType::is_basic_type() const { return false; }

bool DeclarationType::is_trivial_type() const { return false; }

bool DeclarationType::is_array_type() const { return false; }

bool DeclarationType::is_bounded_type() const { return false; }

bool DeclarationType::has_boolean_base() const { return is_boolean_type(); }

bool DeclarationType::has_integer_base() const { return is_integer_type(); }

bool DeclarationType::has_floating_base() const { return is_floating_type(); }

bool DeclarationType::is_assignable_from(const DeclarationType& /*assigned_type*/) const { throw NotImplementedException(__PRETTY_FUNCTION__); }

std::unique_ptr<DeclarationType> DeclarationType::infer_common(const DeclarationType& /*ref_type*/) const { throw NotImplementedException(__PRETTY_FUNCTION__); }

std::unique_ptr<DeclarationType> DeclarationType::deep_copy_decl_type() const { throw NotImplementedException(__PRETTY_FUNCTION__); }
