//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_FORWARD_EXPRESSION_H
#define PLAJA_FORWARD_EXPRESSION_H

class Expression;

class PropertyExpression;

class ArrayAccessExpression;

class ArrayConstructorExpression;

class ArrayValueExpression;

class BinaryOpExpression;

class BoolValueExpression;

class ConstantExpression;

class ConstantValueExpression;

class DerivativeExpression;

class DistributionSamplingExpression;

class ExpectationExpression;

class FilterExpression;

class IntegerValueExpression;

class ITE_Expression;

class LValueExpression;

class PathExpression;

class QfiedExpression;

class RealValueExpression;

class StatePredicateExpression;

class UnaryOpExpression;

class VariableExpression;

/* Non-standard. */

class ActionOpTuple;

class ConstantArrayAccessExpression;

class CommentExpression;

class FreeVariableExpression;

class LetExpression;

class LocationValueExpression;

class ObjectiveExpression;

class PAExpression;

class PredicatesExpression;

class ProblemInstanceExpression;

class StateConditionExpression;

class StateValuesExpression;

class StatesValuesExpression;

class VariableValueExpression;

/* Special case structures. */

class LinearExpression;

class NaryExpression;

#endif //PLAJA_FORWARD_EXPRESSION_H
