//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_FORWARD_Z3_H
#define PLAJA_FORWARD_Z3_H

namespace z3 {
    class context;

    class expr;

    template<typename T> class ast_vector_tpl;

    typedef ast_vector_tpl<expr> expr_vector;

    class solver;
}

#endif //PLAJA_FORWARD_Z3_H
