//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "predicate_relations.h"
#include "../../factories/configuration.h"
#include "../../smt/solver/smt_solver_z3.h"
#include "../pa_states/predicate_state.h"
#include "../smt/model_z3_pa.h"

/* Hack to support "predicate_abstraction_test" legacy. */
STMT_IF_DEBUG(extern bool allowTrivialPreds)
STMT_IF_DEBUG(bool allowTrivialPreds = false)

// auxiliary

namespace PREDICATE_RELATIONS {

    inline bool check_push_pop_pair(Z3_IN_PLAJA::SMTSolver& solver, const z3::expr& e1, const z3::expr& e2) {
        solver.push();
        solver.add(e1);
        solver.add(e2);
        const auto rlt = solver.check();
        solver.pop();
        return rlt;
    }

    inline std::unique_ptr<PA::EntailmentValueType> check_ground_truth_entailment(Z3_IN_PLAJA::SMTSolver& solver, const z3::expr& pred) {
        // Check whether !pred  is entailed:
        if (not solver.check_push_pop(pred)) { return std::make_unique<PA::EntailmentValueType>(PA::False); }
        // Check whether  pred  is entailed:
        if (not solver.check_push_pop(!pred)) { return std::make_unique<PA::EntailmentValueType>(PA::True); }
        //
        return nullptr;
    }

    inline bool check_entailment(Z3_IN_PLAJA::SMTSolver& solver, const z3::expr& p_b, const z3::expr& p_e) {
        // Check whether base p_b entails a truth value for p_e:

        // Check whether p_e is entailed
        if (not PREDICATE_RELATIONS::check_push_pop_pair(solver, p_b, !p_e)) { return true; }

        // Check whether not p_e is entailed
        if (not PREDICATE_RELATIONS::check_push_pop_pair(solver, p_b, p_e)) { return true; }

        return false;

    }

    inline std::pair<std::unique_ptr<PA::EntailmentValueType>, std::unique_ptr<PA::EntailmentValueType>> check_binary_relation(Z3_IN_PLAJA::SMTSolver& solver, const z3::expr& pred1, const z3::expr& pred2) {

        std::unique_ptr<PA::EntailmentValueType> pos_entails; // i.e., P1 entails P2 iff true; and P1 entails !P2 iff false
        std::unique_ptr<PA::EntailmentValueType> neg_entails; // i.e., !P1 entails P2 iff true; and !P1 entails !P2 iff false

        // Check P1 && P2
        if (PREDICATE_RELATIONS::check_push_pop_pair(solver, pred1, pred2)) {
            bool rlt = true; // cache
            // Check !P1 && P2
            if (not PREDICATE_RELATIONS::check_push_pop_pair(solver, !pred1, pred2)) {
                rlt = false;
                neg_entails = std::make_unique<PA::EntailmentValueType>(PA::False); // !P1 entails !P2 (P2 entails P1)
            }
            // Check P1 && !P2
            if (not PREDICATE_RELATIONS::check_push_pop_pair(solver, pred1, !pred2)) {
                rlt = false;
                pos_entails = std::make_unique<PA::EntailmentValueType>(PA::True); // P1 entails P2 (!P2 entails !P1)
            }

            // if one of the above was unsat then do not check !P1 && !P2
            if (not rlt) { return std::make_pair(std::move(pos_entails), std::move(neg_entails)); }

        } else { // Do not check P1 && !P2 and !P1 && P2, they will be sat, except P1 or P2 are unsat themselves, which is excluded by invariant
            pos_entails = std::make_unique<PA::EntailmentValueType>(PA::False); // P1 entails !P2 (P2 entails !P1)
        }

        // Check !P1 && !P2 (note: for linear constraints if A && B is unsat, !A && !B is sat)
        if (not PREDICATE_RELATIONS::check_push_pop_pair(solver, !pred1, !pred2)) {
            neg_entails = std::make_unique<PA::EntailmentValueType>(PA::True); // !P1 entails P2 (!P2 entails P1)
        }

        return std::make_pair(std::move(pos_entails), std::move(neg_entails));
    }

    inline bool check_equivalence(Z3_IN_PLAJA::SMTSolver& solver, const z3::expr& pred1, const z3::expr& pred2) {
        return not solver.check_push_pop(pred1 != pred2);
    }
}

/**********************************************************************************************************************/

PredicateRelations::PredicateRelations(const PLAJA::Configuration& config, const ModelZ3PA& model_z3, const Expression* ground_truth):
    modelZ3Own(nullptr)
    , modelZ3(&model_z3)
    , solverOwn(new Z3_IN_PLAJA::SMTSolver(modelZ3->share_context(), config.get_sharable_ptr<PLAJA::StatsBase>(PLAJA::SharableKey::STATS)))
    , solver(solverOwn.get())
    CONSTRUCT_IF_DEBUG(emptyGroundTruth(not ground_truth)) {
    // variable bounds:
    modelZ3->add_src_bounds(*solver, true);
    // ground truth:
    if (ground_truth) { modelZ3->add_expression(*solver, *ground_truth, 0); }

    if (modelZ3->get_number_predicates() > 0) { increment(); }
}

#if 0
PredicateRelations::PredicateRelations(const Model& model, const PredicatesExpression& predicates, const Expression* ground_truth)
    : modelZ3Own(nullptr), modelZ3(modelZ3Own.get())
    , solverOwn(nullptr), solver(solverOwn.get()) {

    PLAJA::Configuration config;
    config.set_sharable_const(PLAJA::SharableKey::MODEL, &model);
    modelZ3Own = std::make_unique<ModelZ3PA>(config, &predicates);
    modelZ3 = modelZ3Own.get();

    solverOwn = std::make_unique<Z3_IN_PLAJA::SMTSolver>(modelZ3->get_context(), modelZ3->share_stats());
    solver = solverOwn.get();

    // variable bounds:
    modelZ3->add_src_bounds(*solver);
    // ground truth:
    if (ground_truth) { modelZ3->add_expression(*solver, *ground_truth, 0); }

    if (modelZ3->get_number_predicates() > 0) { increment(); }

    PLAJA_ASSERT(!is_any_entailed()) // predicates are intended to be neither trivially true nor trivially false
    PLAJA_ASSERT(!check_equivalence()) // predicates are expected to be non-redundant
}
#endif

PredicateRelations::PredicateRelations(const ModelZ3PA& model_z3, Z3_IN_PLAJA::SMTSolver& solver):
    modelZ3(&model_z3)
    , solver(&solver)
    CONSTRUCT_IF_DEBUG(emptyGroundTruth(false)) { // do not know ...
    if (modelZ3->get_number_predicates() > 0) { increment(); }
}

PredicateRelations::~PredicateRelations() = default;

std::unique_ptr<PredicateRelations> PredicateRelations::construct(const PLAJA::Configuration& config, const Expression* ground_truth) {
    PLAJA_ASSERT(config.has_sharable(PLAJA::SharableKey::MODEL_Z3))
    const auto& model_z3 = *config.get_sharable_cast<ModelZ3, ModelZ3PA>(PLAJA::SharableKey::MODEL_Z3);
    return std::make_unique<PredicateRelations>(config, model_z3, ground_truth);
}

/**********************************************************************************************************************/

void PredicateRelations::increment() {
    if (modelZ3Own) { modelZ3Own->increment(); }

    auto old_num_predicates = entailmentsPerPred.size();
    auto new_num_predicates = modelZ3->get_number_predicates(); // incremental usage: new number of predicates
    PLAJA_ASSERT(old_num_predicates <= new_num_predicates)

    if (old_num_predicates == new_num_predicates) { return; } // most likely already incremented by another "share holder" or we do lazy PA.

    PLAJA_ASSERT(entailmentsPerPred.size() == entailmentsPerNegPred.size())
    entailmentsPerPred.resize(new_num_predicates);
    entailmentsPerNegPred.resize(new_num_predicates);
    entailmentsForPred.resize(new_num_predicates);
    entailmentsForNegPred.resize(new_num_predicates);

    // compute entailments by ground truth
    PLAJA_ASSERT(solver->check()) // ground truth should be sat, however not a hard invariant
    for (PredicateIndex_type i = old_num_predicates; i < new_num_predicates; ++i) { compute_singleton_entailments(i); }

    // compute binary entailments
    // old-to-new predicates
    for (PredicateIndex_type i = 0; i < old_num_predicates; ++i) {
        if (is_entailed(i)) { continue; } // compute_binary_entailments assumes predicate as well as negation to be sat; also no point in computing entailment here
        for (PredicateIndex_type j = old_num_predicates; j < new_num_predicates; ++j) {
            if (is_entailed(j)) { continue; } // no point in computing entailment here
            compute_binary_entailments(i, j);
        }
    }
    // new-to-new predicates
    for (PredicateIndex_type i = old_num_predicates; i < new_num_predicates; ++i) {
        if (is_entailed(i)) { continue; }
        for (PredicateIndex_type j = i + 1; j < new_num_predicates; ++j) {
            if (is_entailed(j)) { continue; }
            compute_binary_entailments(i, j);
        }
    }

    // compute entailments per ascending index
    compute_entailments_per_asc_index();

    if (solverOwn) { solver->reset(); } // do not want to keep the cache from during PA search ...

    PLAJA_ASSERT(allowTrivialPreds or not has_empty_ground_truth() or not is_any_entailed()) // predicates are intended to be neither trivially true nor trivially false
    PLAJA_ASSERT(not has_empty_ground_truth() or not check_equivalence()) // predicates are expected to be non-redundant

}

// compute information:

void PredicateRelations::compute_singleton_entailments(PredicateIndex_type index) {
    auto ground_truth_entailment = PREDICATE_RELATIONS::check_ground_truth_entailment(*solver, modelZ3->get_src_predicate(index));
    if (ground_truth_entailment) { groundTruthEntailments.emplace(index, *ground_truth_entailment); }
}

void PredicateRelations::compute_binary_entailments(PredicateIndex_type index1, PredicateIndex_type index2) {
    PLAJA_ASSERT(!is_entailed(index1))

    auto binary_entailment = PREDICATE_RELATIONS::check_binary_relation(*solver, modelZ3->get_src_predicate(index1), modelZ3->get_src_predicate(index2));

    const auto& pos_entailment = binary_entailment.first;
    if (pos_entailment) {
        if (*pos_entailment) {
            entailmentsPerPred[index1].emplace_back(index2, true); // P1 entails P2
            entailmentsPerNegPred[index2].emplace_back(index1, false); // !P2 entails !P1
            //
            entailmentsForPred[index2].emplace_back(index1, true);
            entailmentsForNegPred[index1].emplace_back(index2, false);
        } else {
            entailmentsPerPred[index1].emplace_back(index2, false); // P1 entails !P2
            entailmentsPerPred[index2].emplace_back(index1, false); // P2 entails !P1
            //
            entailmentsForNegPred[index2].emplace_back(index1, true);
            entailmentsForNegPred[index1].emplace_back(index2, true);
        }
    }
    const auto& neg_entailment = binary_entailment.second;
    if (neg_entailment) {
        if (*neg_entailment) {
            entailmentsPerNegPred[index1].emplace_back(index2, true); // !P1 entails P2
            entailmentsPerNegPred[index2].emplace_back(index1, true); // !P2 entails P1
            //
            entailmentsForPred[index2].emplace_back(index1, false);
            entailmentsForPred[index1].emplace_back(index2, false);
        } else {
            entailmentsPerNegPred[index1].emplace_back(index2, false); // !P1 entails !P2
            entailmentsPerPred[index2].emplace_back(index1, true); // P2 entails P1
            //
            entailmentsForNegPred[index2].emplace_back(index1, false);
            entailmentsForPred[index1].emplace_back(index2, true);
        }
    }
}

void PredicateRelations::compute_entailments_per_asc_index() {
    PLAJA_ASSERT(entailmentsPerPred.size() == entailmentsPerNegPred.size())
    PLAJA_ASSERT(entailmentsPerPredAscIndex.size() == entailmentsPerNegPredAscIndex.size())
    auto num_predicates = entailmentsPerPred.size();
    PLAJA_ASSERT(entailmentsPerPredAscIndex.size() <= num_predicates)

    // (re-)initialize ASC (incremental usage is too complicated)
    entailmentsPerPredAscIndex.clear();
    entailmentsPerNegPredAscIndex.clear();
    entailmentsPerPredAscIndex.reserve(num_predicates);
    entailmentsPerNegPredAscIndex.reserve(num_predicates);

    // extract ASC
    for (PredicateIndex_type i = 0; i < num_predicates; ++i) {
        // p = true
        const auto end = entailmentsPerPred[i].cend();
        for (auto it = entailmentsPerPred[i].cbegin(); it != end; ++it) {
            if (it->first > i) { // relies on the order in which we compute predicate relations above
                entailmentsPerPredAscIndex.push_back(it);
                break;
            }
        }
        // no entailments for predicates of larger index
        // recall: i is predicate index, hence for m predicates we are at index m-1
        if (entailmentsPerPredAscIndex.size() <= i) { entailmentsPerPredAscIndex.push_back(end); }

        // p = false
        const auto end_neg = entailmentsPerNegPred[i].cend();
        for (auto it = entailmentsPerNegPred[i].cbegin(); it != end_neg; ++it) {
            if (it->first > i) {
                entailmentsPerNegPredAscIndex.push_back(it);
                break;
            }
        }
        // no entailments for predicates of larger index
        if (entailmentsPerNegPredAscIndex.size() <= i) { entailmentsPerNegPredAscIndex.push_back(end_neg); }

    }
}

/** use information ***************************************************************************************************/

bool PredicateRelations::is_entailed(PredicateIndex_type predicate_index, PA::TruthValueType truth_value, const PaStateBase& pa_state) const {
    PLAJA_ASSERT(not PA::is_unknown(truth_value))

    PLAJA_ASSERT(predicate_index < entailmentsForPred.size())
    PLAJA_ASSERT(predicate_index < entailmentsForNegPred.size())

    const auto& entailments_for_pred = truth_value ? entailmentsForPred[predicate_index] : entailmentsForNegPred[predicate_index];
    return std::any_of(entailments_for_pred.cbegin(), entailments_for_pred.cend(), [&pa_state](const std::pair<PredicateIndex_type, bool>& p_index_value) { return pa_state.is_set(p_index_value.first) and pa_state.predicate_value(p_index_value.first) == p_index_value.second; });

}

bool PredicateRelations::is_equivalent_to(PredicateIndex_type pred_index1, PredicateIndex_type pred_index2) const {
    /* Retrieve positive entailment. */
    std::unique_ptr<PA::EntailmentValueType> pos_entailment;
    for (const auto& [index, value]: entailmentsPerPred[pred_index1]) {
        if (index == pred_index2) {
            pos_entailment = std::make_unique<PA::EntailmentValueType>(value);
            break;
        }
    }
    if (not pos_entailment) { return false; }

    /* retrieve negative entailment. */
    std::unique_ptr<PA::EntailmentValueType> neg_entailment;
    for (const auto& [index, value]: entailmentsPerNegPred[pred_index1]) {
        if (index == pred_index2) {
            neg_entailment = std::make_unique<PA::EntailmentValueType>(value);
            break;
        }
    }
    if (not neg_entailment) { return false; }

    // iff P1 entails P2 && !P1 entails !P2 or P1 entails !P2 and !P1 entails P2
    return (*pos_entailment and not(*neg_entailment)) or (not(*pos_entailment) and *neg_entailment);
}

bool PredicateRelations::is_equivalent_to_other(PredicateIndex_type pred_index) const {
    auto num_predicates = entailmentsPerPred.size();
    for (PredicateIndex_type pred_index_other = 0; pred_index_other < num_predicates; ++pred_index_other) {
        if (pred_index_other == pred_index) { continue; }
        if (is_equivalent_to(pred_index, pred_index_other)) { return true; }
    }
    return false;
}

bool PredicateRelations::is_equivalent_to_other_asc(PredicateIndex_type pred_index) const {
    auto num_predicates = entailmentsPerPred.size();
    for (PredicateIndex_type pred_index_other = pred_index + 1; pred_index_other < num_predicates; ++pred_index_other) {
        if (is_equivalent_to(pred_index, pred_index_other)) { return true; }
    }
    return false;
}

bool PredicateRelations::check_equivalence() const {
    auto num_predicates = entailmentsPerPred.size();
    for (PredicateIndex_type pred_index = 0; pred_index < num_predicates; ++pred_index) {
        if (is_equivalent_to_other_asc(pred_index)) { return true; }
    }
    return false;
}

void PredicateRelations::fix_predicate_state(PredicateState& predicate_state) const {
    for (const auto& [pred, val]: groundTruthEntailments) { predicate_state.set_entailed(pred, val); }
}

void PredicateRelations::fix_predicate_state(PredicateState& predicate_state, PredicateIndex_type predicate_index) const {
    PLAJA_ASSERT(predicate_index < entailmentsPerPred.size() && predicate_index < entailmentsPerNegPred.size())
    PLAJA_ASSERT(predicate_state.is_defined(predicate_index))

    STMT_IF_DEBUG(std::list<PredicateIndex_type> successfully_set) // to test whether recursive fixing occurs -> should not due to class-internal invariants (binary predicate relations)

    // entailments by predicate ...
    for (const auto& [pred_index, pred_value]: (predicate_state.predicate_value(predicate_index) ? entailmentsPerPred[predicate_index] : entailmentsPerNegPred[predicate_index])) {
        STMT_IF_DEBUG(if (not predicate_state.is_entailed(pred_index)) { successfully_set.push_back(pred_index); })
        predicate_state.set_entailed(pred_index, pred_value);
    }

    STMT_IF_DEBUG(check_recursive(predicate_state, predicate_index, successfully_set, false)) // check recursive fixing is redundant
}

void PredicateRelations::fix_predicate_state_all(PredicateState& predicate_state) const {

    const PredicatesNumber_type num_preds = predicate_state.get_size_predicates();

    for (PredicateIndex_type pred_index = 0; pred_index < num_preds; ++pred_index) {
        if (predicate_state.is_defined(pred_index)) { fix_predicate_state(predicate_state, pred_index); }
    }

}

void PredicateRelations::fix_predicate_state_asc_index(PredicateState& predicate_state, PredicateIndex_type predicate_index) const {
    PLAJA_ASSERT(predicate_index < entailmentsPerPred.size() && predicate_index < entailmentsPerNegPred.size())
    PLAJA_ASSERT(predicate_state.is_defined(predicate_index))

    STMT_IF_DEBUG(std::list<PredicateIndex_type> successfully_set) // to test whether recursive fixing occurs -> should not due to class-internal invariants (binary predicate relations)

    if (predicate_state.predicate_value(predicate_index)) { // entailments by predicate ...
        const auto end = entailmentsPerPred[predicate_index].cend();
        for (auto it = entailmentsPerPredAscIndex[predicate_index]; it != end; ++it) {
            STMT_IF_DEBUG(if (not predicate_state.is_entailed(it->first)) { successfully_set.push_back(it->first); })
            predicate_state.set_entailed(it->first, it->second);
        }
    } else { // entailments by predicate negation ...
        const auto end = entailmentsPerNegPred[predicate_index].cend();
        for (auto it = entailmentsPerNegPredAscIndex[predicate_index]; it != end; ++it) {
            STMT_IF_DEBUG(if (not predicate_state.is_entailed(it->first)) { successfully_set.push_back(it->first); })
            predicate_state.set_entailed(it->first, it->second);
        }
    }

    STMT_IF_DEBUG(check_recursive(predicate_state, predicate_index, successfully_set, true)) // check recursive fixing is redundant
}

#ifndef NDEBUG

void PredicateRelations::check_recursive(PredicateState& predicate_state, PredicateIndex_type original_index, const std::list<PredicateIndex_type>& newly_set, bool asc_index) const {
    // check for each entailed predicate that all its entailments are already set
    for (const PredicateIndex_type index_newly_set: newly_set) {
        PLAJA_ASSERT(predicate_state.is_defined(index_newly_set))
        // entailments by predicate ...
        for (const auto& pred_index_value: (predicate_state.predicate_value(index_newly_set) ? entailmentsPerPred[index_newly_set] : entailmentsPerNegPred[index_newly_set])) {
            if (pred_index_value.first == original_index) { continue; }
            if (asc_index && pred_index_value.first < original_index) {
                PLAJA_ASSERT(predicate_state.is_defined(pred_index_value.first)) // predicates of smaller index are not set entailed; but are intended to have been set
            } else {
                PLAJA_ASSERT(predicate_state.is_entailed(pred_index_value.first))
            }
        }
    }
}

void PredicateRelations::check_all_entailments(const PredicateState& predicate_state) const {

    // collect predicates set ...
    std::list<PredicateIndex_type> set_preds;
    for (auto it = predicate_state.init_pred_state_iterator(); !it.end(); ++it) { if (it.is_defined()) { set_preds.push_back(it.predicate_index()); } }

    // check all entailments ...
    for (const PredicateIndex_type set_pred: set_preds) {
        // entailments by predicate ...
        for (const auto& [pred, val]: (predicate_state.predicate_value(set_pred) ? entailmentsPerPred[set_pred] : entailmentsPerNegPred[set_pred])) {
            PLAJA_ASSERT(predicate_state.is_entailed(pred))
            PLAJA_ASSERT(predicate_state.predicate_value(pred) == val) // detect out-of-domain assignments
        }
    }

}

#endif

/**********************************************************************************************************************/

bool PredicateRelations::check_if_useful(const Expression& predicate, const PaStateBase* pa_state) const {
    PLAJA_ASSERT(PLAJA_GLOBAL::lazyPA or not pa_state) // NOLINT(cert-dcl03-c,misc-static-assert)
    if (pa_state) { return check_if_useful(predicate, *pa_state); }

    const auto predicate_z3 = modelZ3->to_z3_condition(predicate, false);

    // check if constant under ground truth
    auto ground_truth_entailment = PREDICATE_RELATIONS::check_ground_truth_entailment(*solver, predicate_z3);
    if (ground_truth_entailment) { return false; }

    // check equivalence
    auto num_predicates = modelZ3->get_number_predicates();
    for (PredicateIndex_type index = 0; index < num_predicates; ++index) {
        const auto& predicate_ = modelZ3->get_src_predicate(index);
#if 0
        auto binary_entailment = PREDICATE_RELATIONS::check_binary_relation(*solver, predicate_, predicate_z3);
        const auto& pos_entailment = binary_entailment.first;
        const auto& neg_entailment = binary_entailment.second;
        if (pos_entailment && neg_entailment) {
            // equivalent iff: P1 entails P2 && !P1 entails !P2 or P1 entails !P2 and !P1 entails P2
            if ((*pos_entailment && !*neg_entailment) || (!*pos_entailment && *neg_entailment)) { return false; }
        }
#else
        if (PREDICATE_RELATIONS::check_equivalence(*solver, predicate_, predicate_z3) or PREDICATE_RELATIONS::check_equivalence(*solver, predicate_, !predicate_z3)) { return false; }
#endif
    }

    return true;
}

bool PredicateRelations::check_if_useful(const Expression& predicate, const PaStateBase& pa_state) const {
    PLAJA_ASSERT_EXPECT(PLAJA_GLOBAL::lazyPA) // NOLINT(cert-dcl03-c,misc-static-assert)

    const auto predicate_z3 = modelZ3->to_z3_condition(predicate, false);

    // check if constant under ground truth
    auto ground_truth_entailment = PREDICATE_RELATIONS::check_ground_truth_entailment(*solver, predicate_z3);
    if (ground_truth_entailment) { return false; }

    // check equivalence
    auto num_predicates = modelZ3->get_number_predicates();
    for (PredicateIndex_type index = 0; index < num_predicates; ++index) {
        const auto& predicate_old = modelZ3->get_src_predicate(index);

        if (index >= pa_state.get_size_predicates() or not pa_state.is_set(index)) { continue; }

        /* We have a specific truth value for index, so the new predicate is useless if it is entailed by that truth value already. */
        if (PREDICATE_RELATIONS::check_entailment(*solver, pa_state.predicate_value(index) ? predicate_old : !predicate_old, predicate_z3)) { return false; }

    }

    return true;
}

bool PredicateRelations::check_if_useful(const Expression& predicate, const std::list<PredicateIndex_type>& preds_already_used) const {
    PLAJA_ASSERT_EXPECT(PLAJA_GLOBAL::lazyPA) // NOLINT(cert-dcl03-c,misc-static-assert)

    const auto predicate_z3 = modelZ3->to_z3_condition(predicate, false);

    // check if constant under ground truth
    auto ground_truth_entailment = PREDICATE_RELATIONS::check_ground_truth_entailment(*solver, predicate_z3);
    if (ground_truth_entailment) { return false; }

    // check equivalence
    return std::all_of(preds_already_used.cbegin(), preds_already_used.cend(), [this, &predicate_z3](const PredicateIndex_type index) {
        const auto& predicate_old = modelZ3->get_src_predicate(index);
        return not PREDICATE_RELATIONS::check_equivalence(*solver, predicate_old, predicate_z3) and not PREDICATE_RELATIONS::check_equivalence(*solver, predicate_old, !predicate_z3);
    });
}

bool PredicateRelations::check_equivalence(const Expression& pred1, const Expression& pred2) const {
    auto pred1_z3 = modelZ3->to_z3_condition(pred1, false);
    auto pred2_z3 = modelZ3->to_z3_condition(pred2, false);

    return PREDICATE_RELATIONS::check_equivalence(*solver, pred1_z3, pred2_z3) or PREDICATE_RELATIONS::check_equivalence(*solver, pred1_z3, !pred2_z3);
}