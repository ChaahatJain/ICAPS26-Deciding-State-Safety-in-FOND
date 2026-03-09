//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_JANI_2_NNET_H
#define PLAJA_JANI_2_NNET_H

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
#include "forward_jani_2_nnet.h"
#include "using_jani2nnet.h"
#include "../jani_2_interface.h"

namespace PLAJA { class NeuralNetwork; }

/**
 * This class documents the interface between a JANI model and an underlying neural network stored in .nnet format.
 * The interface is dependent on the current implementation of JANI models, \ie, the used indexes and ids.
 */
class Jani2NNet : public Jani2Interface {
    friend class Jani2NNetParser;

protected:
    std::string nnFile;
    std::string torchFile;
    std::vector<unsigned int> hiddenLayerSizes;

private:
    // loaded structures
    mutable std::unique_ptr<PLAJA::NeuralNetwork> nn;
    mutable std::unique_ptr<NNPolicy> nnPolicy;

protected:
    void compute_labels_to_outputs();
    void compute_layer_sizes();
    void compute_extended_inputs();

    explicit Jani2NNet(const Model& model_);
    Jani2NNet(const Jani2NNet& jani_2_nnet);
public:
    virtual ~Jani2NNet();

    Jani2NNet(Jani2NNet&& jani_2_nnet) = delete;
    DELETE_ASSIGNMENT(Jani2NNet)

    static std::unique_ptr<Jani2NNet> load_interface(const Model& model);
    static std::unique_ptr<Jani2NNet> load_interface(const Model& model, const std::string& jani_2_nnet_file);
    std::unique_ptr<Jani2Interface> copy_interface(const std::string& path_to_directory, const std::string* filename_prefix = nullptr, bool dump = true) const override;

    [[nodiscard]] const PLAJA::NeuralNetwork& load_network() const;
    [[nodiscard]] const Policy& load_policy(const PLAJA::Configuration& config) const override;
    // For debugging only, load policy directly for runtime usage.
    FCT_IF_DEBUG([[nodiscard]] ActionLabel_type eval_policy(const StateBase& state) const override;)
    FCT_IF_DEBUG([[nodiscard]] bool is_chosen(const StateBase& state, ActionLabel_type action_label) const override;)

    [[nodiscard]] inline const std::string& get_policy_file() const override  { return nnFile; }

    [[nodiscard]] inline const std::string& get_torch_file() const { return torchFile; }

    /* Layers: */

    [[nodiscard]] inline std::size_t get_num_layers() const { return layerSizes.size(); }

    [[nodiscard]] inline std::size_t get_num_hidden_layers() const { return layerSizes.size() - 2; }

    [[nodiscard]] inline const std::vector<unsigned int>& get_layer_sizes() const { return layerSizes; }

    [[nodiscard]] inline const std::vector<unsigned int>& get_hidden_layer_sizes() const { return hiddenLayerSizes; }

    /* Auxiliary counting: */

    [[nodiscard]] inline std::size_t count_hidden_neurons() const {
        std::size_t rlt = 0;
        for (const auto num_neurons: get_hidden_layer_sizes()) { rlt += num_neurons; }
        return rlt;
    }

    [[nodiscard]] inline std::size_t count_neurons() const { return get_num_input_features() + count_hidden_neurons() + get_num_output_features(); }

    [[nodiscard]] inline std::size_t count_weighted_sums() const { return count_hidden_neurons() + get_num_output_features(); } // for each hidden/output neuron a weighted input sum

    [[nodiscard]] inline std::size_t count_activations() const { return count_hidden_neurons(); } // for each hidden neuron an activation

    [[nodiscard]] inline std::size_t count_output_constraints() const { return get_num_output_features() - 1; } // #outputs - 1 argmax constraints

    [[nodiscard]] inline std::size_t count_linear_constraints() const { return count_weighted_sums() + count_output_constraints(); }

    [[nodiscard]] inline std::size_t number_of_marabou_encoding_vars() const { return count_neurons() + count_hidden_neurons(); } // number of vars in the standard Marabou encoding: one variable per neuron + one auxiliary variable for each hidden neuron

/**********************************************************************************************************************/
};

#endif //PLAJA_JANI_2_NNET_H
