//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PREDICATE_ABSTRACTION_TEST_H
#define PLAJA_PREDICATE_ABSTRACTION_TEST_H

#include <memory>
#include "../../parser/ast/expression/non_standard/predicate_abstraction_expression.h"
#include "../../parser/ast/automaton.h"
#include "../../parser/ast/edge.h"
#include "../../parser/ast/model.h"
#include "../../parser/visitor/to_normalform.h"
#include "../../parser/visitor/semantics_checker.h"
#include "../../search/factories/predicate_abstraction/pa_factory.h"
#include "../../search/factories/predicate_abstraction/pa_options.h"
#include "../../search/factories/predicate_abstraction/search_engine_config_pa.h"
#include "../../search/information/property_information.h"
#include "../../search/predicate_abstraction/cegar/pa_cegar.h"
#include "../../search/predicate_abstraction/cegar/predicates_structure.h"
#include "../../stats/stats_base.h"
#include "../utils/test_header.h"

const std::string filename1("../../../tests/test_instances/for_pa_cegar.jani"); // NOLINT(*-err58-cpp)
const std::string filename2("../../../tests/test_instances/for_pa_cegar2.jani"); // NOLINT(*-err58-cpp)
const std::string filename3("../../../tests/test_instances/for_pa_cegar3.jani"); // NOLINT(*-err58-cpp)

class PACegarTest: public PLAJA_TEST::TestSuite {

private:
    PLAJA::ModelConstructorForTests modelConstructor1;
    PLAJA::ModelConstructorForTests modelConstructor2;
    PLAJA::ModelConstructorForTests modelConstructor3;
    std::unique_ptr<Model> model4;

    // aux:

    static std::unique_ptr<PACegar> construct_cegar_instance(PLAJA::ModelConstructorForTests& constructor, std::size_t prop_index) {
        return std::unique_ptr<PACegar>(PLAJA_UTILS::cast_unique<PACegar>(constructor.construct_instance(SearchEngineFactory::PaCegarType, prop_index)));
    }

    static std::list<std::unique_ptr<Expression>> parse_and_standardize_pred(const std::string& exp_str, const PLAJA::ModelConstructorForTests& model_constructor) {
        return TO_NORMALFORM::normalize_and_split(model_constructor.parse_exp_str(exp_str), true);
    }

    std::unique_ptr<Expression> parse_and_standardize_singleton_pred(const std::string& exp_str, const PLAJA::ModelConstructorForTests& model_constructor) {
        auto pred_splits = parse_and_standardize_pred(exp_str, model_constructor);
        TS_ASSERT(pred_splits.size() == 1)
        return std::move(pred_splits.front());
    }

    inline static unsigned int iterations(const PACegar& engine) { return engine.searchStatistics->get_attr_unsigned(PLAJA::StatsUnsigned::ITERATIONS); }

    inline static unsigned int iterations(const std::unique_ptr<PACegar>& engine) { return iterations(*engine); }

public:

    PACegarTest() {
        // parse test Model1
        modelConstructor1.get_config().set_bool_option(PLAJA_OPTION::exclude_entailments, false);
        // TODO mt is not loc-aware
        modelConstructor1.get_config().set_bool_option(PLAJA_OPTION::incremental_search, false);
        // TODO due to locations
        STMT_IF_LAZY_PA(modelConstructor1.get_config().set_bool_option(PLAJA_OPTION::lazy_pa_state, false);)
        STMT_IF_LAZY_PA(modelConstructor1.get_config().set_bool_option(PLAJA_OPTION::lazy_pa_app, false);)
        STMT_IF_LAZY_PA(modelConstructor1.get_config().set_bool_option(PLAJA_OPTION::lazy_pa_sel, false);)
        STMT_IF_LAZY_PA(modelConstructor1.get_config().set_bool_option(PLAJA_OPTION::lazy_pa_target, false);)
        modelConstructor1.construct_model(filename1);
        // parse test Model2
        modelConstructor2.get_config().set_bool_option(PLAJA_OPTION::exclude_entailments, false);
        modelConstructor2.get_config().set_bool_option(PLAJA_OPTION::incremental_search, false);
        STMT_IF_LAZY_PA(modelConstructor1.get_config().set_bool_option(PLAJA_OPTION::lazy_pa_state, false);)
        STMT_IF_LAZY_PA(modelConstructor1.get_config().set_bool_option(PLAJA_OPTION::lazy_pa_app, false);)
        STMT_IF_LAZY_PA(modelConstructor1.get_config().set_bool_option(PLAJA_OPTION::lazy_pa_sel, false);)
        STMT_IF_LAZY_PA(modelConstructor1.get_config().set_bool_option(PLAJA_OPTION::lazy_pa_target, false);)
        modelConstructor2.construct_model(filename2);
        // parse test Model3
        modelConstructor3.get_config().set_bool_option(PLAJA_OPTION::exclude_entailments, false);
        modelConstructor3.get_config().set_bool_option(PLAJA_OPTION::incremental_search, false);
        STMT_IF_LAZY_PA(modelConstructor1.get_config().set_bool_option(PLAJA_OPTION::lazy_pa_state, false);)
        STMT_IF_LAZY_PA(modelConstructor1.get_config().set_bool_option(PLAJA_OPTION::lazy_pa_app, false);)
        STMT_IF_LAZY_PA(modelConstructor1.get_config().set_bool_option(PLAJA_OPTION::lazy_pa_sel, false);)
        STMT_IF_LAZY_PA(modelConstructor1.get_config().set_bool_option(PLAJA_OPTION::lazy_pa_target, false);)
        modelConstructor3.construct_model(filename3);

        model4 = modelConstructor3.constructedModel->deep_copy();
        model4->get_automatonInstance(0)->get_edge(0)->set_guardExpression(modelConstructor3.parse_exp_str(R"({"op": "<", "left":"x", "right":6})"));
        SemanticsChecker::check_semantics(model4.get());
        model4->compute_model_information();

        // set flags (actually last constructor should suffice)
        for (auto& mc: { &modelConstructor1, &modelConstructor2, &modelConstructor3 }) {
            mc->get_config().set_flag(PLAJA_OPTION::add_guards, true);
            mc->get_config().set_flag(PLAJA_OPTION::add_predicates, true);
        }
    }

    ~PACegarTest() override = default;

    DELETE_CONSTRUCTOR(PACegarTest)

/**********************************************************************************************************************/

    // Tests on Model1

    void testPACegarEmptyPath() {
        std::cout << std::endl << "Test PA CEGAR in case concrete initial state is a goal state:" << std::endl;
        PLAJA_GLOBAL::currentModel = modelConstructor1.constructedModel.get(); // just for completeness (only needed for runtime check on concrete array access)
        std::unique_ptr<PACegar> cegar = construct_cegar_instance(modelConstructor1, 0);
        cegar->search();

        TS_ASSERT(iterations(cegar) == 1)
        TS_ASSERT(cegar->predicates->size() == 4)

        const auto split_preds1 = parse_and_standardize_pred(R"({"op":"∧","left":{"op":"=","left":"x","right":3},"right":{"op":"=","left":"y","right":3}})", modelConstructor1);
        TS_ASSERT(split_preds1.size() == 4)
        for (const auto& split_pred: split_preds1) { TS_ASSERT(cegar->predicates->contains(split_pred.get())) }
    }

    void testPACegarEmptyPathSolvable() {
        std::cout << std::endl << "Test PA CEGAR in case concrete initial state is not a goal state but there are no initial predicates:" << std::endl;
        PLAJA_GLOBAL::currentModel = modelConstructor1.constructedModel.get();
        std::unique_ptr<PACegar> cegar = construct_cegar_instance(modelConstructor1, 1);
        cegar->search();

        TS_ASSERT(iterations(cegar) == 1)
        TS_ASSERT(cegar->predicates->size() == 3)

        const auto split_preds1 = parse_and_standardize_pred(R"({"op":"∧","left":{"op":"=","left":"x","right":5},"right":{"op":"=","left":"y","right":4}})", modelConstructor1);
        TS_ASSERT(split_preds1.size() == 4)
        // for (const auto& split_pred: split_preds1) { TS_ASSERT(cegar->predicateSet.count(split_pred.get())) }
        auto pred11 = modelConstructor1.parse_exp_str(R"({"op":"≥","left":"x","right":5})");
        auto pred12_1 = modelConstructor1.parse_exp_str(R"({"op":"≤","left":"x","right":5})"); // standardized to:
        auto pred12_2 = modelConstructor1.parse_exp_str(R"({"op":"≥","left":"x","right":6})");
        auto pred13 = modelConstructor1.parse_exp_str(R"({"op":"≥","left":"y","right":4})");
        auto pred14_1 = modelConstructor1.parse_exp_str(R"({"op":"≤","left":"y","right":4})"); // standardized to:
        auto pred14_2 = modelConstructor1.parse_exp_str(R"({"op":"≥","left":"y","right":5})");
        TS_ASSERT(cegar->predicates->contains(pred11.get()))
        TS_ASSERT(!cegar->predicates->contains(pred12_1.get())) // standardized to:
        TS_ASSERT(!cegar->predicates->contains(pred12_2.get())) // ground truth
        TS_ASSERT(cegar->predicates->contains(pred13.get()))
        TS_ASSERT(!cegar->predicates->contains(pred14_1.get())) // standardized to:
        TS_ASSERT(cegar->predicates->contains(pred14_2.get()))
    }

    void testPACegarEmptyPathUnsolvable() {
        std::cout << std::endl << "Test PA CEGAR in case no goal state is reachable but there are no initial predicates:" << std::endl;
        PLAJA_GLOBAL::currentModel = modelConstructor1.constructedModel.get();
        std::unique_ptr<PACegar> cegar = construct_cegar_instance(modelConstructor1, 2);
        cegar->search();

        TS_ASSERT(iterations(cegar) == 1)
        TS_ASSERT(cegar->predicates->size() == 2)

        const auto split_preds1 = parse_and_standardize_pred(R"({"op":"∧","left":{"op":"≥","left":"x","right":5},"right":{"op":"≥","left":"y","right":5}})", modelConstructor1);
        TS_ASSERT(split_preds1.size() == 2) // not x,y <= 5 as ground truth (see preceding test)
        for (const auto& split_pred: split_preds1) { TS_ASSERT(cegar->predicates->contains(split_pred.get())) }
    }

    // Tests on Model2

    void testPACegarParallelRefinement() {
        std::cout << std::endl << "Test PA CEGAR on refining non-disjoint variable sets at parallel:" << std::endl;
        PLAJA_GLOBAL::currentModel = modelConstructor2.constructedModel.get();
        std::unique_ptr<PACegar> cegar = construct_cegar_instance(modelConstructor2, 0);
        cegar->search();

        TS_ASSERT(iterations(cegar) == 2)
        TS_ASSERT(cegar->predicates->size() == 5)

        // check predicates
        // initial predicates
        const auto split_preds1 = parse_and_standardize_pred(R"({"op":"∧", "left":{"op":"=", "left":"x", "right":5}, "right":"swapped"})", modelConstructor2);
        TS_ASSERT(split_preds1.size() == 3) // this predicate is added initially; but also as a guard
        // for (const auto& split_pred: split_preds1) { TS_ASSERT(cegar->predicateSet.count(split_pred.get())) }
        auto pred11 = modelConstructor2.parse_exp_str(R"({"left":"x","op":"≥","right":5})");
        auto pred12 = parse_and_standardize_singleton_pred(R"({"left":"x","op":"≤","right":5})", modelConstructor2);
        auto pred13 = modelConstructor2.parse_exp_str(R"("swapped")");
        TS_ASSERT(cegar->predicates->contains(pred11.get()))
        TS_ASSERT(!cegar->predicates->contains(pred12.get())) // ground truth
        TS_ASSERT(cegar->predicates->contains(pred13.get()))
        //
        auto pred2 = parse_and_standardize_singleton_pred(R"({"op":"≤", "left": {"op":"+", "left":"x", "right":"y"}, "right":10})", modelConstructor2);
        TS_ASSERT(cegar->predicates->contains(pred2.get()))
        auto pred3 = modelConstructor2.parse_exp_str(R"({"op":"¬", "exp":"swapped"})");
        TS_ASSERT(!cegar->predicates->contains(pred3.get())) // equivalence
        // check added predicate
        // guard (sub)
        const auto split_preds4 = parse_and_standardize_pred(R"({"left":{"left":"x","op":"+","right":"y"},"op":"=","right":10})", modelConstructor2);
        TS_ASSERT(split_preds4.size() == 2)
        // "<=" is not violated, thus not considered, but it is already present anyway (see above); moreover:
        std::unique_ptr<Expression> pred4 = parse_and_standardize_singleton_pred(R"({"left":{"left":"x","op":"+","right":"y"},"op":"≥","right":10})", modelConstructor2);
        TS_ASSERT(cegar->predicates->contains(pred4.get())) // due to standardization,
        // predicate sub
        auto pred5 = modelConstructor2.parse_exp_str(R"({"left":"y","op":"≥","right":5})");
        TS_ASSERT(cegar->predicates->contains(pred5.get())) // substitution
    }

    void testPACegarParallelRefinement2() {
        std::cout << std::endl << "Test PA CEGAR on refining non-disjoint variable sets at parallel with conjunction predicate:" << std::endl;
        PLAJA_GLOBAL::currentModel = modelConstructor2.constructedModel.get();
        std::unique_ptr<PACegar> cegar = construct_cegar_instance(modelConstructor2, 1);
        cegar->search();

        TS_ASSERT(iterations(cegar) == 2)
        TS_ASSERT(cegar->predicates->size() == 6) // one guard split is not added as fulfilled

        // check predicates
        // initial predicates
        const auto split_preds1 = parse_and_standardize_pred(R"({"op":"∧", "left":{"op":"=", "left":"x", "right":5}, "right":"swapped"})", modelConstructor2);
        TS_ASSERT(split_preds1.size() == 3) // this predicate is added initially; but also as a guard
        // for (const auto& split_pred: split_preds1) { TS_ASSERT(cegar->predicateSet.count(split_pred.get())) }
        auto pred11 = modelConstructor2.parse_exp_str(R"({"left":"x","op":"≥","right":5})");
        auto pred12 = parse_and_standardize_singleton_pred(R"({"left":"x","op":"≤","right":5})", modelConstructor2);
        auto pred13 = modelConstructor2.parse_exp_str(R"("swapped")");
        TS_ASSERT(cegar->predicates->contains(pred11.get()))
        TS_ASSERT(!cegar->predicates->contains(pred12.get())) // ground truth
        TS_ASSERT(cegar->predicates->contains(pred13.get()))
        //
        const auto split_preds2 = parse_and_standardize_pred(R"({"op":"∧", "left":{"op":"≤", "left": {"op":"+", "left": {"op":"*", "left":2, "right":"x"}, "right":"y"}, "right":10}, "right":"swapped"})", modelConstructor2);
        TS_ASSERT(split_preds2.size() == 2)
        for (const auto& split_pred: split_preds2) { TS_ASSERT(cegar->predicates->contains(split_pred.get())) }
        // check added predicate
        // guard (sub)
        const auto split_preds3 = parse_and_standardize_pred(R"({"left":{"left":"x","op":"+","right":"y"},"op":"=","right":10})", modelConstructor2); // guard
        TS_ASSERT(split_preds3.size() == 2)
        auto pred31 = parse_and_standardize_singleton_pred(R"({"left":{"left":"x","op":"+","right":"y"},"op":"≥","right":10})", modelConstructor2);
        auto pred32 = parse_and_standardize_singleton_pred(R"({"left":{"left":"x","op":"+","right":"y"},"op":"≤","right":10})", modelConstructor2);
        TS_ASSERT(cegar->predicates->contains(pred31.get())) // due to standardization
        TS_ASSERT(not cegar->predicates->contains(pred32.get())) // not violated (thus not considered)
        auto pred31_sub = modelConstructor2.parse_exp_str(R"({"left":{"left":"y","op":"+","right":"x"},"op":"≥","right":10})");
        auto pred32_sub = parse_and_standardize_singleton_pred(R"({"left":{"left":"y","op":"+","right":"x"},"op":"≤","right":10})", modelConstructor2);
        TS_ASSERT(not cegar->predicates->contains(pred31_sub.get())) // // due to standardization
        TS_ASSERT(not cegar->predicates->contains(pred32_sub.get())) // pre-sub not violated, thus not considered
        //
        auto pred4 = modelConstructor2.parse_exp_str(R"({"left":"y","op":"≥","right":5})"); // substitution
        TS_ASSERT(cegar->predicates->contains(pred4.get()))
        auto pred5 = parse_and_standardize_singleton_pred(R"({"op":"≤", "left": {"op":"+", "left": {"op":"*", "left":2, "right":"y"}, "right":"x"}, "right":10})", modelConstructor2); // substitution (x -> y, y -> x) (*negation* due to abstract state value is ignored as added internally)
        TS_ASSERT(cegar->predicates->contains(pred5.get()))
    }

    // Test Model 3

    void testPACegarFirstTransitionSpurious() {
        std::cout << std::endl << "Test PA CEGAR where the first transition is spurious:" << std::endl;
        PLAJA_GLOBAL::currentModel = modelConstructor3.constructedModel.get();
        std::unique_ptr<PACegar> cegar = construct_cegar_instance(modelConstructor3, 0);
        cegar->search();

        TS_ASSERT(iterations(cegar) == 2)
        TS_ASSERT(cegar->predicates->size() == 4)

        // check predicates
        auto split_preds1 = parse_and_standardize_pred(R"({"op":"=", "left":"x", "right":6})", modelConstructor3);
        TS_ASSERT(split_preds1.size() == 2)
        for (const auto& split_pred: split_preds1) { TS_ASSERT(cegar->predicates->contains(split_pred.get())) }
        std::unique_ptr<Expression> pred11 = modelConstructor3.parse_exp_str(R"({"left":"x", "op":"≥", "right":6})");
        std::unique_ptr<Expression> pred12 = parse_and_standardize_singleton_pred(R"({"left":"x", "op":"≤", "right":6})", modelConstructor3);
        TS_ASSERT(cegar->predicates->contains(pred11.get()))
        TS_ASSERT(cegar->predicates->contains(pred12.get()))
        //
        std::unique_ptr<Expression> pred2 = parse_and_standardize_singleton_pred(R"({"op":"<", "left":{"op":"+", "left":"x", "right":"y"}, "right":6})", modelConstructor3);
        TS_ASSERT(cegar->predicates->contains(pred2.get()))
        // check added predicates
        std::unique_ptr<Expression> pred3 = parse_and_standardize_singleton_pred(R"({"left":{"left":"x","op":"+","right":"x"},"op":"<","right":6})", modelConstructor3); // substitution y -> x
        TS_ASSERT(cegar->predicates->contains(pred3.get()))
    }

    // Test Model 4

    void testPACegarSequentialRefinement() {
        std::cout << std::endl << "Test PA CEGAR on sequential refinement:" << std::endl;
        PLAJA_GLOBAL::currentModel = model4.get();
        PACegarFactory factory;
        const auto property_info = PropertyInformation::analyse_property(modelConstructor3.config, *model4->get_property(1), *model4);
        factory.supports_property(*property_info);
        SearchEngineConfigPA config(modelConstructor3.optionParser);
        config.set_sharable_const(PLAJA::SharableKey::MODEL, model4.get());
        config.set_sharable_const(PLAJA::SharableKey::PROP_INFO, property_info.get());
        PACegar cegar(config);
        cegar.search();

        TS_ASSERT(iterations(cegar) == 2)
        TS_ASSERT(cegar.predicates->size() == 10) // 8 + 3 due to "==" splitting, - 1 due to equivalence checks (see below)

        // check predicates
        std::unique_ptr<Expression> pred1 = parse_and_standardize_singleton_pred(R"({"op":"<", "left":{"op":"+", "left":"x", "right":"y"}, "right":6})", modelConstructor3);
        TS_ASSERT(cegar.predicates->contains(pred1.get()))
        const auto split_preds2 = parse_and_standardize_pred(R"({"op":"=", "left":"x", "right":6})", modelConstructor3);
        for (const auto& split_pred: split_preds2) { TS_ASSERT(cegar.predicates->contains(split_pred.get())) }
        // check added predicates
        auto pred3 = parse_and_standardize_singleton_pred(R"({"op":"<", "left":{"op":"+", "left":{"op":"+", "left":2, "right":"y"}, "right":"y"}, "right":6})", modelConstructor3); // substitution (x -> 2 + y) (*negation* due to abstract state value is ignored as added internally)
        TS_ASSERT(cegar.predicates->contains(pred3.get()))
        const auto split_preds4 = parse_and_standardize_pred(R"({"left":{"left":"y","op":"+","right":2},"op":"=","right":6})", modelConstructor3); // substitution (x -> 2 + y)
        TS_ASSERT(split_preds4.size() == 2)
        for (const auto& split_pred: split_preds4) { TS_ASSERT(cegar.predicates->contains(split_pred.get())) }

        std::unique_ptr<Expression> pred5 = parse_and_standardize_singleton_pred(R"({"left":{"left":"x","op":"+","right":"x"},"op":"<","right":6})", modelConstructor3); // substitution (y -> x)
        TS_ASSERT(cegar.predicates->contains(pred5.get()))
        std::unique_ptr<Expression> pred6 = parse_and_standardize_singleton_pred(R"({"op":"<", "left":{"op":"+", "left":{"op":"+", "left":2, "right":"x"}, "right":"x"}, "right":6})", modelConstructor3); // substitution (y -> x)
        TS_ASSERT(cegar.predicates->contains(pred6.get()))
        const auto split_preds7 = parse_and_standardize_pred(R"({"left":{"left":"x","op":"+","right":2},"op":"=","right":6})", modelConstructor3); // substitution (y -> x)
        TS_ASSERT(split_preds7.size() == 2)
        for (const auto& split_pred: split_preds7) { TS_ASSERT(cegar.predicates->contains(split_pred.get())) }

        // {"left":"x","op":"<","right":6} -> x >= 6 (which is already present, pred2)
        auto guard_split = TO_NORMALFORM::normalize_and_split(model4->get_automaton(0)->get_edge(0)->get_guardExpression()->deepCopy_Exp(), true);
        TS_ASSERT(guard_split.size() == 1)
        TS_ASSERT(cegar.predicates->contains(guard_split.front().get()))
    }

/**********************************************************************************************************************/

    void run_tests() override {
        RUN_TEST(testPACegarEmptyPath())
        RUN_TEST(testPACegarEmptyPathSolvable())
        RUN_TEST(testPACegarEmptyPathUnsolvable())
        RUN_TEST(testPACegarParallelRefinement())
        RUN_TEST(testPACegarParallelRefinement2())
        RUN_TEST(testPACegarFirstTransitionSpurious())
        RUN_TEST(testPACegarSequentialRefinement())
    }

};

TEST_MAIN(PACegarTest)

#endif //PLAJA_PREDICATE_ABSTRACTION_TEST_H
