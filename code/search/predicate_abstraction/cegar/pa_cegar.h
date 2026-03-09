//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PA_CEGAR_H
#define PLAJA_PA_CEGAR_H

#include "../../../include/ct_config_const.h"
#include "../../../parser/ast/expression/forward_expression.h"
#include "../../../parser/ast/forward_ast.h"
#include "../../../utils/default_constructors.h"
#include "../../factories/predicate_abstraction/forward_factories_pa.h"
#include "../../fd_adaptions/search_engine.h"
#include "../../using_search.h"
#include "forward_cegar.h"
#include <list>
#include <memory>
#include <unordered_set>
#include <vector>
#include "../../states/state_values.h"

class StateBase;
class SearchPAPath;

class PACegar final: public SearchEngine {
    friend class PACegarTest;

private:
    // cached:
    std::unique_ptr<Properties> cachedProperties;

    // abstract search
    std::unique_ptr<Property> paProperty;
    std::unique_ptr<PredicatesStructure> predicates;
    std::unique_ptr<PropertyInformation> propInfoSub;
    unsigned int maxNumPreds;
    // sub-engine (have to cache to prohibit dangling pointer in case of signal handling)
    std::unique_ptr<SearchPAPath> cegarSearch;

    // concrete path certification:
    std::unique_ptr<SpuriousnessChecker> spuriousnessChecker;
    std::unique_ptr<SpuriousnessResult> current_spuriousness_result;

    PAExpression* get_pa_expression();
    void cache_pa_property();
    void predicates_to_file() const;

    // override:
    SearchStatus initialize() override;
    SearchStatus step() override;

    std::vector<StateValues> unsafe_path;


public:
    explicit PACegar(const SearchEngineConfigPA& config);
    ~PACegar() override;
    DELETE_CONSTRUCTOR(PACegar)

    std::unordered_set<std::unique_ptr<StateBase>> extract_concrete_unsafe_path();
    std::vector<StateValues> extract_ordered_concrete_unsafe_path();

    // stats
    void update_statistics() const override;
    void print_statistics() const override;
    void stats_to_csv(std::ofstream& file) const override;
    void stat_names_to_csv(std::ofstream& file) const override;

    // work around for getting a solved status after finalize
    bool is_safe = false;
    std::vector<StateValues> getUnsafePath() const {
        return unsafe_path;
    }
};

#endif //PLAJA_PA_CEGAR_H
