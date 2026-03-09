//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#include "equation.h"

namespace BDD_IN_PLAJA {

    Equation::Equation(const std::vector<BDD_IN_PLAJA::Context::Addend>& addends_, int scalar_)
        : addends(addends_), scalar(scalar_) {
    }

    Equation::~Equation() = default;

    BDD Equation::to_bdd(std::shared_ptr<Context> context, int vari, int bit, int c) const {
        int v = addends.size();
        int var = addends.at(vari).var_id;
        BDD n_index = context->get_var_bit(var, bit);
        if (vari == v - 1 and bit == context->get_num_bits(var) - 1) {
            BDD f = c == 0 ? !n_index : context->get_false();
            BDD g = (c + addends.at(vari).factor == 0) ? n_index : context->get_false();
            return f | g;
        } else if (vari == v - 1) {
            BDD f = c%2 == 0 ? ((!n_index) & to_bdd(context, 0, bit + 1, c/2)) : context->get_false();
            BDD g = ((c + addends.at(vari).factor)%2 == 0) ? (n_index & to_bdd(context, 0, bit + 1, (c + addends.at(vari).factor)/2)) : context->get_false();
            return f | g;
        } else {
            BDD f = (!n_index) & to_bdd(context, vari + 1, bit, c);
            BDD g = n_index & to_bdd(context, vari + 1, bit, c + addends.at(vari).factor);
            return f | g;
        }
    };

    // Other methods (if any)
    // a    b   c
    // 1    1   n_index or !n_index
    // 0    1   n_index
    // 1    0   !n_index
    // 0    0   n_index & !n_index
}
