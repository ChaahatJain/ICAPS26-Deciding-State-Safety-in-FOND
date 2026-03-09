#
# This file is part of the PlaJA code base.
# Copyright (c) (2019 - 2023) Marcel Vinzent.
# See README.md in the top-level directory for licensing information.
#

# Compile time configuration:

SET(ENABLE_APPLICABILITY_FILTERING ON)
SET(ENABLE_APPLICABILITY_CACHE ON) # Optimization when using applicability filtering.
SET(ENABLE_TERMINAL_STATE_SUPPORT ON)
SET(REACH_MAY_BE_TERMINAL ON) # Reach states may be terminal -- disable to encode reach-avoid.

# PA:

set(LAZY_PA OFF)
set(QUERY_PER_BRANCH_INIT ON) # Query satisfiability for each predicate set during initial state enumeration.
set(QUERY_PER_BRANCH_EXPANSION OFF) # Query satisfiability for each predicate set during the predicate state enumeration of expansion.
set(ENABLE_HEURISTIC_SEARCH_PA ON) # Enable heuristic search options for predicate abstraction.

set(PA_ONLY_REACHABILITY ON) # Only check for reachability, skipping redundant transition tests.

# Transition tests
set(REUSE_SOLUTIONS ON) # Try to reuse solutions of previous queries.
set(ROUND_RELAXED_SOLUTIONS ON) # Round relaxed solutions to check for integer solution, e.g., in branch & bound.
set(SUPPORT_RELAXED_NN_SAT OFF) # Support continuously-relaxed NN-SAT checks.
#
set(DO_SELECTION_TEST OFF) # Enable action selection tests.
set(DO_NN_APPLICABILITY_TEST OFF) # Enable (NN-SAT) applicability tests.
set(DO_RELAXED_SELECTION_TEST_ONLY OFF) # Only support relaxed action selection tests.
set(DO_RELAXED_NN_APPLICABILITY_TEST_ONLY OFF) # Only support relaxed (NN-SAT) applicability tests.
#
# Reusing solutions:
# Applicability test are redundant on e.g. deterministic npuzzle (under entailment optimizations and given box constraint predicates).
# Here reusing solutions will help, alternatively one might disable either applicability or transition tests (in the former case however effectively disabling step-reachability caching).

# Technical (enabled as runtime option)
SET(ENABLE_STALLING_DETECTION OFF) # CURRENTLY NOT MAINTAINED/SUPPORTED
SET(ENABLE_QUERY_CACHING OFF) # CURRENTLY NOT MAINTAINED/SUPPORTED

# Marabou

SET(MARABOU_DNC_SUPPORT OFF) # CURRENTLY NOT MAINTAINED/SUPPORTED
SET(MARABOU_DIS_BASELINE_SUPPORT OFF) # If disabled all disjunctions optimizations are used.
SET(MARABOU_IGNORE_ARRAY_LEGACY OFF) # Some dangerous legacy support.
