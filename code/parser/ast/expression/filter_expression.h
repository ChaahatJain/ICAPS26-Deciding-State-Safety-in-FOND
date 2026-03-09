//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_FILTER_EXPRESSION_H
#define PLAJA_FILTER_EXPRESSION_H

#include <memory>
#include "property_expression.h"

class FilterExpression final: public PropertyExpression {
public:
    enum FilterFun { MIN, MAX, SUM, AVG, COUNT, FORALL, EXISTS, ARGMIN, ARGMAX, VALUES };
private:
    FilterFun fun;
    std::unique_ptr<PropertyExpression> values;
    std::unique_ptr<PropertyExpression> states;
public:
    explicit FilterExpression(FilterFun fun);
    ~FilterExpression() override;

    // static:
    static const std::string& filter_fun_to_str(FilterFun fun);
    static std::unique_ptr<FilterFun> str_to_filter_fun(const std::string& fun_str);

    // setter
    [[maybe_unused]] inline void set_fun(FilterFun filter_fun) { fun = filter_fun;}
    inline void set_values(std::unique_ptr<PropertyExpression>&& values_) { values = std::move(values_); }
    inline void set_states(std::unique_ptr<PropertyExpression>&& states_) { states = std::move(states_); }

    // getter:
    [[nodiscard]] inline FilterFun get_fun() const { return fun; }
    [[nodiscard]] inline PropertyExpression* get_values() { return values.get(); }
    [[nodiscard]] inline const PropertyExpression* get_values() const { return values.get(); }
    [[nodiscard]] inline PropertyExpression* get_states() { return states.get(); }
    [[nodiscard]] inline const PropertyExpression* get_states() const { return states.get(); }

    // override:
    const DeclarationType* determine_type() override;
    void accept(AstVisitor* astVisitor) override;
	void accept(AstVisitorConst* astVisitor) const override;
    [[nodiscard]] std::unique_ptr<PropertyExpression> deepCopy_PropExp() const override;
};

#endif //PLAJA_FILTER_EXPRESSION_H
