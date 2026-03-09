//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_NN_IN_Z3_H
#define PLAJA_NN_IN_Z3_H

#include <memory>
#include <vector>
#include "../utils/forward_z3.h"
#include "../../../parser/nn_parser/using_nn.h"
#include "../../../assertions.h"
#include "../using_smt.h"

// forward declaration
struct NNVarsInZ3;
namespace PLAJA { class NeuralNetwork; }
namespace Z3_IN_PLAJA { class Context; }


class NNInZ3 {

    enum Encoding { N1V, N2V, MIP };

    struct NeuronInZ3 {
    private:
        Z3_IN_PLAJA::VarId_type var;
        Z3_IN_PLAJA::VarId_type aux;
        Z3_IN_PLAJA::VarId_type binary;
    public:
        NeuronInZ3();
        explicit NeuronInZ3(Z3_IN_PLAJA::VarId_type var_);
        NeuronInZ3(Z3_IN_PLAJA::Context& c, LayerIndex_type layer, NeuronIndex_type neuron, const std::string& postfix, Encoding mode);
        DEFAULT_CONSTRUCTOR(NeuronInZ3)
        ~NeuronInZ3();

        [[nodiscard]] inline Z3_IN_PLAJA::VarId_type _var() const { return var; }
        [[nodiscard]] inline Z3_IN_PLAJA::VarId_type _aux() const { PLAJA_ASSERT(aux != Z3_IN_PLAJA::noneVar) return aux; }
        [[nodiscard]] inline Z3_IN_PLAJA::VarId_type _binary() const { PLAJA_ASSERT(aux != Z3_IN_PLAJA::noneVar) return binary; }
        [[nodiscard]] const z3::expr& get_var(const Z3_IN_PLAJA::Context& c) const;
        [[nodiscard]] const z3::expr& get_aux(const Z3_IN_PLAJA::Context& c) const;
        [[nodiscard]] const z3::expr& get_binary(const Z3_IN_PLAJA::Context& c) const;
    };

private:
    Z3_IN_PLAJA::Context* context;
    const PLAJA::NeuralNetwork& nn;
    Encoding mode;
    std::vector<std::vector<NeuronInZ3>> neuronToVar; // i.e., the input variables for hidden layer neurons
    std::vector<std::vector<std::unique_ptr<z3::expr>>> neuronToZ3;

public:
    NNInZ3(const PLAJA::NeuralNetwork& nn_, Z3_IN_PLAJA::Context& c, const std::vector<Z3_IN_PLAJA::VarId_type>& input_vars, const std::string& postfix);
    ~NNInZ3();
    DELETE_CONSTRUCTOR(NNInZ3)

    [[nodiscard]] inline Z3_IN_PLAJA::Context& _context() const { return *context; }

    [[nodiscard]] inline std::size_t get_num_layers() const { return neuronToVar.size(); }
    [[nodiscard]] inline std::size_t get_layer_size(LayerIndex_type layer) const { PLAJA_ASSERT(layer < neuronToVar.size()) return neuronToVar[layer].size(); }
    [[nodiscard]] inline std::size_t get_input_layer_size() const { return get_layer_size(0); }
    [[nodiscard]] inline std::size_t get_output_layer_size() const { PLAJA_ASSERT(neuronToVar.size() >= 3) return get_layer_size(neuronToVar.size() - 1); }

    [[nodiscard]] inline const NNInZ3::NeuronInZ3& get_neuron(LayerIndex_type layer, NeuronIndex_type neuron) const { PLAJA_ASSERT(layer < get_num_layers() and neuron < get_layer_size(layer)) return neuronToVar[layer][neuron]; }
    [[nodiscard]] inline const z3::expr& get_neuron_z3(LayerIndex_type layer, NeuronIndex_type neuron) const { PLAJA_ASSERT(layer < get_num_layers() and neuron < get_layer_size(layer)) return *neuronToZ3[layer][neuron]; }
    [[nodiscard]] inline const z3::expr& get_output_var_z3(NeuronIndex_type neuron) const { PLAJA_ASSERT(neuron < get_layer_size(get_num_layers() - 1)) return neuronToVar[get_num_layers() - 1][neuron].get_var(*context); }

    [[nodiscard]] z3::expr to_sum(LayerIndex_type target_layer, NeuronIndex_type target, bool include_bias = true);

    void add(Z3_IN_PLAJA::SMTSolver& solver) const;

    FCT_IF_DEBUG(void dump() const;)

};


#endif //PLAJA_NN_IN_Z3_H
