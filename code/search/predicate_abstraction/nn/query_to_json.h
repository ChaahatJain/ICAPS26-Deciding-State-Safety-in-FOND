//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_QUERY_TO_JSON_H
#define PLAJA_QUERY_TO_JSON_H

#include <list>
#include "../../information/jani2nnet/using_jani2nnet.h"
#include "../../using_search.h"


// forward declaration:
class ActionOpMarabouPA;
class Equation;
class InputQuery;
class Jani2NNet;
namespace QUERY_TO_JSON { struct QueryExtract; }
namespace PLAJA { class OptionParser; }


class QueryToJson {

private:
    std::string pathToFolder;
    unsigned int outputFileCounter;

    // sampling
    unsigned int sampleCounterSelectionTest;
    unsigned int sampleRateSelectionTest;
    unsigned int sampleCounterApplicabilityTest;
    unsigned int sampleRateApplicabilityTest;
    unsigned int sampleCounterTransitionTest;
    unsigned int sampleRateTransitionTest;

    // cached queries
    std::list<QUERY_TO_JSON::QueryExtract> selectionTestQueries;
    std::list<QUERY_TO_JSON::QueryExtract> applicabilityTestQueries;
    std::list<QUERY_TO_JSON::QueryExtract> transitionTestQueries;

    // query base data
    const Jani2NNet& jani2NNet;
    std::size_t constraintVarsOffset;
    std::size_t constraintsOffset;

    QUERY_TO_JSON::QueryExtract extract_query(const InputQuery& input_query, OutputIndex_type output_index, const bool* rlt) const;

    static void dump_cached_queries(const std::string& filename, std::list<QUERY_TO_JSON::QueryExtract>& query_cache);
    void dump_selection_test_queries();
    void dump_applicability_test_queries();
    void dump_transition_test_queries();

public:
    explicit QueryToJson(const Jani2NNet& jani_2_nnet);
    ~QueryToJson();

    void cache_query(const InputQuery& input_query, OutputIndex_type output_index, const ActionOpMarabouPA* action_op, UpdateIndex_type update_op, const bool* rlt = nullptr);
    void dump_cached_queries();

};


#endif //PLAJA_QUERY_TO_JSON_H
