//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_EXPRESSION_UTILS_H
#define PLAJA_EXPRESSION_UTILS_H

#include <memory>
#include "../../ast_forward_declarations.h"
#include "../../using_parser.h"

namespace PLAJA_EXPRESSION {

    [[nodiscard]] extern std::unique_ptr<Expression> make_bool(bool value);
    [[nodiscard]] extern std::unique_ptr<Expression> make_int(PLAJA::integer value);
    [[nodiscard]] extern std::unique_ptr<Expression> make_real(PLAJA::floating value);

    [[nodiscard]] extern std::unique_ptr<ConstantValueExpression> make_bool_value(bool value);
    [[nodiscard]] extern std::unique_ptr<ConstantValueExpression> make_int_value(PLAJA::integer value);
    [[nodiscard]] extern std::unique_ptr<ConstantValueExpression> make_real_value(PLAJA::floating value);

}

#endif //PLAJA_EXPRESSION_UTILS_H
