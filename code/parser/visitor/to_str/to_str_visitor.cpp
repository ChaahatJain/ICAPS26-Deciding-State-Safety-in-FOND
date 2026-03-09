//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <sstream>
#include "to_str_visitor.h"
#include "../../../utils/utils.h"

#include "../../ast/iterators/model_iterator.h"
#include "../../ast/non_standard/free_variable_declaration.h"
#include "../../ast/non_standard/properties.h"
#include "../../ast/non_standard/reward_accumulation.h"
#include "../../ast/action.h"
#include "../../ast/assignment.h"
#include "../../ast/automaton.h"
#include "../../ast/composition.h"
#include "../../ast/composition_element.h"
#include "../../ast/constant_declaration.h"
#include "../../ast/destination.h"
#include "../../ast/edge.h"
#include "../../ast/location.h"
#include "../../ast/metadata.h"
#include "../../ast/property.h"
#include "../../ast/property_interval.h"
#include "../../ast/reward_bound.h"
#include "../../ast/reward_instant.h"
#include "../../ast/transient_value.h"
#include "../../ast/synchronisation.h"
#include "../../ast/variable_declaration.h"

#include "../../ast/expression/non_standard/comment_expression.h"
#include "../../ast/expression/non_standard/constant_array_access_expression.h"
#include "../../ast/expression/non_standard/location_value_expression.h"
#include "../../ast/expression/non_standard/predicate_abstraction_expression.h"
#include "../../ast/expression/non_standard/predicates_expression.h"
#include "../../ast/expression/non_standard/variable_value_expression.h"
#include "../../ast/expression/special_cases/linear_expression.h"
#include "../../ast/expression/special_cases/nary_expression.h"
#include "../../ast/expression/array_value_expression.h"
#include "../../ast/expression/array_access_expression.h"
#include "../../ast/expression/array_constructor_expression.h"
#include "../../ast/expression/array_value_expression.h"
#include "../../ast/expression/bool_value_expression.h"
#include "../../ast/expression/constant_expression.h"
#include "../../ast/expression/derivative_expression.h"
#include "../../ast/expression/distribution_sampling_expression.h"
#include "../../ast/expression/expectation_expression.h"
#include "../../ast/expression/filter_expression.h"
#include "../../ast/expression/free_variable_expression.h"
#include "../../ast/expression/integer_value_expression.h"
#include "../../ast/expression/ite_expression.h"
#include "../../ast/expression/path_expression.h"
#include "../../ast/expression/qfied_expression.h"
#include "../../ast/expression/real_value_expression.h"
#include "../../ast/expression/state_predicate_expression.h"
#include "../../ast/expression/unary_op_expression.h"

#include "../../ast/type/array_type.h"
#include "../../ast/type/bool_type.h"
#include "../../ast/type/bounded_type.h"
#include "../../ast/type/clock_type.h"
#include "../../ast/type/continuous_type.h"
#include "../../ast/type/int_type.h"
#include "../../ast/type/real_type.h"

#include "../../jani_words.h"

std::unique_ptr<std::string> ToStrVisitor::to_str(const AstElement& ast_element) {
    ToStrVisitor to_str_visitor;
    ast_element.accept(&to_str_visitor);
    return std::move(to_str_visitor.rlt);
}

void ToStrVisitor::dump(const AstElement& ast_element) {
    auto rlt = to_str(ast_element);
    PLAJA_ASSERT(rlt)
    std::cout << *rlt << std::endl;
}

ToStrVisitor::ToStrVisitor():
    rlt() {
}

ToStrVisitor::~ToStrVisitor() = default;

/**********************************************************************************************************************/

namespace TO_STR_VISITOR {

    inline std::string to_str(const AstElement* ast_element) {
        if (not ast_element) { return PLAJA_UTILS::emptyString; }
        auto rlt = ToStrVisitor::to_str(*ast_element);
        if (not rlt) { return PLAJA_UTILS::emptyString; }
        return *rlt;
    }

    inline void make_unique_str(std::unique_ptr<std::string>& rlt, const std::string& str) { rlt = std::make_unique<std::string>(str); }

    inline void make_unique_str(std::unique_ptr<std::string>& rlt, std::string&& str) { rlt = std::make_unique<std::string>(std::move(str)); }

    inline void append_str(std::unique_ptr<std::string>& rlt, const std::string& str) {
        PLAJA_ASSERT(rlt)
        rlt->append(str);
    }

    inline void append_ast_if(std::unique_ptr<std::string>& rlt, const AstElement* ast_element) {
        if (not ast_element) { return; }
        TO_STR_VISITOR::append_str(rlt, PLAJA_UTILS::spaceString + *ToStrVisitor::to_str(*ast_element));
    }

}

/**********************************************************************************************************************/

void ToStrVisitor::visit(const Action* action) { TO_STR_VISITOR::make_unique_str(rlt, action->get_name()); }

void ToStrVisitor::visit(const Assignment* assignment) {

    if (assignment->get_index() > 0) { return; } // not supported

    if (assignment->is_deterministic()) {
        TO_STR_VISITOR::make_unique_str(rlt, TO_STR_VISITOR::to_str(assignment->get_ref()) + " := " + TO_STR_VISITOR::to_str(assignment->get_value()));
    } else {
        auto rlt_tmp = TO_STR_VISITOR::to_str(assignment->get_ref());
        if (assignment->get_lower_bound()) { rlt_tmp = TO_STR_VISITOR::to_str(assignment->get_lower_bound()) + "<=" + rlt_tmp; }
        if (assignment->get_upper_bound()) { rlt_tmp += "<=" + TO_STR_VISITOR::to_str(assignment->get_upper_bound()); }
        TO_STR_VISITOR::make_unique_str(rlt, std::move(rlt_tmp));
    }

}

void ToStrVisitor::visit(const ConstantDeclaration* decl) {
    TO_STR_VISITOR::make_unique_str(rlt, "{ " + decl->get_name() + ":const, type: " + TO_STR_VISITOR::to_str(decl->get_type()) + ", value: " + TO_STR_VISITOR::to_str(decl->get_value()) + "}");
}

void ToStrVisitor::visit(const Destination* destination) {
    TO_STR_VISITOR::make_unique_str(rlt, "{ loc=" + destination->get_location_name() + ", p=" + TO_STR_VISITOR::to_str(destination->get_probabilityExpression()) + ", ");

    const auto num_assignments = destination->get_number_assignments();
    if (num_assignments > 0) {

        auto rlt_tmp = ToStrVisitor::to_str(*destination->get_assignment(0));

        for (std::size_t index = 1; index < num_assignments; ++index) {

            TO_STR_VISITOR::append_str(rlt_tmp, " & " + TO_STR_VISITOR::to_str(destination->get_assignment(index)));

        }

        TO_STR_VISITOR::append_str(rlt, *rlt_tmp);

    }
    //JSON_ITERATOR_IF(destination->assignmentIterator(), JANI::ASSIGNMENTS, destination->get_number_assignments())

    TO_STR_VISITOR::append_str(rlt, " }");
}

void ToStrVisitor::visit(const Edge* edge) {
    PLAJA_ASSERT(not edge->get_rateExpression())

    TO_STR_VISITOR::make_unique_str(rlt, "{ loc=" + edge->get_location_name() + ", label=" + edge->get_action_name() + PLAJA_UTILS::commaString + PLAJA_UTILS::lineBreakString);

    if (edge->get_guardExpression()) {
        TO_STR_VISITOR::append_str(rlt, "\t g: " + TO_STR_VISITOR::to_str(edge->get_guardExpression()) + PLAJA_UTILS::lineBreakString);
    }

    const auto num_destinations = edge->get_number_destinations();
    if (num_destinations > 0) {

        auto rlt_tmp = ToStrVisitor::to_str(*edge->get_destination(0));
        TO_STR_VISITOR::append_str(rlt_tmp, PLAJA_UTILS::lineBreakString);

        for (std::size_t index = 1; index < num_destinations; ++index) {

            TO_STR_VISITOR::append_str(rlt_tmp, "| " + TO_STR_VISITOR::to_str(edge->get_destination(index)));
            TO_STR_VISITOR::append_str(rlt_tmp, PLAJA_UTILS::lineBreakString);

        }

        TO_STR_VISITOR::append_str(rlt, *rlt_tmp);

    }

    TO_STR_VISITOR::append_str(rlt, "}");
}

/** expressions *******************************************************************************************************/

void ToStrVisitor::visit(const ArrayAccessExpression* exp) {
    TO_STR_VISITOR::make_unique_str(rlt, TO_STR_VISITOR::to_str(exp->get_accessedArray()) + "[" + TO_STR_VISITOR::to_str(exp->get_index()) + "]");
}

void ToStrVisitor::visit(const ArrayValueExpression* exp) {
    std::stringstream ss;
    ss << "[";
    if (exp->get_number_elements() > 0) {
        auto it = exp->init_element_it();
        ss << TO_STR_VISITOR::to_str(it());
        for (++it; !it.end(); ++it) { ss << PLAJA_UTILS::commaString << PLAJA_UTILS::spaceString << TO_STR_VISITOR::to_str(it()); }
    }
    ss << "]";

    return TO_STR_VISITOR::make_unique_str(rlt, ss.str());
}

void ToStrVisitor::visit(const BinaryOpExpression* exp) {
    TO_STR_VISITOR::make_unique_str(rlt, "(" + TO_STR_VISITOR::to_str(exp->get_left()) + ") " + BinaryOpExpression::binary_op_to_str(exp->get_op()) + " (" + TO_STR_VISITOR::to_str(exp->get_right()) + ")");
}

void ToStrVisitor::visit(const BoolValueExpression* exp) { TO_STR_VISITOR::make_unique_str(rlt, std::to_string(exp->get_value())); }

void ToStrVisitor::visit(const ConstantExpression* exp) { TO_STR_VISITOR::make_unique_str(rlt, exp->get_name()); }

void ToStrVisitor::visit(const IntegerValueExpression* exp) { TO_STR_VISITOR::make_unique_str(rlt, std::to_string(exp->get_value())); }

void ToStrVisitor::visit(const ITE_Expression* exp) {
    TO_STR_VISITOR::make_unique_str(rlt, "(" + TO_STR_VISITOR::to_str(exp->get_condition()) + ") ? (" + TO_STR_VISITOR::to_str(exp->get_consequence()) + ") : (" + TO_STR_VISITOR::to_str(exp->get_alternative()) + ")");
}

void ToStrVisitor::visit(const RealValueExpression* exp) {
    std::stringstream ss;
    ss << exp->get_value();
    TO_STR_VISITOR::make_unique_str(rlt, ss.str());
}

void ToStrVisitor::visit(const UnaryOpExpression* exp) {
    TO_STR_VISITOR::make_unique_str(rlt, UnaryOpExpression::unary_op_to_str(exp->get_op()) + "(" + TO_STR_VISITOR::to_str(exp->get_operand()) + ")");
}

void ToStrVisitor::visit(const VariableExpression* exp) { TO_STR_VISITOR::make_unique_str(rlt, exp->get_name()); }

// special cases:

void ToStrVisitor::visit(const LinearExpression* exp) { exp->to_standard()->accept(this); }

void ToStrVisitor::visit(const NaryExpression* exp) { exp->to_standard()->accept(this); }

// non-standard:

void ToStrVisitor::visit(const ConstantArrayAccessExpression* exp) { TO_STR_VISITOR::make_unique_str(rlt, exp->get_accessed_array()->get_name() + "[" + TO_STR_VISITOR::to_str(exp->get_index()) + "]"); }

void ToStrVisitor::visit(const CommentExpression* exp) { TO_STR_VISITOR::make_unique_str(rlt, exp->get_comment()); }

void ToStrVisitor::visit(const LocationValueExpression* exp) { TO_STR_VISITOR::make_unique_str(rlt, exp->get_automaton_name() + " = " + exp->get_location_name()); }

void ToStrVisitor::visit(const VariableValueExpression* exp) { TO_STR_VISITOR::make_unique_str(rlt, exp->get_var()->get_name() + " = " + TO_STR_VISITOR::to_str(exp->get_val())); }

/** types *************************************************************************************************************/

void ToStrVisitor::visit(const ArrayType* type) {
    PLAJA_ASSERT(type->get_element_type())
    TO_STR_VISITOR::make_unique_str(rlt, "array(" + *ToStrVisitor::to_str(*type->get_element_type()) + ")");
}

void ToStrVisitor::visit(const BoolType* /*type*/) {
    TO_STR_VISITOR::make_unique_str(rlt, DeclarationType::kind_to_str(DeclarationType::Bool));
}

void ToStrVisitor::visit(const ClockType* /*type*/) { TO_STR_VISITOR::make_unique_str(rlt, DeclarationType::kind_to_str(DeclarationType::Clock)); }

void ToStrVisitor::visit(const ContinuousType* /*type*/) { TO_STR_VISITOR::make_unique_str(rlt, DeclarationType::kind_to_str(DeclarationType::Continuous)); }

void ToStrVisitor::visit(const IntType* /*type*/) { TO_STR_VISITOR::make_unique_str(rlt, DeclarationType::kind_to_str(DeclarationType::Int)); }

void ToStrVisitor::visit(const RealType* /*type*/) { TO_STR_VISITOR::make_unique_str(rlt, DeclarationType::kind_to_str(DeclarationType::Real)); }

void ToStrVisitor::visit(const LocationType* /*type*/) { TO_STR_VISITOR::make_unique_str(rlt, DeclarationType::kind_to_str(DeclarationType::Location)); }
