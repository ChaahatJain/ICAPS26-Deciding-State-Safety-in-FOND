//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "ast_element_visitor.h"
#include "../ast/ast_element.h"

AstElementVisitor::AstElementVisitor() = default;

AstElementVisitor::~AstElementVisitor() = default;

/* */

void AstElementVisitor::visit(Action*) {}

void AstElementVisitor::visit(Assignment*) {}

void AstElementVisitor::visit(Automaton*) {}

void AstElementVisitor::visit(Composition*) {}

void AstElementVisitor::visit(CompositionElement*) {}

void AstElementVisitor::visit(ConstantDeclaration*) {}

void AstElementVisitor::visit(Destination*) {}

void AstElementVisitor::visit(Edge*) {}

void AstElementVisitor::visit(Location*) {}

void AstElementVisitor::visit(Metadata*) {}

void AstElementVisitor::visit(Model*) {}

void AstElementVisitor::visit(Property*) {}

void AstElementVisitor::visit(PropertyInterval*) {}

void AstElementVisitor::visit(RewardBound*) {}

void AstElementVisitor::visit(RewardInstant*) {}

void AstElementVisitor::visit(Synchronisation*) {}

void AstElementVisitor::visit(TransientValue*) {}

void AstElementVisitor::visit(VariableDeclaration*) {}

/* Non-standard. */

void AstElementVisitor::visit(FreeVariableDeclaration*) {}

void AstElementVisitor::visit(Properties*) {}

void AstElementVisitor::visit(RewardAccumulation*) {}

/* Expressions. */

void AstElementVisitor::visit(ArrayAccessExpression*) {}

void AstElementVisitor::visit(ArrayConstructorExpression*) {}

void AstElementVisitor::visit(ArrayValueExpression*) {}

void AstElementVisitor::visit(BinaryOpExpression*) {}

void AstElementVisitor::visit(BoolValueExpression*) {}

void AstElementVisitor::visit(ConstantExpression*) {}

void AstElementVisitor::visit(DerivativeExpression*) {}

void AstElementVisitor::visit(DistributionSamplingExpression*) {}

void AstElementVisitor::visit(ExpectationExpression*) {}

void AstElementVisitor::visit(FilterExpression*) {}

void AstElementVisitor::visit(FreeVariableExpression*) {}

void AstElementVisitor::visit(IntegerValueExpression*) {}

void AstElementVisitor::visit(ITE_Expression*) {}

void AstElementVisitor::visit(PathExpression*) {}

void AstElementVisitor::visit(QfiedExpression*) {}

void AstElementVisitor::visit(RealValueExpression*) {}

void AstElementVisitor::visit(StatePredicateExpression*) {}

void AstElementVisitor::visit(UnaryOpExpression*) {}

void AstElementVisitor::visit(VariableExpression*) {}

/* Non-standard. */

void AstElementVisitor::visit(ActionOpTuple*) {}

void AstElementVisitor::visit(CommentExpression*) {}

void AstElementVisitor::visit(LetExpression*) {}

void AstElementVisitor::visit(LocationValueExpression*) {}

void AstElementVisitor::visit(ObjectiveExpression*) {}

void AstElementVisitor::visit(PAExpression*) {}

void AstElementVisitor::visit(PredicatesExpression*) {}

void AstElementVisitor::visit(ProblemInstanceExpression*) {}

void AstElementVisitor::visit(StateConditionExpression*) {}

void AstElementVisitor::visit(StateValuesExpression*) {}

void AstElementVisitor::visit(StatesValuesExpression*) {}

void AstElementVisitor::visit(VariableValueExpression*) {}

/* Special cases. */

void AstElementVisitor::visit(LinearExpression*) {}

void AstElementVisitor::visit(NaryExpression*) {}

/* Types. */

void AstElementVisitor::visit(ArrayType*) {}

void AstElementVisitor::visit(BoolType*) {}

void AstElementVisitor::visit(BoundedType*) {}

void AstElementVisitor::visit(ClockType*) {}

void AstElementVisitor::visit(ContinuousType*) {}

void AstElementVisitor::visit(IntType*) {}

void AstElementVisitor::visit(RealType*) {}

/* Non-standard. */

void AstElementVisitor::visit(LocationType*) {}