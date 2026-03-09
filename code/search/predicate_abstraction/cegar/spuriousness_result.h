//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SPURIOUSNESS_RESULT_H
#define PLAJA_SPURIOUSNESS_RESULT_H

#include <list>
#include <memory>
#include <vector>
#include "../../../parser/ast/expression/forward_expression.h"
#include "../../../parser/visitor/forward_visitor.h"
#include "../../../stats/forward_stats.h"
#include "../../../utils/default_constructors.h"
#include "../pa_states/forward_pa_states.h"
#include "../../successor_generation/forward_successor_generation.h"
#include "../../using_search.h"
#include "../using_predicate_abstraction.h"
#include "predicates_structure.h"
#include "../forward_pa.h"
#include "forward_cegar.h"
#include "../solution_checker_instance_pa.h"

struct PathWP;

class SolutionCheckerInstancePaPath;

class SpuriousnessResult final {
    friend SpuriousnessChecker;
    friend PathExistenceChecker;

public:
    enum SpuriousnessCause { None, Concretization, Terminal, GuardApp, ActionSel, Target };

private:
    const PredicatesStructure* predicates;
    std::unique_ptr<SolutionCheckerInstancePaPath> pathInstance;
    const AbstractPath* path; // As stored in path instance.

    StepIndex_type targetStep; // i.e., non-spurious prefix, not the targetStep as per path instances!
    SpuriousnessCause spuriousnessCause;
    FIELD_IF_LAZY_PA(bool doLocalRefinement;) // TODO for now either local or global per refinement, if we ever intermingle: first add global then local

    std::unique_ptr<PathWP> pathWpIn;
    std::unique_ptr<PathWP> pathWpOut;
    std::vector<std::pair<ExpressionSet, ExpressionSet>> predicateSet; // Predicates in wp out and cache of predicates already used in pa state (per path step).

    std::list<std::unique_ptr<Expression>> globalPredicates;
    std::pair<ExpressionSet, ExpressionSet> globalPredicatesSet;

    PredicatesNumber_type globalPredicatesAdded; // Stats during refine.
    PredicatesNumber_type localPredicatesAdded;

    [[nodiscard]] inline bool do_local_refinement() const {
#ifdef LAZY_PA
        return doLocalRefinement;
#else
        return false;
#endif
    }

    /* Routine to be used by friends. As it enables some simplifications. */
    [[nodiscard]] std::unique_ptr<SolutionCheckerInstancePaPath> retrieve_path_instance();

    void set_concretization(std::vector<std::unique_ptr<StateValues>>&& concretization);

    void set_spurious(SpuriousnessCause cause);

    inline void set_prefix_length(StepIndex_type target_step) { targetStep = target_step; }

    ExpressionSet& get_new_predicates(StepIndex_type step);

    void compute_wp();
    void compute_wp_internals();
    void add_wp_out(std::unique_ptr<Expression>&& pred, StepIndex_type step);

    explicit SpuriousnessResult(const PredicatesStructure& predicates, std::unique_ptr<SolutionCheckerInstancePaPath>&& path_instance);
public:
    ~SpuriousnessResult();
    DELETE_CONSTRUCTOR(SpuriousnessResult)

    [[nodiscard]] inline const PredicatesStructure& get_predicates() const { return *predicates; }

    [[nodiscard]] inline const SolutionCheckerInstancePaPath& get_path_instance() const { return *pathInstance; }

    [[nodiscard]] const ExpressionSet* get_used_predicates(StepIndex_type step) const;

    [[nodiscard]] const PaStateBase& get_pa_state(StepIndex_type step) const;

    [[nodiscard]] const PaStateBase* get_pa_state_if_local(StepIndex_type step) const;

    [[nodiscard]] inline bool do_add_non_entailed_guards_externally() const { return predicates->do_add_guards() and predicates->do_exclude_entailments(); }

    [[nodiscard]] inline bool do_add_non_entailed_predicates_externally() const { return predicates->do_add_predicates() and predicates->do_exclude_entailments(); }

    [[nodiscard]] inline bool do_add_guards_internally() const { return predicates->do_add_guards() and not predicates->do_exclude_entailments(); }

    [[nodiscard]] inline bool do_add_predicates_internally() const { return predicates->do_add_predicates() and not predicates->do_exclude_entailments(); }

    [[nodiscard]] inline bool do_exclude_entailment() const { return predicates->do_exclude_entailments(); }

    [[nodiscard]] inline bool is_spurious() const { return spuriousnessCause != SpuriousnessCause::None; }

    [[nodiscard]] inline SpuriousnessCause get_spuriousness_cause() const { return spuriousnessCause; }

    [[nodiscard]] inline std::size_t get_prefix_length() const { return targetStep; }

    [[nodiscard]] inline std::size_t get_spurious_prefix_length() const {
        PLAJA_ASSERT(is_spurious())
        return targetStep;
    }

    std::unordered_set<VariableID_type> collect_updated_vars();
    std::unordered_set<VariableIndex_type> collect_updated_state_indexes(bool include_locs);
    std::list<VariableIndex_type> compute_entailments(const StateBase& state, bool complete);
    std::unordered_set<VariableIndex_type> compute_entailments_set(const StateBase& state, bool complete);

    void update_stats(PLAJA::StatsBase& stats);

    /** Add predicate/constraint for refinement. */
    void add_refinement_predicate(std::unique_ptr<Expression>&& refinement_pred, StepIndex_type step_index);

    /** Add existing predicate for refinement, i.e., feed through wp computation of spurious path. */
    void add_refinement_predicate(PredicateIndex_type predicate_index, StepIndex_type step_index);

    /** Immediately trigger wp computation for added predicate. */
    void add_refinement_predicate_wp(std::unique_ptr<Expression>&& refinement_pred);

    /** Predicate is not associated with a path step & will not be feed to wp computation. */
    void add_refinement_predicate(std::unique_ptr<Expression>&& refinement_pred);

    [[nodiscard]] bool refine(PredicatesStructure& predicates_); // return number of preds added

    [[nodiscard]] bool has_new_predicates() const;

    void dump_concretization(const std::string& filename) const;

};

#endif //PLAJA_SPURIOUSNESS_RESULT_H
