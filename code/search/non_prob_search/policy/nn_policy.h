//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_NN_POLICY_H
#define PLAJA_NN_POLICY_H

#include <memory>
#include <vector>
#include "../../../parser/ast/forward_ast.h"
#include "../../../parser/using_parser.h"
#include "../../../assertions.h"
#include "../../factories/forward_factories.h"
#include "../../information/jani2nnet/forward_jani_2_nnet.h"
#include "../../information/jani2nnet/using_jani2nnet.h"
#include "../../states/forward_states.h"
#include "../../successor_generation/forward_successor_generation.h"
#include "policy.h"

namespace PLAJA { class NeuralNetwork; }

class NNPolicy : public Policy {

    using Floating_type = double;

private:
    const PLAJA::NeuralNetwork* network;
    const Jani2Interface* nnInterface;
    
    std::vector<VariableIndex_type> inputFeatures; // cached input features

    // for applicability filtering
    std::shared_ptr<const SuccessorGeneratorC> successorGenerator;

    mutable ActionLabel_type cachedLabel; // label chosen with precision as per last evaluation
    mutable std::vector<Floating_type> cachedOutputs;
    mutable std::vector<bool> cachedEnabledLabels;
    mutable Floating_type selectionDelta; // NN output difference between chosen label and label of interest during last call to "is_chosen"

    void evaluate_nn(const StateBase& state, std::vector<Floating_type>& outputs) const;

    ActionLabel_type evaluate_argmax(const std::vector<Floating_type>& outputs) const;
    [[nodiscard]] bool is_chosen_argmax(ActionLabel_type action_label, double tolerance) const;

    struct Compare {
    public:
        bool operator()(const std::pair<::NNPolicy::Floating_type, OutputIndex_type>& a, const std::pair<Floating_type, OutputIndex_type>& b) const { return a.first > b.first or (a.first == b.first and a.second < b.second); }
    } outputSort;

    ActionLabel_type evaluate_app_filtering(const StateBase& state, const std::vector<Floating_type>& outputs, std::vector<bool>& enabled_labels) const;
    [[nodiscard]] bool is_chosen_app_filtering(ActionLabel_type action_label, double tolerance) const;

public:
    explicit NNPolicy(const PLAJA::Configuration& config, const Jani2NNet& nn_interface);
    ~NNPolicy();
    DELETE_CONSTRUCTOR(NNPolicy)

    // getter:
    inline const Jani2Interface& get_interface() const override { return *nnInterface; }

    ActionLabel_type evaluate(const StateBase& state) const override;
    std::vector<double> action_values(const StateBase& state) const override;
    [[nodiscard]] bool is_chosen(const StateBase& state, ActionLabel_type action_label, double tolerance = PLAJA_NN::argMaxPrecision) const override;
    [[nodiscard]] bool is_chosen(ActionLabel_type action_label, double tolerance = PLAJA_NN::argMaxPrecision) const override;

    [[nodiscard]] Floating_type get_selection_delta() const override {
        PLAJA_ASSERT(selectionDelta >= 0)
        return selectionDelta;
    }

    [[nodiscard]] bool is_chosen_with_tolerance() const override { return get_selection_delta() > 0; }

    // printing:
    FCT_IF_DEBUG(void dump_input_features(const StateBase& state) const;)
    FCT_IF_DEBUG(void dump_cached_outputs() const override;)

};

#endif //PLAJA_NN_POLICY_H
