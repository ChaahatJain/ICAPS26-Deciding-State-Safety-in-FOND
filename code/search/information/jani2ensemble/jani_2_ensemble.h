//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#ifndef PLAJA_JANI_2_ENSEMBLE_H
#define PLAJA_JANI_2_ENSEMBLE_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "../../../parser/ast/expression/forward_expression.h"
#include "../../../parser/ast/forward_ast.h"
#include "../../../assertions.h"
#include "../../factories/forward_factories.h"
#include "../../non_prob_search/policy/forward_policy.h"
#include "../../states/forward_states.h"
#include "../forward_information.h"
#include "forward_jani_2_ensemble.h"
#include "using_jani2ensemble.h"
#include "../jani_2_interface.h"
#include "addtree.hpp"
/**
 * This class documents the interface between a JANI model and an underlying tree ensemble stored in .json format.
 * The interface is dependent on the current implementation of JANI models, \ie, the used indexes and ids.
 */
using namespace veritas;
class Jani2Ensemble : public Jani2Interface {
    friend class Jani2EnsembleParser;
    using OutputIndex_type = unsigned int;
    using InputIndex_type = unsigned int;
private:
    // loaded structures
    mutable veritas::AddTree ensemble = veritas::AddTree(0, veritas::AddTreeType::REGR);
    mutable std::string ensembleFile;
    mutable std::unique_ptr<EnsemblePolicy> ensemblePolicy;

protected:
    void compute_labels_to_outputs();
    void compute_layer_sizes();
    void compute_extended_inputs();

    explicit Jani2Ensemble(const Model& model_);
    Jani2Ensemble(const Jani2Ensemble& jani_2_ensemble);
public:
    virtual ~Jani2Ensemble();

    Jani2Ensemble(Jani2Ensemble&& jani_2_ensemble) = delete;
    DELETE_ASSIGNMENT(Jani2Ensemble)

    static std::unique_ptr<Jani2Ensemble> load_interface(const Model& model);
    static std::unique_ptr<Jani2Ensemble> load_interface(const Model& model, const std::string& jani_2_ensemble_file);
    std::unique_ptr<Jani2Interface> copy_interface(const std::string& path_to_directory, const std::string* filename_prefix = nullptr, bool dump = true) const override;

    [[nodiscard]] const veritas::AddTree& load_ensemble() const;
    [[nodiscard]] const Policy& load_policy(const PLAJA::Configuration& config) const override;
    // For debugging only, load policy directly for runtime usage.
    FCT_IF_DEBUG([[nodiscard]] ActionLabel_type eval_policy(const StateBase& state) const override;)
    FCT_IF_DEBUG([[nodiscard]] bool is_chosen(const StateBase& state, ActionLabel_type action_label) const override;)

    [[nodiscard]] inline const std::string& get_policy_file() const override { return ensembleFile; }   

/**********************************************************************************************************************/

};

#endif //PLAJA_JANI_2_ENSEMBLE_H
