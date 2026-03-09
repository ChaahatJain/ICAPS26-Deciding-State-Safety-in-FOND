//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2024) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PREDICATE_DEPENDENCY_GRAPH_TEST_H
#define PLAJA_PREDICATE_DEPENDENCY_GRAPH_TEST_H

#include <algorithm>
#include <memory>
#include "../../option_parser/option_parser.h"
#include "../../option_parser/plaja_options.h"
#include "../../parser/ast/expression/non_standard/predicate_abstraction_expression.h"
#include "../../parser/ast/expression/non_standard/predicates_expression.h"
#include "../../parser/ast/model.h"
#include "../../parser/ast/property.h"
#include "../../parser/ast/variable_declaration.h"
#include "../../search/predicate_abstraction/smt/model_z3_pa.h"
#include "../../search/predicate_abstraction/predicate_dependency_graph.h"
#include "../utils/test_header.h"
#include "../utils/test_parser.h"

extern bool allowTrivialPreds;

class PredicateDependencyGraphTest: public PLAJA_TEST::TestSuite {

private:
    PLAJA::ModelConstructorForTests modelConstructorBlocks;
    PLAJA::ModelConstructorForTests modelConstructorPuzzle;

public:

    PredicateDependencyGraphTest() {
        OPTION_PARSER::set_suppress_known_option_check(true);
        allowTrivialPreds = true;

        /* Blocks. */

        modelConstructorBlocks.set_constant("failing_probability", "0");
        modelConstructorBlocks.set_constant("item_cost_bound", "1");
        modelConstructorBlocks.get_config().set_value_option(PLAJA_OPTION::additional_properties, "../../../tests/test_instances/blocksworld/additional_properties/safety_verification/blocksworld_4_3/compact_starts_cost_predicates_afterwards/pa_compact_starts_cost_predicates_afterwards_blocksworld_4_3.jani");

        modelConstructorBlocks.construct_model("../../../tests/test_instances/blocksworld/cost_terminal/models/legacy_enc/blocksworld_4_3.jani");
        modelConstructorBlocks.load_property(2);
        modelConstructorBlocks.get_config().set_sharable_const(PLAJA::SharableKey::PROP_INFO, modelConstructorBlocks.cachedPropertyInfo.get());

        /* Puzzle. */

        modelConstructorPuzzle.set_constant("failing_probability", "0");
        modelConstructorPuzzle.set_constant("item_cost_bound", "1");
        modelConstructorPuzzle.set_constant("cost_terminal", "false");
        modelConstructorPuzzle.get_config().set_value_option(PLAJA_OPTION::additional_properties, "../../../tests/test_instances/npuzzle/additional_properties/safety_verification/preds_lab_id_1.jani");

        modelConstructorPuzzle.construct_model("../../../tests/test_instances/npuzzle/compact_starts_non_terminal/models/offset_enc/npuzzle_3_3.jani");
        modelConstructorPuzzle.load_property(1);
        modelConstructorPuzzle.get_config().set_sharable_const(PLAJA::SharableKey::PROP_INFO, modelConstructorPuzzle.cachedPropertyInfo.get());

    }

    ~PredicateDependencyGraphTest() override = default;

    DELETE_CONSTRUCTOR(PredicateDependencyGraphTest)

    void test_blocks() {

        const auto model_smt = std::make_unique<ModelZ3PA>(modelConstructorBlocks.get_config());
        const auto pred_dep_graph = std::make_unique<PredicateDependencyGraph>(*model_smt);

        /* Check structure. */

        TS_ASSERT_EQUALS(pred_dep_graph->varsPerPred.size(), 85)
        TS_ASSERT_EQUALS(pred_dep_graph->predsPerVar.size(), 14)

        for (const auto& vars_per_pred: pred_dep_graph->varsPerPred) { TS_ASSERT_EQUALS(vars_per_pred.size(), 1) }

        const auto& model = modelConstructorBlocks.constructedModel;

        for (const auto& [var, preds_per_var]: pred_dep_graph->predsPerVar) {
            const auto* var_decl = model->get_variable_by_id(var);

            if (PLAJA_UTILS::contains(var_decl->get_name(), "cost")) {
                TS_ASSERT_EQUALS(preds_per_var.size(), 15)
            } else if (PLAJA_UTILS::contains(var_decl->get_name(), "block")) {
                TS_ASSERT_EQUALS(preds_per_var.size(), 4)
            } else if (PLAJA_UTILS::contains(var_decl->get_name(), "table-counter")) {
                TS_ASSERT_EQUALS(preds_per_var.size(), 4)
            } else if (PLAJA_UTILS::contains(var_decl->get_name(), "hand-empty") or PLAJA_UTILS::contains(var_decl->get_name(), "clear")) {
                TS_ASSERT_EQUALS(preds_per_var.size(), 1)
            } else {
                TS_ASSERT(false)
            }

        }

        /* Check interface. */
        for (PredicateIndex_type pred = 0; pred < model_smt->get_number_predicates(); ++pred) {

            const auto var_closure = pred_dep_graph->compute_variable_closure(pred);

            TS_ASSERT_EQUALS(var_closure.size(), 1)

            const auto* var_decl = model->get_variable_by_id(*var_closure.cbegin());

            const auto dependent_preds = pred_dep_graph->compute_dependent_predicates(pred);

            if (PLAJA_UTILS::contains(var_decl->get_name(), "cost")) {
                TS_ASSERT_EQUALS(dependent_preds.size(), 15)
            } else if (PLAJA_UTILS::contains(var_decl->get_name(), "block")) {
                TS_ASSERT_EQUALS(dependent_preds.size(), 4)
            } else if (PLAJA_UTILS::contains(var_decl->get_name(), "table-counter")) {
                TS_ASSERT_EQUALS(dependent_preds.size(), 4)
            } else if (PLAJA_UTILS::contains(var_decl->get_name(), "hand-empty") or PLAJA_UTILS::contains(var_decl->get_name(), "clear")) {
                TS_ASSERT_EQUALS(dependent_preds.size(), 1)
            } else {
                TS_ASSERT(false)
            }
        }

        /* Check increment. */

        const auto* predicates_const_view = PLAJA_UTILS::cast_ptr<PAExpression>(modelConstructorBlocks.constructedModel->get_property(2)->get_propertyExpression())->get_predicates();
        TS_ASSERT_EQUALS(predicates_const_view->get_number_predicates(), 85)

        auto* predicates = const_cast<PredicatesExpression*>(predicates_const_view);
        predicates->add_predicate(modelConstructorBlocks.parse_exp_str(R"({"op":"=", "left":"clear_0", "right": "clear_1"})"));
        predicates->add_predicate(modelConstructorBlocks.parse_exp_str(R"({"op":"=", "left":"clear_2", "right": "clear_3"})"));

        model_smt->increment();
        pred_dep_graph->increment();

        TS_ASSERT_EQUALS(pred_dep_graph->varsPerPred.size(), 87)
        TS_ASSERT_EQUALS(pred_dep_graph->varsPerPred[85].size(), 2)
        TS_ASSERT_EQUALS(pred_dep_graph->varsPerPred[86].size(), 2)

    }

    void test_puzzle() {

        const auto model_smt = std::make_unique<ModelZ3>(modelConstructorPuzzle.get_config());
        const auto pred_dep_graph = std::make_unique<PredicateDependencyGraph>(*model_smt);

        /* Check structure. */

        TS_ASSERT_EQUALS(pred_dep_graph->varsPerPred.size(), 53)
        TS_ASSERT_EQUALS(pred_dep_graph->predsPerVar.size(), 17)

        for (PredicateIndex_type pred = 0; pred < pred_dep_graph->varsPerPred.size(); ++pred) {
            TS_ASSERT_EQUALS(pred_dep_graph->varsPerPred[pred].size(), (48 <= pred and pred <= 50) ? 2 : 1)
        }

        /* Check interface. */

        const auto& model = modelConstructorPuzzle.constructedModel;
        const auto& empty_var = model->get_variable_by_name("empty");
        const auto& tile_6 = model->get_variable_by_name("tile_6");
        const auto& tile_8 = model->get_variable_by_name("tile_8");
        std::unordered_set<VariableID_type> non_trivial_closure { empty_var.get_id(), tile_6.get_id(), tile_8.get_id() };

        /* Iterate. */
        for (PredicateIndex_type pred = 0; pred < model_smt->get_number_predicates(); ++pred) {

            const auto var_closure = pred_dep_graph->compute_variable_closure(pred);

            if (std::any_of(var_closure.begin(), var_closure.end(), [&non_trivial_closure](VariableID_type var) { return non_trivial_closure.count(var); })) {
                TS_ASSERT_EQUALS(var_closure.size(), 3)
                TS_ASSERT_EQUALS(pred_dep_graph->compute_dependent_predicates(pred).size(), 3 + 2 + 6 + 5)
            } else {
                TS_ASSERT_EQUALS(var_closure.size(), 1)
            }

        }

        /* Case of particular interest. */

        std::unordered_set<VariableID_type> var_closure;
        pred_dep_graph->compute_variable_closure(var_closure, 48);
        TS_ASSERT_EQUALS(var_closure.size(), 3)
        pred_dep_graph->compute_variable_closure(var_closure, 49);
        pred_dep_graph->compute_variable_closure(var_closure, 50);
        TS_ASSERT_EQUALS(var_closure.size(), 3)

        pred_dep_graph->compute_variable_closure(var_closure, 0);
        TS_ASSERT_EQUALS(var_closure.size(), 4)

    }

/**********************************************************************************************************************/

    void run_tests() override {
        RUN_TEST(test_blocks())
        RUN_TEST(test_puzzle())
    }

};

TEST_MAIN(PredicateDependencyGraphTest)

#endif //PLAJA_PREDICATE_DEPENDENCY_GRAPH_TEST_H
