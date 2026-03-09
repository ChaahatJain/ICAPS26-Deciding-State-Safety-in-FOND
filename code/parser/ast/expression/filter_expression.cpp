//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <unordered_map>
#include "filter_expression.h"
#include "../../visitor/ast_visitor.h"
#include "../../visitor/ast_visitor_const.h"
#include "../type/bool_type.h"
#include "../type/int_type.h"
#include "../type/real_type.h"

FilterExpression::FilterExpression(FilterFun fun):
    fun(fun)
    , values()
    , states() {}

FilterExpression::~FilterExpression() = default;

// static:

namespace FILTER_EXPRESSION {
    const std::string filterFunToStr[] { "min", "max", "sum", "avg", "count", "∀", "∃", "argmin", "argmax", "values" }; // NOLINT(cert-err58-cpp)
    const std::unordered_map<std::string, FilterExpression::FilterFun> strToFilterFun { { "min",    FilterExpression::MIN },
                                                                                        { "max",    FilterExpression::MAX },
                                                                                        { "sum",    FilterExpression::SUM },
                                                                                        { "avg",    FilterExpression::AVG },
                                                                                        { "count",  FilterExpression::COUNT },
                                                                                        { "∀",      FilterExpression::FORALL },
                                                                                        { "∃",      FilterExpression::EXISTS },
                                                                                        { "argmin", FilterExpression::ARGMIN },
                                                                                        { "argmax", FilterExpression::ARGMAX },
                                                                                        { "values", FilterExpression::VALUES } }; // NOLINT(cert-err58-cpp)
}

const std::string& FilterExpression::filter_fun_to_str(FilterExpression::FilterFun fun) { return FILTER_EXPRESSION::filterFunToStr[fun]; }

std::unique_ptr<FilterExpression::FilterFun> FilterExpression::str_to_filter_fun(const std::string& fun_str) {
    auto it = FILTER_EXPRESSION::strToFilterFun.find(fun_str);
    if (it == FILTER_EXPRESSION::strToFilterFun.end()) { return nullptr; }
    else { return std::make_unique<FilterFun>(it->second); }
}

// override:

const DeclarationType* FilterExpression::determine_type() {
    if (resultType) { return resultType.get(); }
    else {
        switch (fun) {
            case FilterFun::MIN:
            case FilterFun::MAX:
            case FilterFun::SUM:
            case FilterFun::AVG: { return (resultType = std::make_unique<RealType>()).get(); }
            case FilterFun::COUNT: { return (resultType = std::make_unique<IntType>()).get(); }
            case FilterFun::FORALL:
            case FilterFun::EXISTS: { return (resultType = std::make_unique<BoolType>()).get(); }
            default: { return nullptr; }
        }
    }
}

void FilterExpression::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void FilterExpression::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }

std::unique_ptr<PropertyExpression> FilterExpression::deepCopy_PropExp() const {
    std::unique_ptr<FilterExpression> copy(new FilterExpression(fun));
    if (resultType) { copy->resultType = resultType->deep_copy_decl_type(); }
    if (values) { copy->set_values(values->deepCopy_PropExp()); }
    if (states) { copy->set_states(states->deepCopy_PropExp()); }
    return copy;
}