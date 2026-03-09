//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <fstream>
#include <iomanip>
#include <json.hpp>
#include "query_to_json.h"
#include "../../../include/marabou_include/input_query.h"
#include "../../../option_parser/option_parser.h"
#include "../../../assertions.h"
#include "../../information/jani2nnet/jani_2_nnet.h"
#include "../../smt_nn/using_marabou.h"


// extern:
namespace PLAJA_OPTION { extern const std::string query_cache; }
//
namespace PLAJA_UTILS {
    extern void make_directory(const std::string& path_to_dir);
    extern const std::string slashString;
}


// internal auxiliaries:

namespace QUERY_TO_JSON {

    constexpr unsigned int maxCacheSize = 5000;
    //
    constexpr unsigned int startSampleRate = 1;
    constexpr unsigned int sampleRateUpdateFactor = 1;
    constexpr unsigned int sampleRateUpdateMax = 1024; // the maximal rate at which the rate may be updated; not necessarily the maximal rate
    constexpr unsigned int sampleRateUpdate = 1000;

    //

    enum QueryResult { SAT, UNSAT, UNKNOWN };

    QueryResult to_query_rlt(const bool* rlt) { return rlt ? ( *rlt ? QueryResult::SAT : QueryResult::UNSAT ) : QueryResult::UNKNOWN; }
    std::string query_rlt_to_str(const QueryResult& query_rlt) {
        switch (query_rlt) {
            case SAT: { return "SAT"; }
            case UNSAT: { return "UNSAT"; }
            case UNKNOWN: { return "UNKNOWN"; }
        }
        PLAJA_ABORT
    }

    struct QueryVariable {
        MarabouVarIndex_type index;
        MarabouFloating_type lowerBound;
        MarabouFloating_type upperBound;
        QueryVariable(MarabouVarIndex_type index_, MarabouFloating_type lower_bound, MarabouFloating_type upper_bound):
            index(index_), lowerBound(lower_bound), upperBound(upper_bound) {}
    };

    struct QueryExtract {
        OutputIndex_type label;
        std::vector<QueryVariable> variables;
        std::list<Equation> constraints;
        QueryResult result;
        explicit QueryExtract(OutputIndex_type label_, const bool* rlt = nullptr): label(label_), result(to_query_rlt(rlt)) {}
        inline void add_variable(MarabouVarIndex_type index, MarabouFloating_type lower_bound, MarabouFloating_type upper_bound) { variables.emplace_back(index, lower_bound, upper_bound); }
        inline void add_equation(const Equation& eq) { constraints.emplace_back(eq); }
    };

    const std::string selectionTestsFolder("selection_tests/"); // NOLINT(cert-err58-cpp)
    const std::string applicabilityTestsFolder("applicability_tests/"); // NOLINT(cert-err58-cpp)
    const std::string transitionTestsFolder("transition_tests/"); // NOLINT(cert-err58-cpp)
    const std::string queryFilePrefix("query_"); // NOLINT(cert-err58-cpp)
    const std::string queryFilePostfix(".json"); // NOLINT(cert-err58-cpp)

    nlohmann::json variable_to_json(const QueryVariable& query_variable) {
        nlohmann::json var_j;
        var_j.emplace("name", "v" + std::to_string(query_variable.index));
        var_j.emplace("lower", query_variable.lowerBound);
        var_j.emplace("upper", query_variable.upperBound);
        return var_j;
    }

    std::string op_to_string(Equation::EquationType op) {
        switch (op) {
            case Equation::EQ: { return "="; }
            case Equation::GE: { return ">="; }
            case Equation::LE: { return "<="; }
        }
        PLAJA_ABORT
    }

    nlohmann::json addend_to_json(const Equation::Addend& addend) {
        PLAJA_ASSERT(addend._coefficient != 0) // quick check, not important enough to check with floating precision
        nlohmann::json addend_j;
        addend_j.emplace("var", "v" + std::to_string(addend._variable));
        addend_j.emplace("factor", addend._coefficient);
        return addend_j;
    }

    nlohmann::json equation_to_json(const Equation& equation) {
        nlohmann::json equation_j;
        nlohmann::json addends_j;
        for (const auto& addend: equation._addends) {
            addends_j.push_back(addend_to_json(addend));
        }
        equation_j.emplace("left(sum)", std::move(addends_j));
        equation_j.emplace("op", op_to_string(equation._type));
        equation_j.emplace("right(scalar)", equation._scalar);
        return equation_j;
    }

    nlohmann::json query_to_json(const QueryExtract& query) {
        nlohmann::json query_j;

        // variables
        nlohmann::json vars_j = nlohmann::json::array();
        for (auto var: query.variables) { vars_j.push_back(QUERY_TO_JSON::variable_to_json(var)); }
        query_j.emplace("vars", std::move(vars_j));

        nlohmann::json constraints_j = nlohmann::json::array();
        for (const auto& equation: query.constraints) { constraints_j.push_back(QUERY_TO_JSON::equation_to_json(equation)); }
        query_j.emplace("constraints", std::move(constraints_j));

        // label
        query_j.emplace("label", query.label);

        // result
        query_j.emplace("result", QUERY_TO_JSON::query_rlt_to_str(query.result));

        return query_j;
    }

}


/**********************************************************************************************************************/


QueryToJson::QueryToJson(const Jani2NNet& jani_2_nnet):
    pathToFolder(),
    outputFileCounter(0),
    sampleCounterSelectionTest(0), sampleRateSelectionTest(QUERY_TO_JSON::startSampleRate),
    sampleCounterApplicabilityTest(0), sampleRateApplicabilityTest(QUERY_TO_JSON::startSampleRate),
    sampleCounterTransitionTest(0), sampleRateTransitionTest(QUERY_TO_JSON::startSampleRate),
    jani2NNet(jani_2_nnet), constraintVarsOffset(jani2NNet.number_of_marabou_encoding_vars() - jani2NNet.get_num_input_features()), constraintsOffset(jani2NNet.count_linear_constraints()) {

    if (PLAJA_GLOBAL::optionParser->has_value_option(PLAJA_OPTION::query_cache)) {
        pathToFolder = PLAJA_GLOBAL::optionParser->get_value_option_string(PLAJA_OPTION::query_cache) + PLAJA_UTILS::slashString;
    }

    // make directories
    PLAJA_UTILS::make_directory(pathToFolder + QUERY_TO_JSON::selectionTestsFolder);
    PLAJA_UTILS::make_directory(pathToFolder + QUERY_TO_JSON::applicabilityTestsFolder);
    PLAJA_UTILS::make_directory(pathToFolder + QUERY_TO_JSON::transitionTestsFolder);

}

QueryToJson::~QueryToJson() = default;

//

QUERY_TO_JSON::QueryExtract QueryToJson::extract_query(const InputQuery& input_query, OutputIndex_type output_index, const bool* rlt) const {
    QUERY_TO_JSON::QueryExtract query_extract(output_index, rlt);

    // variables
    const auto num_vars = input_query.getNumberOfVariables();
    query_extract.variables.reserve(num_vars - constraintVarsOffset);
    STMT_IF_DEBUG(Set<MarabouVarIndex_type> state_vars)
    // input vars
    MarabouVarIndex_type var = 0;
    for (; var < jani2NNet.get_num_input_features(); ++var) {
        query_extract.add_variable(var, input_query.getLowerBound(var), input_query.getUpperBound(var));
        STMT_IF_DEBUG(state_vars.insert(var))
        PLAJA_ASSERT(input_query._variableToInputIndex.exists(var))
    }
    // additional vars:
    PLAJA_ASSERT(input_query.getOutputVariables().back() + 1 == var + constraintVarsOffset)
    for (var += constraintVarsOffset; var < num_vars; ++var) {
        query_extract.add_variable(var, input_query.getLowerBound(var), input_query.getUpperBound(var));
        STMT_IF_DEBUG(state_vars.insert(var))
    }
    PLAJA_ASSERT(state_vars.size() == num_vars - constraintVarsOffset)

    // constraints
    std::size_t constraint_index = 0;
    for (const auto& equation: input_query.getEquations()) {
        if (constraint_index++ >= constraintsOffset) { // skip constraints of the NN
            PLAJA_ASSERT(Set<MarabouVarIndex_type>::containedIn(equation.getParticipatingVariables(), state_vars))
            query_extract.add_equation(equation);
        } else { PLAJA_ASSERT(!Set<MarabouVarIndex_type>::containedIn(equation.getParticipatingVariables(), state_vars)) }
    }

    return query_extract;
}

void QueryToJson::dump_cached_queries(const std::string& filename, std::list<QUERY_TO_JSON::QueryExtract>& query_cache) {

    nlohmann::json query_cache_j;

    // queries::
    nlohmann::json queries_j = nlohmann::json::array();
    for (const auto& query: query_cache) { queries_j.push_back(QUERY_TO_JSON::query_to_json(query)); }
    query_cache_j.emplace("queries", std::move(queries_j));

    // output
    std::ofstream out_file(filename, std::ios::trunc);
    out_file << std::setw(4);
    out_file << query_cache_j;
    out_file.close();

    std::cout << "Dumped " << query_cache.size() << " queries to " << filename << std::endl;
    query_cache.clear();
}

void QueryToJson::dump_selection_test_queries() {
    const std::string filename = pathToFolder + QUERY_TO_JSON::selectionTestsFolder + QUERY_TO_JSON::queryFilePrefix + std::to_string(outputFileCounter) + QUERY_TO_JSON::queryFilePostfix;
    dump_cached_queries(filename, selectionTestQueries);
    ++outputFileCounter;
}

void QueryToJson::dump_applicability_test_queries() {
    const std::string filename = pathToFolder + QUERY_TO_JSON::applicabilityTestsFolder + QUERY_TO_JSON::queryFilePrefix + std::to_string(outputFileCounter) + QUERY_TO_JSON::queryFilePostfix;
    dump_cached_queries(filename, applicabilityTestQueries);
    ++outputFileCounter;
}

void QueryToJson::dump_transition_test_queries() {
    const std::string filename = pathToFolder + QUERY_TO_JSON::transitionTestsFolder + QUERY_TO_JSON::queryFilePrefix + std::to_string(outputFileCounter) + QUERY_TO_JSON::queryFilePostfix;
    dump_cached_queries(filename, transitionTestQueries);
    ++outputFileCounter;
}

//

void QueryToJson::cache_query(const InputQuery& input_query, OutputIndex_type output_index, const ActionOpMarabouPA* action_op, UpdateIndex_type update_op, const bool* rlt) {
    if (action_op) {
        if (update_op != ACTION::noneUpdate) {
            if (sampleCounterTransitionTest++ % sampleRateTransitionTest == 0) {
                // transition test
                transitionTestQueries.push_back(extract_query(input_query, output_index, rlt));
                // update sample rate
                if (sampleRateTransitionTest < QUERY_TO_JSON::sampleRateUpdateMax && transitionTestQueries.size() % QUERY_TO_JSON::sampleRateUpdate == 0) {
                    sampleCounterTransitionTest = 0;
                    sampleRateTransitionTest *= QUERY_TO_JSON::sampleRateUpdateFactor;
                }
                // dump
                if (transitionTestQueries.size() >= QUERY_TO_JSON::maxCacheSize) { dump_transition_test_queries(); }
            }
        } else {
            if (sampleCounterApplicabilityTest++ % sampleRateApplicabilityTest == 0) {
                // applicability test
                applicabilityTestQueries.push_back(extract_query(input_query, output_index, rlt));
                // update sample rate
                if (sampleRateApplicabilityTest < QUERY_TO_JSON::sampleRateUpdateMax && applicabilityTestQueries.size() % QUERY_TO_JSON::sampleRateUpdate == 0) {
                    sampleCounterApplicabilityTest = 0;
                    sampleRateApplicabilityTest *= QUERY_TO_JSON::sampleRateUpdateFactor;
                }
                // dump
                if (applicabilityTestQueries.size() >= QUERY_TO_JSON::maxCacheSize) { dump_applicability_test_queries(); }
            }
        }
    } else {
        if (sampleCounterSelectionTest++ % sampleRateSelectionTest == 0) {
            // selection test
            selectionTestQueries.push_back(extract_query(input_query, output_index, rlt));
            // update sample rate
            if (sampleRateSelectionTest < QUERY_TO_JSON::sampleRateUpdateMax && selectionTestQueries.size() % QUERY_TO_JSON::sampleRateUpdate == 0) {
                sampleCounterSelectionTest = 0;
                sampleRateSelectionTest *= QUERY_TO_JSON::sampleRateUpdateFactor;
            }
            // dump
            if (selectionTestQueries.size() >= QUERY_TO_JSON::maxCacheSize) { dump_selection_test_queries(); }
        }
    }
}

void QueryToJson::dump_cached_queries() {
    dump_selection_test_queries();
    dump_applicability_test_queries();
    dump_transition_test_queries();
}
