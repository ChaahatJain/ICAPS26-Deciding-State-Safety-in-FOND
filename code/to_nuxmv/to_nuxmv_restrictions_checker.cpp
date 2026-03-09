//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2024) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "to_nuxmv_restrictions_checker.h"
#include "../parser/visitor/ast_visitor_const.h"
#include "../parser/ast/model.h"

class ToNuxmvRestrictionsChecker final: public AstVisitorConst {

public:
    explicit ToNuxmvRestrictionsChecker();
    ~ToNuxmvRestrictionsChecker() override;
    DELETE_CONSTRUCTOR(ToNuxmvRestrictionsChecker)

};

ToNuxmvRestrictionsChecker::ToNuxmvRestrictionsChecker() = default;

ToNuxmvRestrictionsChecker::~ToNuxmvRestrictionsChecker() = default;

namespace TO_NUXMV_RESTRICTIONS_CHECKER {

    void check(const Model& model) {
        ToNuxmvRestrictionsChecker checker;
        model.accept(&checker);
    }

}