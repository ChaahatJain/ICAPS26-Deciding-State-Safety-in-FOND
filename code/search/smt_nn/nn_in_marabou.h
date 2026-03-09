//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_NN_IN_MARABOU_H
#define PLAJA_NN_IN_MARABOU_H

#include <memory>
#include <unordered_map>
#include <vector>
#include "../../include/marabou_include/forward_marabou.h"
#include "../../parser/nn_parser/using_nn.h"
#include "../../utils/utils.h"
#include "forward_smt_nn.h"
#include "using_marabou.h"

namespace PLAJA { class NeuralNetwork; }

class NNInMarabou {

private:
    const PLAJA::NeuralNetwork& nn;
    MARABOU_IN_PLAJA::Context& context;
    std::vector<std::vector<std::unique_ptr<MARABOU_IN_PLAJA::NeuronInMarabou>>> neuronToMarabou;
    std::unordered_map<MarabouVarIndex_type, std::pair<LayerIndex_type, NeuronIndex_type>> varToNeuron; // i.e., the input variables for hidden layer neurons

    void set_neuron_to_var(LayerIndex_type layer, NeuronIndex_type neuron, std::unique_ptr<MARABOU_IN_PLAJA::NeuronInMarabou>&& neuron_in_marabou);

    template<typename NeuronInMarabou_t> [[nodiscard]]
    inline const NeuronInMarabou_t* get_neuron_in_marabou_cast(LayerIndex_type layer, NeuronIndex_type neuron) const {
        return &PLAJA_UTILS::cast_ref<NeuronInMarabou_t>(get_neuron_in_marabou(layer, neuron));
    }

public:
    explicit NNInMarabou(const PLAJA::NeuralNetwork& nn_, MARABOU_IN_PLAJA::Context& c, const std::vector<MarabouVarIndex_type>* input_vars = nullptr);
    ~NNInMarabou();
    DELETE_CONSTRUCTOR(NNInMarabou)

    [[nodiscard]] inline const PLAJA::NeuralNetwork& _nn() const { return nn; }

    [[nodiscard]] inline MARABOU_IN_PLAJA::Context& _context() const { return context; }

    [[nodiscard]] inline std::size_t get_num_layers() const { return neuronToMarabou.size(); }

    [[nodiscard]] inline std::size_t get_layer_size(LayerIndex_type layer) const {
        PLAJA_ASSERT(layer < neuronToMarabou.size())
        return neuronToMarabou[layer].size();
    }

    [[nodiscard]] inline std::size_t get_input_layer_size() const { return get_layer_size(0); }

    [[nodiscard]] inline std::size_t get_output_layer_size() const {
        PLAJA_ASSERT(neuronToMarabou.size() >= 3)
        return get_layer_size(neuronToMarabou.size() - 1);
    }

    [[nodiscard]] std::size_t get_nn_size() const;

    [[nodiscard]] inline const MARABOU_IN_PLAJA::NeuronInMarabou& get_neuron_in_marabou(LayerIndex_type layer, NeuronIndex_type neuron) const {
        PLAJA_ASSERT(layer < get_num_layers() and neuron < get_layer_size(layer))
        return *neuronToMarabou[layer][neuron];
    }

    [[nodiscard]] inline MARABOU_IN_PLAJA::NeuronInMarabou& get_neuron_in_marabou(LayerIndex_type layer, NeuronIndex_type neuron) {
        PLAJA_ASSERT(layer < get_num_layers() and neuron < get_layer_size(layer))
        return *neuronToMarabou[layer][neuron];
    }

    [[nodiscard]] MarabouVarIndex_type get_var(LayerIndex_type layer, NeuronIndex_type neuron) const;
    [[nodiscard]] MarabouVarIndex_type get_activation_var(LayerIndex_type layer, NeuronIndex_type neuron) const;

    [[nodiscard]] inline MarabouVarIndex_type get_input_var(NeuronIndex_type neuron) const { return get_var(0, neuron); }

    [[nodiscard]] inline MarabouVarIndex_type get_output_var(NeuronIndex_type neuron) const {
        PLAJA_ASSERT(neuronToMarabou.size() >= 3)
        return get_var(get_num_layers() - 1, neuron);
    }

    [[nodiscard]] inline const std::pair<LayerIndex_type, NeuronIndex_type>& get_neuron(MarabouVarIndex_type var) const {
        PLAJA_ASSERT(varToNeuron.count(var))
        return varToNeuron.at(var);
    }

    void add_to_query(MARABOU_IN_PLAJA::MarabouQuery& query) const;

    // static std::unique_ptr<NNInMarabou> add_nn_to_query(const PLAJA::NeuralNetwork& nn, InputQuery& input_query, const std::vector<MarabouVarIndex_type>* input_vars = nullptr);

    class NeuronIterator {
        friend NNInMarabou;

    private:
        const NNInMarabou* parent;
        LayerIndex_type layerIndex;
        NeuronIndex_type neuronIndex;

        explicit NeuronIterator(const NNInMarabou& parent_):
            parent(&parent_)
            , layerIndex(0)
            , neuronIndex(0) {}

    public:
        ~NeuronIterator() = default;
        DELETE_CONSTRUCTOR(NeuronIterator)

        inline void operator++() {
            ++neuronIndex;
            if (neuronIndex >= parent->get_layer_size(layerIndex)) {
                ++layerIndex;
                neuronIndex = 0;
            }
        }

        [[nodiscard]] inline bool end() const { return layerIndex >= parent->get_num_layers(); }

        [[nodiscard]] inline LayerIndex_type _layer() const { return layerIndex; }

        [[nodiscard]] inline NeuronIndex_type _neuron() const { return neuronIndex; }

        [[nodiscard]] inline MarabouVarIndex_type get_var() const { return parent->get_var(layerIndex, neuronIndex); }

        [[nodiscard]] inline bool has_activation_var() const { return 0 < layerIndex and layerIndex < parent->get_num_layers() - 1; }

        [[nodiscard]] inline MarabouVarIndex_type get_activation_var() const { return parent->get_activation_var(layerIndex, neuronIndex); }

    };

    [[nodiscard]] inline NeuronIterator neuronIterator() const { return NeuronIterator(*this); }
};

#endif //PLAJA_NN_IN_MARABOU_H
