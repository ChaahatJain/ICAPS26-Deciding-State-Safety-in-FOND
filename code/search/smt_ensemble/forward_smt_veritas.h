//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#ifndef PLAJA_FORWARD_SMT_ENSEMBLE_H
#define PLAJA_FORWARD_SMT_ENSEMBLE_H

class ModelVeritas;

namespace VERITAS_IN_PLAJA { class QueryConstructable; }

namespace VERITAS_IN_PLAJA { class VeritasQuery; }

namespace VERITAS_IN_PLAJA { class Solver; class SolverVeritas; class SolverGurobi; }

namespace VERITAS_IN_PLAJA { class SolutionCheckWrapper; }

namespace VERITAS_IN_PLAJA { struct Solution; }

namespace VERITAS_IN_PLAJA { class Equation; class Tightening; }

struct UpdateVeritas;

class ActionOpVeritas;

struct UpdateToVeritas;

class ActionOpToVeritas;

class VeritasConstraint;

namespace VERITAS_IN_PLAJA { class PredicateConstraint; class EquationConstraint; class BoundConstraint; } 

class DisjunctionInVeritas;

class ConjunctionInVeritas;

namespace VERITAS_IN_PLAJA { class SuccessorGenerator; }

namespace VERITAS_IN_PLAJA { class Context; }



class EnsembleInVeritas;

namespace VERITAS_IN_PLAJA { class OutputConstraints; }

namespace VERITAS_IN_PLAJA { class OutputConstraintsNoFilter; }

class StateIndexesInVeritas;

class OpApplicability; // For convenience.

#endif //PLAJA_FORWARD_SMT_ENSEMBLE_H
