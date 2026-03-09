//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_ADVERSARIAL_ATTACK_H
#define PLAJA_ADVERSARIAL_ATTACK_H

#include "../search/information/jani2nnet/using_jani2nnet.h"
#include "../search/predicate_abstraction/using_predicate_abstraction.h"

// forward declaration:
class Equation;
class InputQuery;
class Jani2NNet;
class StateIndexesToMarabou;
class SearchStatisticsNNSat;
namespace ADVERSARIAL_ATTACK { struct Initializer; }
namespace pybind11 {
    class scoped_interpreter;
    class object;
}

#pragma GCC visibility push(hidden)
class AdversarialAttack {
private:
    SearchStatisticsNNSat* sharedStatistics;
    std::unique_ptr<ADVERSARIAL_ATTACK::Initializer> initializer; // hack to set flags for python
    std::unique_ptr<pybind11::scoped_interpreter> interpreter;
    std::unique_ptr<pybind11::object> attackInstance;
    std::unique_ptr<pybind11::object> queryInstance;
    std::unique_ptr<pybind11::object> ConstraintModule;

    std::size_t inputSize;
    std::size_t constraintVarsOffset;
    std::size_t constraintsOffset;

    [[nodiscard]] unsigned int marabou_var_to_py(MarabouVarIndex_type var) const;
    [[nodiscard]] MarabouVarIndex_type py_to_marabou_var(unsigned int var) const;

public:
    AdversarialAttack(const Jani2NNet& jani_2_nnet, std::size_t num_additional_vars);
    ~AdversarialAttack();

    void set_statistics(SearchStatisticsNNSat* shared_statistics) { sharedStatistics = shared_statistics; }

    inline void set_constraint_vars_offset(std::size_t constraint_vars_offset) { constraintVarsOffset = constraint_vars_offset; }

    // interface
    void set_bounds(MarabouVarIndex_type var, int lower_bound, int upper_bound);
    void add_constraint(const Equation& eq);
    void set_label(OutputIndex_type output_index);
    void set_solution_guess(const StateValues& solution_guess, const InputQuery& input_query, const StateIndexesToMarabou& state_indexes_to_marabou);
    //
    bool check();
    bool check(const InputQuery& input_query, OutputIndex_type output_index, const StateValues* solution_guess, const StateIndexesToMarabou* state_indexes_to_marabou);
    void extract_solution(StateValues& solution, const InputQuery& input_query, const StateIndexesToMarabou& state_indexes_to_marabou);

};

#pragma GCC visibility pop

#endif //PLAJA_ADVERSARIAL_ATTACK_H
