//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#include "tightening.h"

namespace BDD_IN_PLAJA {

    Tightening::Tightening(const std::vector<BDD_IN_PLAJA::Context::Addend>& addends_, int scalar_)
        : addends(addends_), scalar(scalar_) {
    }

    Tightening::~Tightening() = default;

    BDD Tightening::to_bdd(std::shared_ptr<Context> context, int vari, int bit, int c) const {
        int v = addends.size();
        int var = addends.at(vari).var_id;
        BDD n_index = context->get_var_bit(var, bit);
        if (vari == v - 1 and bit == context->get_num_bits(var) - 1) {
            BDD f = c < 0 ? !n_index : context->get_false();
            BDD g = (c + addends.at(vari).factor < 0) ? n_index : context->get_false();
            return f | g;
        } else if (vari == v - 1) {
            BDD f = c%2 == 0 ? ((!n_index) & to_bdd(context, 0, bit + 1, c/2)) : (!n_index) & to_bdd(context, 0, bit + 1, (c - 1)/2);
            BDD g = ((c + addends.at(vari).factor)%2 == 0) ? (n_index & to_bdd(context, 0, bit + 1, (c + addends.at(vari).factor)/2)) : (n_index & to_bdd(context, 0, bit + 1, (c + addends.at(vari).factor - 1)/2));
            return f | g;
        } else {
            BDD f = (!n_index) & to_bdd(context, vari + 1, bit, c);
            BDD g = n_index & to_bdd(context, vari + 1, bit, c + addends.at(vari).factor);
            return f | g;
        }
    };
    // Other methods (if any)
}
