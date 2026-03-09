//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_TESTS_MODEL_Z3_TEST_CPP
#define PLAJA_TESTS_MODEL_Z3_TEST_CPP

#include "../../../parser/ast/model.h"
#include "../../../parser/visitor/extern/ast_specialization.h"
#include "../../../parser/visitor/extern/to_normalform.h"
#include "../../../search/fd_adaptions/timer.h"
#include "../../../search/information/property_information.h"
#include "../../../search/smt_nn/solver/smt_solver_marabou.h"
#include "../../../search/smt_nn/visitor/extern/to_marabou_visitor.h"
#include "../../../search/smt_nn/constraints_in_marabou.h"
#include "../../../search/smt_nn/marabou_context.h"
#include "../../../search/smt_nn/vars_in_marabou.h"
#include "../../utils/test_header.h"

const std::string filename("../../../../tests/test_instances/blocksworld/cost_terminal/models/legacy_enc/blocksworld_8_7.jani"); // NOLINT(*-err58-cpp)

/**
 * Stress test on generation of large constraint in Marabou.
 */
class ToMarabouLargeTest: public PLAJA_TEST::TestSuite {

private:
    PLAJA::ModelConstructorForTests modelConstructor;
    std::unique_ptr<PropertyInformation> propInfo;

    std::shared_ptr<MARABOU_IN_PLAJA::Context> context;
    std::unique_ptr<StateIndexesInMarabou> stateIndexesMapping;
    std::unique_ptr<MARABOU_IN_PLAJA::SMTSolver> solver;

public:

    ToMarabouLargeTest() {
        modelConstructor.set_constant("failing_probability", std::to_string(0));
        modelConstructor.set_constant("item_cost_bound", std::to_string(100)); // NOLINT(*-avoid-magic-numbers)

        modelConstructor.construct_model(filename);
        modelConstructor.construct_instance(SearchEngineFactory::PaCegarType, 0); // dummy to add options

        propInfo = modelConstructor.analyze_property(0);

        context = std::make_shared<MARABOU_IN_PLAJA::Context>();
        stateIndexesMapping = std::make_unique<StateIndexesInMarabou>(modelConstructor.constructedModel->get_model_information(), *context);
        stateIndexesMapping->init(false, true);

        const PLAJA::Configuration config;
        solver = MARABOU_IN_PLAJA::SMT_SOLVER::construct(config, std::shared_ptr<MARABOU_IN_PLAJA::Context>(context));
    }

    ~ToMarabouLargeTest() override = default;

    DELETE_CONSTRUCTOR(ToMarabouLargeTest)

/**********************************************************************************************************************/

    void test_to_marabou_large() {
        PLAJA_LOG(PLAJA_UTILS::lineBreakString + "Test to Marabou Large:")

        const auto time = PLAJA_GLOBAL::timer->operator()();

        auto start = propInfo->get_start()->deepCopy_Exp();
        start = TO_NORMALFORM::normalize(*start);

        auto conjuncts = TO_NORMALFORM::split_conjunction(std::move(start), false);

        std::list<std::unique_ptr<Expression>> conjuncts_processed;

        std::vector<std::size_t> expected_sizes { 9, 6304, 704, 704, 704, 704, 704, 704, 384, 9, 3, 3, 3, 3, 3, 3, 3, 2 };
        std::size_t index(0);

        for (auto& conjunct: conjuncts) {

            TO_NORMALFORM::to_dnf(conjunct);
            auto disjuncts = TO_NORMALFORM::split_disjunction(std::move(conjunct), false);

            PLAJA_ASSERT(not disjuncts.empty())

            if (disjuncts.size() == 1) { conjuncts_processed.push_back(std::move(disjuncts.front())); }
            else {

                auto it = disjuncts.begin();
                for (; it != disjuncts.end();) {

                    solver->push();

                    MARABOU_IN_PLAJA::to_marabou_constraint(**it, *stateIndexesMapping)->add_to_query(*solver);

                    const auto rlt = solver->pre_and_check();
                    PLAJA_ASSERT(not PLAJA::SmtSolver::is_unknown(rlt))
                    PLAJA_ASSERT(rlt == PLAJA::SmtSolver::UnSat or rlt == PLAJA::SmtSolver::Sat)
                    if (rlt == PLAJA::SmtSolver::Sat) { ++it; }
                    else { it = disjuncts.erase(it); }

                    solver->pop();

                }

                TS_ASSERT(not disjuncts.empty())

                TS_ASSERT_EQUALS(disjuncts.size(), expected_sizes[index++])

                conjuncts_processed.push_back(TO_NORMALFORM::construct_disjunction(std::move(disjuncts)));
            }

        }

        TS_ASSERT(conjuncts.size() == conjuncts_processed.size())

        PLAJA_LOG(PLAJA_UTILS::string_f("Preprocessed [%f]...", PLAJA_GLOBAL::timer->operator()() - time))

        auto expr_opt = TO_NORMALFORM::construct_conjunction(std::move(conjuncts_processed));

        auto rlt = MARABOU_IN_PLAJA::to_marabou_constraint(*expr_opt, *stateIndexesMapping);

        TS_ASSERT_EQUALS(PLAJA_UTILS::cast_ptr<ConjunctionInMarabou>(rlt.get())->size(), 39)

        PLAJA_LOG(PLAJA_UTILS::string_f("... encoded [%f].", PLAJA_GLOBAL::timer->operator()() - time))

    }

/**********************************************************************************************************************/

    void run_tests() override { RUN_TEST(test_to_marabou_large()) }

};

TEST_MAIN(ToMarabouLargeTest)

#endif //PLAJA_TESTS_MODEL_Z3_TEST_CPP
