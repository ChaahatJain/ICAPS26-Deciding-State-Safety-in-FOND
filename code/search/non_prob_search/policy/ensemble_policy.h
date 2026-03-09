//
// This file is part of the PlaJA code base (2019 - 2021).
//

#ifndef PLAJA_TREE_ENSEMBLE_POLICY_H
#define PLAJA_TREE_ENSEMBLE_POLICY_H


#include <memory>
#include <vector>
#include "../../../parser/using_parser.h"
#include "../../../assertions.h"
#include "../../information/jani_2_interface.h"
#include "../../information/jani2ensemble/using_jani2ensemble.h"
#include "../../../../veritas/src/cpp/addtree.hpp"
#include "policy.h"

// forward-declaration:
class Jani2Ensemble;
class Model;
class SuccessorGeneratorC;
class StateBase;


class EnsemblePolicy : public Policy {
    using Floating_type = double;
private:
    const veritas::AddTree veritas_tree;
    const Jani2Interface* ensembleInterface;
    
    std::vector<VariableIndex_type> inputFeatures; // cached input features
    // for applicability filtering
    std::shared_ptr<const SuccessorGeneratorC> successorGenerator;

    mutable ActionLabel_type cachedLabel;
    mutable std::vector<Floating_type> cachedOutputs;
    mutable std::vector<bool> cachedEnabledLabels;
    mutable bool chosenWithTolerance; // last is_chosen answered positively due to tolerance

    void evaluate_trees(const StateBase& state, std::vector<Floating_type>& outputs) const;

    ActionLabel_type evaluate_argmax(const std::vector<Floating_type>& outputs) const;
    [[nodiscard]] bool is_chosen_argmax(const std::vector<Floating_type>& outputs, ActionLabel_type action_label, double tolerance) const;

    struct Compare {
    public:
        bool operator()(const std::pair<::EnsemblePolicy::Floating_type, OutputIndex_type>& a, const std::pair<Floating_type, OutputIndex_type>& b) const { return a.first > b.first or ( a.first == b.first and a.second < b.second ); }
    } outputSort;

    ActionLabel_type evaluate_app_filtering(const StateBase& state, const std::vector<Floating_type>& outputs, std::vector<bool>& enabled_labels) const;
    [[nodiscard]] bool is_chosen_app_filtering(const std::vector<Floating_type>& outputs, const std::vector<bool>& enabled_labels, ActionLabel_type action_label, double tolerance) const;

public:
    explicit EnsemblePolicy(const PLAJA::Configuration& config, const Jani2Ensemble& ensemble_interface);
    ~EnsemblePolicy();
    DELETE_CONSTRUCTOR(EnsemblePolicy)
    // getter:
    [[nodiscard]] inline const Jani2Interface& get_interface() const override { return *ensembleInterface; }

    ActionLabel_type evaluate(const StateBase& state) const override;
    std::vector<double> action_values(const StateBase& state) const override;
    [[nodiscard]] bool is_chosen(const StateBase& state, ActionLabel_type action_label, double tolerance = PLAJA_NN::argMaxPrecision) const override;
    [[nodiscard]] bool is_chosen(ActionLabel_type action_label, double tolerance = PLAJA_NN::argMaxPrecision) const override;
    [[nodiscard]] bool is_chosen_with_tolerance() const override { return chosenWithTolerance; }
    [[nodiscard]] Floating_type get_selection_delta() const override {return 0; }

    // printing:
    FCT_IF_DEBUG(void dump_input_features(const StateBase& state) const;)
    FCT_IF_DEBUG(void dump_cached_outputs() const override;)

};

#endif //PLAJA_TREE_ENSEMBLE_POLICY_H
