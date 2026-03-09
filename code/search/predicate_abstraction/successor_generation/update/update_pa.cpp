//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "update_pa.h"
#include "../../../../option_parser/option_parser.h"
#include "../../../../parser/ast/expression/expression.h"
#include "../../../../parser/visitor/extern/variables_of.h"
#include "../../../factories/predicate_abstraction/pa_options.h"
#include "../../../smt/solver/smt_solver_z3.h"
#include "../../../smt/visitor/extern/to_z3_visitor.h"
#include "../../../smt/vars_in_z3.h"
#include "../../optimization/pred_entailment_cache_interface.h"
#include "../../optimization/predicate_relations.h"
#include "../../pa_states/abstract_state.h"
#include "../../pa_states/predicate_state.h"
#include "../../smt/model_z3_pa.h"

namespace UPDATE_PA {

    struct EntailmentInfo final {

        enum TruthValue { True, False, None }; // NOLINT(*-enum-size)

    private:
        const PredicateIndex_type predIndex;
        const TruthValue positiveEntailment; // if true in the source, then ...
        const TruthValue negativeEntailment; // if false in the source, then ...

    public:
        explicit EntailmentInfo(PredicateIndex_type pred_index, TruthValue pos_entailment, TruthValue neg_entailment): // NOLINT(*-easily-swappable-parameters)
            predIndex(pred_index)
            , positiveEntailment(pos_entailment)
            , negativeEntailment(neg_entailment) {} // NOLINT(bugprone-easily-swappable-parameters)
        ~EntailmentInfo() = default;
        DELETE_CONSTRUCTOR(EntailmentInfo)

        inline void set_entailment(const AbstractState& source, PredicateState& pred_state) const {
            PLAJA_ASSERT(pred_state.is_undefined(predIndex)/* or ( source.predicate_value(predIndex) ? positiveEntailment : negativeEntailment ) == None*/) // latter is fall-back assert for inc refinement

            if constexpr (PLAJA_GLOBAL::lazyPA) { if (not source.is_set(predIndex)) { return; } }

            PLAJA_ASSERT(source.is_set(predIndex))

            switch (source.predicate_value(predIndex) ? positiveEntailment : negativeEntailment) {
                case True: {
                    pred_state.set_entailed(predIndex, true);
                    break;
                }
                case False: {
                    pred_state.set_entailed(predIndex, false);
                    break;
                }
                case None: break;
            }
        }

        [[nodiscard]] inline bool is_always_entailed() const { return positiveEntailment != TruthValue::None and negativeEntailment != TruthValue::None; }

    };

    struct UpdateInZ3 {
        UpdatePA* parent;
        std::vector<z3::expr> targetPredicates; // computed based on assigned variables

        explicit UpdateInZ3(UpdatePA& parent_ref):
            parent(&parent_ref) {
        }

        ~UpdateInZ3() = default;

        DELETE_CONSTRUCTOR(UpdateInZ3)

        [[nodiscard]] inline bool is_helpful(const ModelZ3PA& model_z3, PredicateIndex_type pred_index) const {
            // (entailed) bounds are helpful (if they are encoded; otherwise they are variable disjoint)
            if (Z3_IN_PLAJA::expr_is(targetPredicates[pred_index])) { return model_z3.predicate_is_bound(pred_index); }
            return false;
        }

        [[nodiscard]] inline std::unique_ptr<Z3_IN_PLAJA::StateVarsInZ3> target_vars_to_z3(const ModelZ3& model_z3) const {
            std::unique_ptr<Z3_IN_PLAJA::StateVarsInZ3> target_vars_z3(new Z3_IN_PLAJA::StateVarsInZ3(model_z3.get_src_vars()));
            const auto& target_vars = model_z3.get_target_vars();
            for (const VariableID_type var_id: parent->_updated_vars()) { target_vars_z3->set(var_id, target_vars.to_z3(var_id)); }
            return target_vars_z3; //: target vars if updated, srcs vars otherwise
        }

        void compute_static_predicate_entailment(PredicateIndex_type pred_index, Z3_IN_PLAJA::SMTSolver& solver) {
            const auto& model_z3 = parent->get_model_z3_pa();

            solver.push();
            parent->add_to_solver(solver); // add update and target information
            PLAJA_ASSERT(Z3_IN_PLAJA::expr_is(targetPredicates[pred_index]))

            // check for positive predicate:
            EntailmentInfo::TruthValue pos_entailment = EntailmentInfo::None;
            solver.push();
            solver.add(model_z3.get_src_predicate(pred_index));
            // if predicate is false, both checks will be unsat, however for us ok to then just fix any value
            if (not solver.check_push_pop(targetPredicates[pred_index])) { pos_entailment = EntailmentInfo::False; } // positive target;
            else if (not solver.check_push_pop(!targetPredicates[pred_index])) { pos_entailment = EntailmentInfo::True; } // negative target
            solver.pop();

            // check for negative predicate:
            EntailmentInfo::TruthValue neg_entailment = EntailmentInfo::None;
            solver.push();
            solver.add(!model_z3.get_src_predicate(pred_index));
            if (not solver.check_push_pop(targetPredicates[pred_index])) { neg_entailment = EntailmentInfo::False; } // positive target
            else if (not solver.check_push_pop(!targetPredicates[pred_index])) { neg_entailment = EntailmentInfo::True; } // negative target
            solver.pop();

            solver.pop();

            if (pos_entailment != EntailmentInfo::None or neg_entailment != EntailmentInfo::None) {
                if (pos_entailment == EntailmentInfo::True and neg_entailment == EntailmentInfo::False) { // frequent special case of entailment (subsuming variable disjoint see below)
                    parent->invariantPreds.push_back(pred_index);
                    if (not model_z3.predicate_is_bound(pred_index)) { targetPredicates[pred_index] = z3::expr(solver.get_context_z3()); } // delete unused z3::expr:
                } else {
                    parent->staticPredEntailments.emplace_back(pred_index, pos_entailment, neg_entailment);
                    if (parent->staticPredEntailments.back().is_always_entailed() and not model_z3.predicate_is_bound(pred_index)) { targetPredicates[pred_index] = z3::expr(solver.get_context_z3()); }
                }
            }
        }

        inline void compute_and_set_predicate_entailment(PredicateIndex_type pred_index, PredicateState& pred_state, Z3_IN_PLAJA::SMTSolver& solver, const PredicateRelations* predicate_relations, std::unordered_map<PredicateIndex_type, bool>* entailments_cache) const {
            PLAJA_ASSERT(not pred_state.is_defined(pred_index))

            PLAJA_ASSERT(PLAJA_GLOBAL::lazyPA or Z3_IN_PLAJA::expr_is(targetPredicates[pred_index]))

            if (PLAJA_GLOBAL::lazyPA and Z3_IN_PLAJA::expr_is_none(targetPredicates[pred_index])) { return; } // predicate may be invariant but unknown in source

            bool truth_value;
            if (not solver.check_push_pop(targetPredicates[pred_index])) { truth_value = false; }
            else if (not solver.check_push_pop(!targetPredicates[pred_index])) { truth_value = true; }
            else { return; }

            // handle entailment
            pred_state.set_entailed(pred_index, truth_value);

            if (entailments_cache) {

                if (predicate_relations) { // to skip unnecessary tests
                    for (const auto& [pred_index_, truth_value_]: predicate_relations->entailments_for(pred_index, truth_value)) {
                        pred_state.set_entailed(pred_index_, truth_value_);
                        auto it = entailments_cache->find(pred_index_); // already added as entailed by update?
                        if (it != entailments_cache->cend()) { entailments_cache->erase(it); }
                    }
                }

                entailments_cache->emplace(pred_index, truth_value);

            } else if (predicate_relations) { predicate_relations->fix_predicate_state_asc_if_non_lazy(pred_state, pred_index); } // to skip unnecessary tests
        }

        void compute_and_set_predicate_entailments(PredicateState& pred_state, Z3_IN_PLAJA::SMTSolver& solver, const PredEntailmentCacheInterface& entailment_cache_interface) const {
            PLAJA_ASSERT(pred_state.get_size_predicates() == targetPredicates.size())

            if (not PLAJA_GLOBAL::lazyPA) { entailment_cache_interface.load_entailment(pred_state); }

            std::unordered_map<PredicateIndex_type, bool> new_entailments;

            solver.push(); // actually only required if we add entailed predicate values during iteration; which is redundant: if S \land g \land u -> p, then: if S \land g \land u \land p -> p', then also S \land g \land u -> p'

            // check for each predicate ...
            for (auto it = pred_state.init_pred_state_iterator(); !it.end(); ++it) {
                auto pred_index = it.predicate_index();

                if (pred_state.is_defined(pred_index)) {
                    PLAJA_ASSERT(pred_state.is_entailed(pred_index)) // expected
                    continue;
                }

                // We need to (re-)compute entailment iff:
                // (1) entailment for that predicate has not been checked before
                // (2) the source involves a new predicate that affects the update operator structure (including guard)
                if (not PLAJA_GLOBAL::lazyPA and pred_index < entailment_cache_interface.get_last_predicates_size() and parent->lastAffectingPred < entailment_cache_interface.get_last_predicates_size()) {  // no entailment will be found
                    if constexpr (PLAJA_GLOBAL::debug) { compute_and_set_predicate_entailment(pred_index, pred_state, solver, entailment_cache_interface.get_predicate_relations(), &new_entailments); }
                    PLAJA_LOG_DEBUG_IF(pred_state.is_defined(pred_index), PLAJA_UTILS::string_f("Num preds: %i, predicate: %i, last affected: %i, last_predicate_size: %i.", pred_state.get_size_predicates(), pred_index, parent->lastAffectingPred, entailment_cache_interface.get_last_predicates_size()))
                    if constexpr (PLAJA_GLOBAL::debug) { if (pred_state.is_defined(pred_index)) { solver.dump_solver(); } }
                    PLAJA_ASSERT(not pred_state.is_defined(pred_index))
                    continue;
                }

                compute_and_set_predicate_entailment(pred_index, pred_state, solver, entailment_cache_interface.get_predicate_relations(), &new_entailments);
            }

            solver.pop();

            if (not PLAJA_GLOBAL::lazyPA) { entailment_cache_interface.store_entailment(new_entailments, pred_state.get_size_predicates()); }
        }

    };

}

/**********************************************************************************************************************/

bool UpdatePA::predicateEntailmentsFlag = false;

/* */

UpdatePA::UpdatePA(const Update& parent, const ModelZ3PA& model_z3, Z3_IN_PLAJA::SMTSolver& solver):
    UpdateAbstract(parent, model_z3)
    , updateInZ3PA(new UPDATE_PA::UpdateInZ3(*this))
    , lastAffectingPred(get_model_z3_pa().get_number_predicates() - 1) {
    PLAJA_ASSERT(staticPredEntailments.empty())
    PLAJA_ASSERT(get_model_z3_pa().get_number_predicates() > 0)
    finalize(solver);
}

UpdatePA::~UpdatePA() = default;

void UpdatePA::set_flags() {
    predicateEntailmentsFlag = PLAJA_GLOBAL::optionParser->get_bool_option(PLAJA_OPTION::predicate_entailments);
    UpdateAbstract::set_flags(false, true);
}

void UpdatePA::increment(Z3_IN_PLAJA::SMTSolver& solver) { finalize(solver); }

/* */

const ModelZ3PA& UpdatePA::get_model_z3_pa() const { return static_cast<const ModelZ3PA&>(_model_z3()); } // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)

std::size_t UpdatePA::get_number_predicates() const { return get_model_z3_pa().get_number_predicates(); }

const Expression* UpdatePA::get_predicate(PredicateIndex_type index) const { return get_model_z3_pa().get_predicate(index); }

/* */

bool UpdatePA::is_always_entailed(PredicateIndex_type pred_index) const {
    PLAJA_ASSERT(pred_index < updateInZ3PA->targetPredicates.size())
    return Z3_IN_PLAJA::expr_is_none(updateInZ3PA->targetPredicates[pred_index]);
}

/* */

void UpdatePA::add_predicate_to_solver(Z3_IN_PLAJA::SMTSolver& solver, PredicateIndex_type pred_index, bool value) const {
    const auto& target_predicates = updateInZ3PA->targetPredicates;
    PLAJA_ASSERT(pred_index <= target_predicates.size())
    PLAJA_ASSERT(PLAJA_GLOBAL::lazyPA or Z3_IN_PLAJA::expr_is(target_predicates[pred_index]))

    if (PLAJA_GLOBAL::lazyPA and Z3_IN_PLAJA::expr_is_none(target_predicates[pred_index])) {
        /* Invariant predicate unknown in source but set in target.
         * Just add source variable predicate */

        if (value) { solver.add(get_model_z3_pa().get_src_predicate(pred_index)); }
        else { solver.add(!get_model_z3_pa().get_src_predicate(pred_index)); }

    } else {

        if (value) { solver.add(target_predicates[pred_index]); }
        else { solver.add(!target_predicates[pred_index]); }

    }

}

void UpdatePA::add_predicates_to_solver(Z3_IN_PLAJA::SMTSolver& solver, const AbstractState& target) const {
    PLAJA_ASSERT(target.get_size_predicates() == updateInZ3PA->targetPredicates.size())
    PLAJA_ASSERT(target.has_entailment_information())
    // add predicates
    for (auto pred_it = target.init_pred_state_iterator(); !pred_it.end(); ++pred_it) {
        if (pred_it.is_entailed() and not updateInZ3PA->is_helpful(get_model_z3_pa(), pred_it.predicate_index())) { continue; }

        if (PLAJA_GLOBAL::lazyPA and not pred_it.is_set()) {
            if (pred_it.is_entailed()) { add_predicate_to_solver(solver, pred_it.predicate_index(), pred_it.entailment_value()); }
        } else {
            PLAJA_ASSERT(pred_it.is_set())
            add_predicate_to_solver(solver, pred_it.predicate_index(), pred_it.predicate_value());
        }

    }
}

bool UpdatePA::check_sat(Z3_IN_PLAJA::SMTSolver& solver, PredicateIndex_type pred_index, bool value) const {
    PLAJA_ASSERT(pred_index <= updateInZ3PA->targetPredicates.size())
    PLAJA_ASSERT(updateInZ3PA->targetPredicates[pred_index])
    return solver.check_push_pop(value ? updateInZ3PA->targetPredicates[pred_index] : !updateInZ3PA->targetPredicates[pred_index]);
}

bool UpdatePA::check_sat(Z3_IN_PLAJA::SMTSolver& solver, const AbstractState& target) const {
    solver.push();
    // add predicates
    add_predicates_to_solver(solver, target);
    const bool rlt = solver.check();
    solver.pop();
    return rlt;
}

/** finalization routines *********************************************************************************************/

void UpdatePA::finalize(Z3_IN_PLAJA::SMTSolver& solver) {
    const auto& model_z3 = get_model_z3_pa();
    auto new_num_preds = model_z3.get_number_predicates();
    PLAJA_ASSERT((PLAJA_GLOBAL::lazyPA and updateInZ3PA->targetPredicates.size() == new_num_preds) or updateInZ3PA->targetPredicates.size() < new_num_preds) // incremental usage: number of predicates should only increase

    // target vars set for target-invariant optimization (see below)
    const auto target_vars_z3 = updateInZ3PA->target_vars_to_z3(model_z3);
    const auto target_vars_set = std::unordered_set<VariableID_type>(_updated_vars().cbegin(), _updated_vars().cend());

    for (PredicateIndex_type pred_index = updateInZ3PA->targetPredicates.size(); pred_index < new_num_preds; ++pred_index) { // incremental usage: reuse predicates so far
        const auto* predicate = model_z3.get_predicate(pred_index);
        // check target invariant
        if (not VARIABLES_OF::contains(predicate, target_vars_set, false)) {
            invariantPreds.push_back(pred_index);
            updateInZ3PA->targetPredicates.emplace_back(model_z3.get_context_z3());
        } else {
            updateInZ3PA->targetPredicates.push_back(Z3_IN_PLAJA::to_z3_condition(*predicate, *target_vars_z3)); // predicate encoding
            if (predicateEntailmentsFlag) { updateInZ3PA->compute_static_predicate_entailment(pred_index, solver); } // optimization
        }
    }
}

/** optimizations ++++++++*********************************************************************************************/

void UpdatePA::fix(PredicateState& target, Z3_IN_PLAJA::SMTSolver& solver, const AbstractState& source, const PredEntailmentCacheInterface& entailment_cache_interface) const {

    // STATIC ENTAILMENTS

    for (const PredicateIndex_type pred: invariantPreds) {
        PLAJA_ASSERT(not target.is_defined(pred))
        if (not PLAJA_GLOBAL::lazyPA or source.is_set(pred)) { target.set_entailed(pred, source.predicate_value(pred)); }
    }

    for (const auto& entailment_info: staticPredEntailments) { entailment_info.set_entailment(source, target); }

    // DYNAMIC ENTAILMENTS

    if (predicateEntailmentsFlag) { updateInZ3PA->compute_and_set_predicate_entailments(target, solver, entailment_cache_interface); }

    if (PLAJA_GLOBAL::lazyPA and entailment_cache_interface.get_predicate_relations()) { entailment_cache_interface.get_predicate_relations()->fix_predicate_state_all(target); }

}
