//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "ast_element_visitor_const.h"

AstElementVisitorConst::AstElementVisitorConst() = default;

AstElementVisitorConst::~AstElementVisitorConst() = default;

/* */

void AstElementVisitorConst::visit(const Action*) {}

void AstElementVisitorConst::visit(const Assignment*) {}

void AstElementVisitorConst::visit(const Automaton*) {}

void AstElementVisitorConst::visit(const Composition*) {}

void AstElementVisitorConst::visit(const CompositionElement*) {}

void AstElementVisitorConst::visit(const ConstantDeclaration*) {}

void AstElementVisitorConst::visit(const Destination*) {}

void AstElementVisitorConst::visit(const Edge*) {}

void AstElementVisitorConst::visit(const Location*) {}

void AstElementVisitorConst::visit(const Metadata*) {}

void AstElementVisitorConst::visit(const Model*) {}

void AstElementVisitorConst::visit(const Property*) {}

void AstElementVisitorConst::visit(const PropertyInterval*) {}

void AstElementVisitorConst::visit(const RewardBound*) {}

void AstElementVisitorConst::visit(const RewardInstant*) {}

void AstElementVisitorConst::visit(const Synchronisation*) {}

void AstElementVisitorConst::visit(const TransientValue*) {}

void AstElementVisitorConst::visit(const VariableDeclaration*) {}

/* Non-standard. */

void AstElementVisitorConst::visit(const FreeVariableDeclaration*) {}

void AstElementVisitorConst::visit(const Properties*) {}

void AstElementVisitorConst::visit(const RewardAccumulation*) {}

/* Expressions. */

void AstElementVisitorConst::visit(const ArrayAccessExpression*) {}

void AstElementVisitorConst::visit(const ArrayConstructorExpression*) {}

void AstElementVisitorConst::visit(const ArrayValueExpression*) {}

void AstElementVisitorConst::visit(const BinaryOpExpression*) {}

void AstElementVisitorConst::visit(const BoolValueExpression*) {}

void AstElementVisitorConst::visit(const ConstantExpression*) {}

void AstElementVisitorConst::visit(const DerivativeExpression*) {}

void AstElementVisitorConst::visit(const DistributionSamplingExpression*) {}

void AstElementVisitorConst::visit(const ExpectationExpression*) {}

void AstElementVisitorConst::visit(const FilterExpression*) {}

void AstElementVisitorConst::visit(const FreeVariableExpression*) {}

void AstElementVisitorConst::visit(const IntegerValueExpression*) {}

void AstElementVisitorConst::visit(const ITE_Expression*) {}

void AstElementVisitorConst::visit(const PathExpression*) {}

void AstElementVisitorConst::visit(const QfiedExpression*) {}

void AstElementVisitorConst::visit(const RealValueExpression*) {}

void AstElementVisitorConst::visit(const StatePredicateExpression*) {}

void AstElementVisitorConst::visit(const UnaryOpExpression*) {}

void AstElementVisitorConst::visit(const VariableExpression*) {}

/* Non-standard. */

void AstElementVisitorConst::visit(const ActionOpTuple*) {}

void AstElementVisitorConst::visit(const CommentExpression*) {}

void AstElementVisitorConst::visit(const LetExpression*) {}

void AstElementVisitorConst::visit(const LocationValueExpression*) {}

void AstElementVisitorConst::visit(const ObjectiveExpression*) {}

void AstElementVisitorConst::visit(const PAExpression*) {}

void AstElementVisitorConst::visit(const PredicatesExpression*) {}

void AstElementVisitorConst::visit(const ProblemInstanceExpression*) {}

void AstElementVisitorConst::visit(const StateConditionExpression*) {}

void AstElementVisitorConst::visit(const StateValuesExpression*) {}

void AstElementVisitorConst::visit(const StatesValuesExpression*) {}

void AstElementVisitorConst::visit(const VariableValueExpression*) {}

/* Special cases. */

void AstElementVisitorConst::visit(const LinearExpression*) {}

void AstElementVisitorConst::visit(const NaryExpression*) {}

/* Types. */

void AstElementVisitorConst::visit(const ArrayType*) {}

void AstElementVisitorConst::visit(const BoolType*) {}

void AstElementVisitorConst::visit(const BoundedType*) {}

void AstElementVisitorConst::visit(const ClockType*) {}

void AstElementVisitorConst::visit(const ContinuousType*) {}

void AstElementVisitorConst::visit(const IntType*) {}

void AstElementVisitorConst::visit(const RealType*) {}

/* Non-standard. */

void AstElementVisitorConst::visit(const LocationType*) {}
