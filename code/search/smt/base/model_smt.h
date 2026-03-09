//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_MODEL_SMT_H
#define PLAJA_MODEL_SMT_H

#include <list>
#include <memory>
#include <vector>
#include "../../../include/ct_config_const.h"
#include "../../../parser/ast/expression/forward_expression.h"
#include "../../../parser/ast/forward_ast.h"
#include "../../../parser/using_parser.h"
#include "../../../assertions.h"
#include "../../factories/forward_factories.h"
#include "../../information/jani2nnet/forward_jani_2_nnet.h"
#include "../../information/jani2ensemble/forward_jani_2_ensemble.h"
#include "../../information/jani_2_interface.h"
#include "../../information/jani2nnet/jani_2_nnet.h"

#ifdef USE_VERITAS
#include "../../information/jani2ensemble/jani_2_ensemble.h"
#endif

#include "../../information/forward_information.h"
#include "../../successor_generation/forward_successor_generation.h"
#include "../../using_search.h"
#include "../forward_smt_z3.h"
#include "forward_smt.h"

/** Intended as a interface for SMT-related models (Z3, Marabou, possibly Gurobi, Veritas ?) */
class ModelSmt {

private:
    mutable std::shared_ptr<PLAJA::SmtContext> context; // Context needs to be last *SMT struct* to be destructed.

protected:
    const Model* model;
    const ModelInformation* modelInfo;
    const Jani2Interface* interface;
    std::list<VariableIndex_type> inputLocations; // Cache input locations.
    std::unique_ptr<Expression> start;
    std::unique_ptr<Expression> reach;
    FIELD_IF_TERMINAL_STATE_SUPPORT(std::unique_ptr<Expression> nonTerminal;)
    FIELD_IF_TERMINAL_STATE_SUPPORT(std::unique_ptr<Expression> reachNonTerminal;)
    const PredicatesExpression* predicates; // To be used by PA-based SMT-models.

    std::shared_ptr<SuccessorGeneratorC> successorGenerator;

    /**
     * Shared cached for (constraint-dependent) applicability of operator guards.
     * Put into ModelSmt for convenience. A SuccessorGeneratorSmt base class might be more suitable.
     * During abstract state expansion of policy predicate abstraction we want to cache abstract source state dependent applicability
     * to optimize NN-sat checks under applicability filtering.
     */
    mutable std::shared_ptr<OpApplicability> cachedOpApp;

    /* Step related. */
    bool ignoreLocations; // Vars/ops include locs?
    StepIndex_type maxStep;

    /* Cache SMT constraints. */ // TODO for now construct in sub-classes.
    FIELD_IF_TERMINAL_STATE_SUPPORT(std::vector<std::unique_ptr<PLAJA::SmtConstraint>> nonTerminalSmt;)

    [[nodiscard]] inline std::shared_ptr<PLAJA::SmtContext>& get_context_ptr() const { return context; }

public:
    ModelSmt(std::shared_ptr<PLAJA::SmtContext>&& context, const PLAJA::Configuration& config);
    virtual ~ModelSmt() = 0;
    DELETE_CONSTRUCTOR(ModelSmt)

    void set_interface(const Jani2Interface* new_interface);

    virtual void set_start(std::unique_ptr<Expression>&& start);

    virtual void set_reach(std::unique_ptr<Expression>&& reach);
    virtual void set_non_terminal(std::unique_ptr<Expression>&& terminal, bool negate);

    /* Retrieve original structures. */

    [[nodiscard]] inline const Model& get_model() const { return *model; }

    [[nodiscard]] inline const ModelInformation& get_model_info() const { return *modelInfo; }

    [[nodiscard]] std::size_t get_number_automata_instances() const;

    [[nodiscard]] std::size_t get_array_size(VariableID_type var_id) const;

    [[nodiscard]] inline const Jani2Interface* get_interface() const { return interface; }

    [[nodiscard]] inline bool has_interface() const { return get_interface(); }

    [[nodiscard]] inline bool has_nn() const { auto nnInterface = dynamic_cast<const Jani2NNet*>(interface); return nnInterface; }

#ifdef USE_VERITAS

    [[nodiscard]] inline bool has_ensemble() const { auto ensembleInterface = dynamic_cast<const Jani2Ensemble*>(interface); return ensembleInterface; }
#endif

    [[nodiscard]] inline const std::list<VariableIndex_type>& get_input_locations() const { return inputLocations; }

    [[nodiscard]] inline const Expression* get_start() const { return start.get(); }

    [[nodiscard]] virtual bool initial_state_is_subsumed_by_start() const;

    [[nodiscard]] inline const Expression* get_reach() const { return reach.get(); }

    [[nodiscard]] const Expression* get_terminal() const;

#ifdef ENABLE_TERMINAL_STATE_SUPPORT

    [[nodiscard]] const Expression* get_non_terminal() const { return nonTerminal.get(); }

    [[nodiscard]] const Expression* get_reach_non_terminal() const {
        PLAJA_ASSERT(not PLAJA_GLOBAL::reachMayBeTerminal)
        PLAJA_ASSERT(reachNonTerminal)
        return reachNonTerminal.get();
    }

#else

    [[nodiscard]] const Expression* get_non_terminal() const { PLAJA_ABORT }
    [[nodiscard]] const Expression* get_reach_non_terminal() const { PLAJA_ABORT }

#endif

    /* Predicates. */

    [[nodiscard]] inline const PredicatesExpression* _predicates() const { return predicates; }

    [[nodiscard]] std::size_t get_number_predicates() const;

    [[nodiscard]] const Expression* get_predicate(std::size_t index) const;

    [[nodiscard]] bool predicate_is_bound(std::size_t index) const;

    /* */

    [[nodiscard]] inline const SuccessorGeneratorC& get_successor_generator() const { return *successorGenerator; }

    [[nodiscard]] const class ActionOp& get_action_op_concrete(ActionOpID_type op_id) const;

    /* Op Applicability cache. */

    void share_op_applicability(std::shared_ptr<OpApplicability> op_app) const;

    [[nodiscard]] inline OpApplicability* get_op_applicability() const { return cachedOpApp.get(); }

    void set_self_applicability(bool value) const;

    void unset_self_applicability() const;

    /* BMC: We always add self-applicability. */

    void add_self_applicability(bool value) const;

    inline void unset_op_applicability() const { cachedOpApp = nullptr; }

    /** Step interface ************************************************************************************************/

    [[nodiscard]] inline bool ignore_locs() const { return ignoreLocations; }

    [[nodiscard]] inline std::size_t get_max_path_len() const { return maxStep; }

    [[nodiscard]] inline StepIndex_type get_max_step() const { return maxStep; }

    [[nodiscard]] inline StepIndex_type get_max_num_step() const { return maxStep + 1; }

    virtual void generate_steps(StepIndex_type /*max_step*/) {}; // TODO make abstract

    /* Iterators look like overkill, but we are going to use them throughout the code. So let's unify here. */

    class StepIterator { // NOLINT(cppcoreguidelines-special-member-functions)
        friend ModelSmt;

    protected:
        const StepIndex_type maxStep; // NOLINT(*-avoid-const-or-ref-data-members)
        StepIndex_type currentIndex;

        explicit StepIterator(StepIndex_type max_step, StepIndex_type start_step = 0):
            maxStep(max_step)
            , currentIndex(start_step) {} // NOLINT(bugprone-easily-swappable-parameters)
    public:
        virtual ~StepIterator() = default;
        DELETE_CONSTRUCTOR(StepIterator)

        inline void operator++() { ++currentIndex; }

        [[nodiscard]] virtual inline bool end() const { return currentIndex > maxStep; } // stop *after* max step

        [[nodiscard]] inline StepIndex_type step() const { return currentIndex; }

    };

    [[nodiscard]] inline StepIterator iterate_steps(StepIndex_type start_step = 0) const { return StepIterator(get_max_step(), start_step); }

    [[nodiscard]] inline static StepIterator iterate_steps_from_to(StepIndex_type start_step, StepIndex_type max_step) { return StepIterator(max_step, start_step); }

    class PathIterator final: public StepIterator { // NOLINT(cppcoreguidelines-special-member-functions)
        friend ModelSmt;

    private:
        explicit PathIterator(StepIndex_type max_step, StepIndex_type start_step = 0):
            StepIterator(max_step, start_step) {}

    public:
        ~PathIterator() final = default;
        DELETE_CONSTRUCTOR(PathIterator)

        [[nodiscard]] inline bool end() const override { return currentIndex >= maxStep; } // stop *before* max step

        [[nodiscard]] inline StepIndex_type successor() const {
            PLAJA_ASSERT(currentIndex < maxStep)
            return currentIndex + 1;
        }

    };

    [[nodiscard]] inline PathIterator iterate_path(StepIndex_type start_step = 0) const { return PathIterator(get_max_step(), start_step); }

    [[nodiscard]] inline static PathIterator iterate_path_from_to(StepIndex_type start_step, StepIndex_type end_step) { return PathIterator(end_step, start_step); }

    /** Solver interface **********************************************************************************************/

    [[nodiscard]] virtual std::unique_ptr<PLAJA::SmtSolver> init_solver(const PLAJA::Configuration& config, StepIndex_type step) const = 0;

    [[nodiscard]] virtual std::unique_ptr<PLAJA::SmtConstraint> to_smt(const Expression& expr, StepIndex_type step) const = 0;

    virtual void add_to_solver(PLAJA::SmtSolver& solver, const Expression& expr, StepIndex_type step) const = 0;


    /** @param prepare Perform some preprocessing operations to achieve a certain form. */
    [[nodiscard]] std::unique_ptr<Expression> optimize_expression(std::unique_ptr<Expression> expr, PLAJA::SmtSolver& solver, bool prepare, bool check_entailment, bool prune_conjuncts, bool optimize_disjuncts) const;
    [[nodiscard]] std::unique_ptr<Expression> optimize_expression(const Expression& expr, PLAJA::SmtSolver& solver, bool prepare, bool check_entailment, bool prune_conjuncts, bool optimize_disjuncts) const;

    /* Per step */

    void add_terminal(PLAJA::SmtSolver& solver, StepIndex_type step) const;

    void exclude_terminal(PLAJA::SmtSolver& solver, StepIndex_type step) const;

};

#endif //PLAJA_MODEL_SMT_H
