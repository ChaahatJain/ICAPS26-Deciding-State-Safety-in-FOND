//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//
// Encoding inequations Sum_i=1^n a_i*x_i < a_0
#ifndef PLAJA_BDD_BOUND_H
#define PLAJA_BDD_BOUND_H
#include "../bdd_context.h"
#include <vector>
namespace BDD_IN_PLAJA {
    class Tightening final {
        private:
            std::vector<BDD_IN_PLAJA::Context::Addend> addends;
            int scalar;

        public:
            Tightening(const std::vector<BDD_IN_PLAJA::Context::Addend>& addends_, int scalar_);
            ~Tightening();
            DELETE_CONSTRUCTOR(Tightening)

            BDD to_bdd(std::shared_ptr<Context> context) {
                for (auto addend: addends) {
                    scalar -= addend.factor*context->get_lower_bound(addend.var_id);
                } 
                return to_bdd(context, 0, 0, -scalar);
            }
            BDD to_bdd(std::shared_ptr<Context> context, int var, int bit, int scalar) const;

            void dump() {
                std::cout << "Bound Constraint: ";
                for (auto i = 0; i < addends.size(); ++i) {
                    auto addend = addends.at(i);
                    std::cout << addend.factor << "*x" << addend.var_id;
                    if (i != addends.size() - 1) {
                        std::cout << " + ";
                    }
                }
                std::cout << " < " << scalar << std::endl; 
            }
    };
}
#endif //PLAJA_BDD_BOUND_H
