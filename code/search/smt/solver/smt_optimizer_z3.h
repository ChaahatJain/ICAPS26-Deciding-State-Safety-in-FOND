//
// Created by Daniel Sherbakov in 2024.
//

#ifndef SMT_OPTIMIZER_Z3_H
#define SMT_OPTIMIZER_Z3_H
#include "smt_solver_z3.h"

// Required for bound registration.
// Can be probably drastically simplified for this use case.
struct Z3Assertions {

private:
    /* Added assertions. */
    z3::expr_vector assertions;
    std::list<z3::expr_vector> vecAssertions;

    /* RegisteredBounds */
    std::unordered_map<Z3_IN_PLAJA::VarId_type, z3::expr> registeredBounds;

public:
    explicit Z3Assertions(z3::context& c) : assertions(c){};
    ~Z3Assertions() = default;
    DELETE_CONSTRUCTOR(Z3Assertions)

    inline void add(const z3::expr& e) { assertions.push_back(e); }

    [[nodiscard]] inline bool exists_bound(Z3_IN_PLAJA::VarId_type var) const { return registeredBounds.count(var); }

    FCT_IF_DEBUG([[nodiscard]] const z3::expr* retrieve_bound(Z3_IN_PLAJA::VarId_type var) const;)

    inline void register_bound(Z3_IN_PLAJA::VarId_type var, const z3::expr& bound) {
        PLAJA_ASSERT(not exists_bound(var))
        registeredBounds.emplace(var, bound);
    }

    z3::expr_vector& operator()() { return assertions; }

};

class SMTOptimizer final: public Z3_IN_PLAJA::SMTSolver {
    z3::optimize optimizer;
    z3::expr objective;
    std::list<Z3Assertions> pushedAssertions;
    z3::expr assertions;

public:
    explicit SMTOptimizer(std::shared_ptr<Z3_IN_PLAJA::Context>&& c, z3::expr objective);
    ~SMTOptimizer() override = default;
    DELETE_CONSTRUCTOR(SMTOptimizer)

    [[nodiscard]] inline bool exists_bound(Z3_IN_PLAJA::VarId_type var) override { return std::any_of(pushedAssertions.begin(), pushedAssertions.end(), [var](const Z3Assertions& assertions) { return assertions.exists_bound(var); }); }


    inline void register_bound_if(Z3_IN_PLAJA::VarId_type var, const z3::expr& e) override {
        if (exists_bound(var)) {
            PLAJA_ASSERT(e.id() == retrieve_bound(var)->id())
            return;
        }
        optimizer.add(e);
        pushedAssertions.back().register_bound(var, e);
    }


    void add(const z3::expr& e) override {
        // optimizer.pop();
        // assertions = (assertions && e).simplify();
        pushedAssertions.back().add(e);
        // const auto e_str = e.to_string();
        // const auto e_name = optimizer.ctx().bool_const(e_str.c_str());
        // optimizer.add(e,e_name);
        optimizer.add(e);
        // optimizer.push();
    }

    void add(const z3::expr_vector& v) override {
        optimizer.add(v);
    }

    void maximize() {
        optimizer.maximize(this->objective);
    }

    void minimize() {
        optimizer.minimize(this->objective);
    }

    void pop() override {
        optimizer.pop();
    }

    void push() override {
        optimizer.push();
    }

    bool check() override;

    [[nodiscard]] std::unique_ptr<Z3_IN_PLAJA::Solution> get_solution() const override;
    [[nodiscard]] inline z3::model get_model() const override { return optimizer.get_model(); }

    void dump_solver_to_file(const std::string& file_name) const override {
        std::stringstream solver_stream;
        solver_stream << optimizer << std::endl << "(check-sat)" << std::endl;
        PLAJA_UTILS::write_to_file(file_name, solver_stream.str(), false);
    }

    void print_bias_value() const {
        std::cout << "Bias = " << optimizer.get_model().eval(this->objective) << std::endl;
    }

    void dump_solver() const override {
        std::cout << optimizer << std::endl;
    }
};



#endif //SMT_OPTIMIZER_Z3_H
