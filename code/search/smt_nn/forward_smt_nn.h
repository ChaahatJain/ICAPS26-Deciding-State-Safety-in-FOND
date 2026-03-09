//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_FORWARD_SMT_NN_H
#define PLAJA_FORWARD_SMT_NN_H

class ModelMarabou;

namespace MARABOU_IN_PLAJA { struct NeuronInMarabou; }

namespace MARABOU_IN_PLAJA { class SMTSolver; }

namespace MARABOU_IN_PLAJA { class SmtSolverBB; }

namespace MARABOU_IN_PLAJA { class SmtSolverEnum; }

namespace MARABOU_IN_PLAJA { class SolutionCheckWrapper; }

namespace MARABOU_IN_PLAJA { struct Solution; }

struct UpdateMarabou;

class ActionOpMarabou;

struct UpdateToMarabou;

class ActionOpToMarabou;

namespace MARABOU_IN_PLAJA { struct PreprocessedBounds; }

class MarabouConstraint;

class PredicateConstraint;

class EquationConstraint;

class BoundConstraint;

class DisjunctionInMarabou;

class ConjunctionInMarabou;

namespace MARABOU_IN_PLAJA { class SuccessorGenerator; }

namespace MARABOU_IN_PLAJA { class Context; }

namespace MARABOU_IN_PLAJA { class QueryConstructable; }

namespace MARABOU_IN_PLAJA { class MarabouQuery; }

class NNInMarabou;

namespace MARABOU_IN_PLAJA { class OutputConstraints; }

namespace MARABOU_IN_PLAJA { class OutputConstraintsNoFilter; }

class StateIndexesInMarabou;

class OpApplicability; // For convenience.

#endif //PLAJA_FORWARD_SMT_NN_H
