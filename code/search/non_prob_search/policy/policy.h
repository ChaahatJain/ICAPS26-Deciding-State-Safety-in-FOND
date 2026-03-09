#ifndef PLAJA_POLICY_H
#define PLAJA_POLICY_H
#include <memory>
#include <vector>
#include "../../../parser/using_parser.h"
#include "../../../assertions.h"
#include "../../information/jani2nnet/using_jani2nnet.h"
#include "../../information/jani_2_interface.h"

// forward-declaration:
class Jani2NNet;
class Model;
class SuccessorGeneratorC;
class StateBase;
namespace PLAJA { class NeuralNetwork; }

class Policy {
public:
    using Floating_type = double;

    virtual ~Policy() {}
    virtual inline const Jani2Interface& get_interface() const = 0;
    virtual ActionLabel_type evaluate(const StateBase& state) const = 0;
    virtual bool is_chosen(const StateBase& state, ActionLabel_type action_label, double tolerance = PLAJA_NN::argMaxPrecision) const = 0;
    virtual bool is_chosen(ActionLabel_type action_label, double tolerance = PLAJA_NN::argMaxPrecision) const = 0;
    virtual bool is_chosen_with_tolerance() const = 0;
    virtual Floating_type get_selection_delta() const = 0;
    FCT_IF_DEBUG(virtual void dump_cached_outputs() const = 0;)
    virtual std::vector<double> action_values(const StateBase& state) const = 0;

};

#endif //PLAJA_POLICY_H
