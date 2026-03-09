//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_USING_NN_H
#define PLAJA_USING_NN_H

using NeuronIndex_type = unsigned int;
using LayerIndex_type = unsigned int;
using InputIndex_type = NeuronIndex_type;
using OutputIndex_type = NeuronIndex_type;

namespace LAYER_INDEX { constexpr LayerIndex_type none = static_cast<LayerIndex_type>(-1); }
namespace NEURON_INDEX { constexpr NeuronIndex_type none = static_cast<NeuronIndex_type>(-1); }
namespace PLAJA_NN { constexpr double argMaxPrecision = 1.0e-7; }

#endif //PLAJA_USING_NN_H
