//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "parser.h"
#include "ast/expression/expression.h"
#include "ast/type/array_type.h"
#include "ast/type/bool_type.h"
#include "ast/type/bounded_type.h"
#include "ast/type/clock_type.h"
#include "ast/type/continuous_type.h"
#include "ast/type/int_type.h"
#include "ast/type/real_type.h"
#include "ast/model.h"
#include "visitor/restrictions_checker.h"
#include "parser_utils.h"

std::unique_ptr<DeclarationType> Parser::parse_DeclarationType(const nlohmann::json& input) { // NOLINT(misc-no-recursion)

    if (input.is_string() and DeclarationType::str_to_kind(input)) { // Trivial type.

        switch (*DeclarationType::str_to_kind(input)) {
            case DeclarationType::Bool: { return std::make_unique<BoolType>(); }
            case DeclarationType::Int: { return std::make_unique<IntType>(); }
            case DeclarationType::Real: { return std::make_unique<RealType>(); }
            case DeclarationType::Clock: {
                const auto model_type = model_ref->get_model_type();
                PARSER::throw_parser_exception_if(not(model_type == Model::TA or model_type == Model::PTA or model_type == Model::STA or model_type == Model::HA or model_type == Model::PHA or model_type == Model::SHA), input, PARSER::not_supported_by_model_type);
                return std::make_unique<ClockType>();
            }
            case DeclarationType::Continuous: {
                const auto model_type = model_ref->get_model_type();
                PARSER::throw_parser_exception_if(not(model_type == Model::HA or model_type == Model::PHA or model_type == Model::SHA), input, PARSER::not_supported_by_model_type);
                return std::make_unique<ContinuousType>();
            }
            default: { PARSER::throw_parser_exception(input, PARSER::unmatched_jani_structure); }
        }

    }

    if (input.is_object() and input.contains(JANI::KIND)) { // Non-trivial type.
        const auto& kind_j = input.at(JANI::KIND);
        PARSER::check_string(kind_j, input);
        if (JANI::ARRAY == kind_j) { return parse_ArrayType(input); }
        if (JANI::BOUNDED == kind_j) { return parse_BoundedType(input); }
    }

    PARSER::throw_parser_exception(input, PARSER::unmatched_jani_structure);
}

std::unique_ptr<BasicType> Parser::parse_BasicType(const nlohmann::json& input) {
    PARSER::check_string(input);
    if (DeclarationType::kind_to_str(DeclarationType::Bool) == input) { return std::make_unique<BoolType>(); }
    if (DeclarationType::kind_to_str(DeclarationType::Int) == input) { return std::make_unique<IntType>(); }
    if (DeclarationType::kind_to_str(DeclarationType::Real) == input) { return std::make_unique<RealType>(); }
    PARSER::throw_parser_exception(input, PARSER::unmatched_jani_structure);
}

std::unique_ptr<ArrayType> Parser::parse_ArrayType(const nlohmann::json& input) { // NOLINT(misc-no-recursion)
    // kind:
    PARSER::check_object_decoupled(input);
    PARSER::check_kind_decoupled(input, JANI::ARRAY);

    std::unique_ptr<ArrayType> arrayType(new ArrayType());
    // base:
    PARSER::check_contains(input, JANI::BASE);
    arrayType->set_element_type(parse_DeclarationType(input.at(JANI::BASE)));

    PARSER::check_for_unexpected_element(input, 2);
    return arrayType;
}

std::unique_ptr<BoundedType> Parser::parse_BoundedType(const nlohmann::json& input) {
    // kind:
    /* To check in non-decoupled case too, due to parse_ConstantDeclaration call. */
    PARSER::check_object(input);
    PARSER::check_kind(input, JANI::BOUNDED);

    std::unique_ptr<BoundedType> boundedType(new BoundedType());

    // base:
    PARSER::check_contains(input, JANI::BASE);
    boundedType->set_base(parse_BasicType(input.at(JANI::BASE)));

    unsigned int object_size = 2;

    // lower-bound
    if (input.contains(JANI::LOWER_BOUND)) {
        ++object_size;
        boundedType->set_lower_bound(parse_Expression(input.at(JANI::LOWER_BOUND)));
    }

    // upper-bound
    if (input.contains(JANI::UPPER_BOUND)) {
        ++object_size;
        boundedType->set_upper_bound(parse_Expression(input.at(JANI::UPPER_BOUND)));
    }

    PARSER::throw_parser_exception_if(object_size < 3, input, JANI::BOUNDS);
    PARSER::check_for_unexpected_element(input, object_size);
    return boundedType;
}

#pragma GCC diagnostic push
#pragma ide diagnostic ignored "UnreachableCode"
#pragma ide diagnostic ignored "UnusedParameter"

[[maybe_unused]] std::unique_ptr<BoolType> Parser::parse_BoolType(const nlohmann::json& input) {
    PLAJA_ABORT_IF_CONST(not PARSER::decoupledParsing)
    PARSER::check_string(input);
    PARSER::throw_parser_exception_if(input != DeclarationType::kind_to_str(DeclarationType::Bool), input, DeclarationType::kind_to_str(DeclarationType::Bool));
    return std::make_unique<BoolType>();
}

[[maybe_unused]] std::unique_ptr<ClockType> Parser::parse_ClockType(const nlohmann::json& input) {
    PLAJA_ABORT_IF_CONST(not PARSER::decoupledParsing)
    PARSER::check_string(input);
    PARSER::throw_parser_exception_if(input != DeclarationType::kind_to_str(DeclarationType::Clock), input, DeclarationType::kind_to_str(DeclarationType::Clock));
    const auto model_type = model_ref->get_model_type();
    PARSER::throw_parser_exception_if(not(model_type == Model::TA or model_type == Model::PTA or model_type == Model::STA or model_type == Model::HA or model_type == Model::PHA or model_type == Model::SHA), input, PARSER::not_supported_by_model_type);
    return std::make_unique<ClockType>();
}

[[maybe_unused]] std::unique_ptr<ContinuousType> Parser::parse_ContinuousType(const nlohmann::json& input) {
    PLAJA_ABORT_IF_CONST(not PARSER::decoupledParsing)
    PARSER::check_string(input);
    PARSER::throw_parser_exception_if(input != DeclarationType::kind_to_str(DeclarationType::Continuous), input, DeclarationType::kind_to_str(DeclarationType::Continuous));
    const auto model_type = model_ref->get_model_type();
    PARSER::throw_parser_exception_if(not(model_type == Model::HA or model_type == Model::PHA or model_type == Model::SHA), input, PARSER::not_supported_by_model_type);
    return std::make_unique<ContinuousType>();
}

[[maybe_unused]] std::unique_ptr<IntType> Parser::parse_IntType(const nlohmann::json& input) {
    PLAJA_ABORT_IF_CONST(not PARSER::decoupledParsing)
    PARSER::check_string(input);
    PARSER::throw_parser_exception_if(input != DeclarationType::kind_to_str(DeclarationType::Int), input, DeclarationType::kind_to_str(DeclarationType::Int));
    return std::make_unique<IntType>();
}

[[maybe_unused]] std::unique_ptr<RealType> Parser::parse_RealType(const nlohmann::json& input) {
    PLAJA_ABORT_IF_CONST(not PARSER::decoupledParsing)
    PARSER::check_string(input);
    PARSER::throw_parser_exception_if(input != DeclarationType::kind_to_str(DeclarationType::Real), input, DeclarationType::kind_to_str(DeclarationType::Real));
    return std::make_unique<RealType>();
}

#pragma GCC diagnostic pop