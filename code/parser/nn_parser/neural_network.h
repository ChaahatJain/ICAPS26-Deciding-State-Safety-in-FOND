//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_ACAS_NEURAL_NETWORK_H
#define PLAJA_ACAS_NEURAL_NETWORK_H

#include <string>
#include <vector>

// forward declaration:
class Jani2NNet;

namespace PLAJA {

    /**
     * Reimplementation (of a subset) of Marabou's AcasNeuralNetwork
     * specifically https://github.com/NeuralNetworkVerification/Marabou/blob/019f16633c3b18999e253d0a325e92ed8cda3c1f/src/input_parsers/AcasNeuralNetwork.h
     * and https://github.com/NeuralNetworkVerification/Marabou/blob/019f16633c3b18999e253d0a325e92ed8cda3c1f/src/input_parsers/AcasNeuralNetwork.cpp
     * (April 2022).
     */
    class NeuralNetwork {
        friend ::Jani2NNet;
    private:
        std::vector<double> min;
        std::vector<double> max;
        std::vector<double> means;
        std::vector<double> ranges;
        std::vector<std::size_t> layerSizes; // including input layer
        std::vector<std::vector<std::vector<double>>> weights;
        std::vector<std::vector<double>> biases;

    public:
        explicit NeuralNetwork(const std::string& path);

        ~NeuralNetwork();

        [[nodiscard]] std::size_t getNumLayers() const { return biases.size(); } // excluding input layer
        [[nodiscard]] std::size_t getLayerSize(unsigned layer) const { return layerSizes[layer]; }

        [[nodiscard]] double getWeight(std::size_t source_layer, std::size_t source_neuron, std::size_t target_neuron) const { return weights[source_layer][target_neuron][source_neuron]; }

        [[nodiscard]] double getBias(std::size_t layer, std::size_t neuron) const { return biases[layer - 1][neuron]; }

        [[nodiscard]] double getInputLowerBound(std::size_t neuron) const { return min[neuron]; }

        [[nodiscard]] double getInputUpperBound(std::size_t neuron) const { return max[neuron]; }

        void evaluate(const std::vector<double>& inputs, std::vector<double>& outputs, unsigned output_size) const;

        [[nodiscard]] double computeDiffSum(const std::vector<double>& inputs, const std::vector<double>& ref_outputs) const; // PlaJA extension

    };

}

#endif //PLAJA_ACAS_NEURAL_NETWORK_H
