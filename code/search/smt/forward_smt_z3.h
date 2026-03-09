//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_FORWARD_SMT_Z3_H
#define PLAJA_FORWARD_SMT_Z3_H

#include "base/forward_smt.h"

class ModelZ3;

class NNInZ3;

namespace Z3_IN_PLAJA { class OutputConstraints; }

namespace Z3_IN_PLAJA { class OutputConstraintsNoFilter; }

namespace Z3_IN_PLAJA { class SMTSolver; }

namespace Z3_IN_PLAJA { class SolutionCheckWrapper; }

namespace Z3_IN_PLAJA { class Solution; }

class ActionOpToZ3;

class SuccessorGeneratorToZ3;

class OpApplicability;

class UpdateToZ3;

struct ToZ3Expr;

struct ToZ3ExprSplits;

namespace Z3_IN_PLAJA { class Constraint; }

namespace Z3_IN_PLAJA { class ConstraintExpr; }

namespace Z3_IN_PLAJA { class ConstraintTrivial; }

namespace Z3_IN_PLAJA { class ConstraintVec; }

namespace Z3_IN_PLAJA { class Context; }

namespace Z3_IN_PLAJA { class StateVarsInZ3; }

namespace Z3_IN_PLAJA { class StateIndexes; }

#endif //PLAJA_FORWARD_SMT_Z3_H
