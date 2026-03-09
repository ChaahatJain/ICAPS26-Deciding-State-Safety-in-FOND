//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_NEURAL_NETWORK_H
#define PLAJA_NEURAL_NETWORK_H

#include <torch/nn/module.h>
#include <torch/nn/modules/linear.h>
#include <torch/nn/modules/batchnorm.h>
#include "../using_policy_learning.h"

namespace TORCH_IN_PLAJA {

    class NeuralNetwork: public torch::nn::Module {

    private:
        std::vector<unsigned int> layerSizes;
        std::vector<std::string> layerDesc;
        unsigned int inputSize;
        unsigned int outputSize;
        unsigned int totalPasses;
        bool useBatchNorms;
        std::vector<torch::nn::Linear> forwardingFuncs;
        std::vector<torch::nn::BatchNorm1d> batchNorms;
        std::vector<std::vector<int64_t>> zeroFrequencies;

        /* For to nnet. */
        std::vector<TORCH_IN_PLAJA::Floating> lowerBounds;
        std::vector<TORCH_IN_PLAJA::Floating> upperBounds;
        const double reductionThreshold;

    public:
        explicit NeuralNetwork(std::vector<unsigned int> layers, double reductionThreshold = 0.0, std::vector<TORCH_IN_PLAJA::Floating> lower_bounds = std::vector<TORCH_IN_PLAJA::Floating>(), std::vector<TORCH_IN_PLAJA::Floating> upper_bounds = std::vector<TORCH_IN_PLAJA::Floating>(), bool use_batch_norms = false);
        ~NeuralNetwork() override;

        std::vector<std::string> get_LayerDesc() const;

        unsigned int get_totalPasses() const;

        void resetTracking();

        std::vector<std::vector<int64_t>> get_zeroFrequencies();

        torch::Tensor forward(torch::Tensor x); // Manual does parameter not by copy.

        bool isNeuronZeroed(unsigned int layer, unsigned int row);

        void setNeuronToZero(unsigned int layer, unsigned int row, bool compact);

        void load_from_nnet(const std::string& file_name);

        void to_nnet(const std::string& file_name);

        void dump() const;

    };

    TORCH_MODULE_IMPL(NeuralNetworkModule, NeuralNetwork);

}

#endif //PLAJA_NEURAL_NETWORK_H
