//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_LINEAR_EXPRESSION_H
#define PLAJA_LINEAR_EXPRESSION_H

#include <list>
#include <map>
#include "../binary_op_expression.h"
#include "../forward_expression.h"
#include "special_case_expression.h"

/* Special case of BinaryOpExpression */
class LinearExpression final: public SpecialCaseExpression {
    friend class ToLinearExp; //
    friend class SplitEquality; //
    friend class PushDownNeg; //
    friend class ToMarabouVisitor; //
    friend class ToVeritasVisitor; // 

private:
    struct Addend {
        friend LinearExpression;

    private:
        std::unique_ptr<ConstantValueExpression> factor;
        std::unique_ptr<VariableExpression> var;

        [[nodiscard]] inline const ConstantValueExpression* get_factor() const { return factor.get(); }

        [[nodiscard]] inline const VariableExpression* get_var_expr() const { return var.get(); }

        [[nodiscard]] inline VariableExpression* get_var_expr() { return var.get(); }

        [[nodiscard]] VariableID_type get_var_id() const;

        [[nodiscard]] VariableIndex_type get_var_index() const;

        [[nodiscard]] std::unique_ptr<Expression> to_standard() const;

    public:
        Addend(std::unique_ptr<ConstantValueExpression>&& factor, std::unique_ptr<VariableExpression>&& var_expr);
        ~Addend();
        DELETE_CONSTRUCTOR(Addend)

        [[nodiscard]] bool is_integer() const;

        bool operator==(const Addend& other) const;

        inline bool operator!=(const Addend& other) const { return not operator==(other); }
    };

private:
    bool static is_linear_op(BinaryOpExpression::BinaryOp op);
    [[nodiscard]] static BinaryOpExpression::BinaryOp invert_op(BinaryOpExpression::BinaryOp op);

    std::map<VariableIndex_type, std::unique_ptr<Addend>> addends;
    BinaryOpExpression::BinaryOp op;
    std::unique_ptr<ConstantValueExpression> scalar; // right-hand scalar (like in Marabou)

    // internals
    [[nodiscard]] const Addend* get_addend(VariableIndex_type var_index) const;
    [[nodiscard]] Addend* get_addend(VariableIndex_type var_index);
    void to_integer(); // store factors as scalars as integers if possible
    void set_trivially_true();
    void set_trivially_false();

    // construction:
    void add_addend(std::unique_ptr<ConstantValueExpression>&& factor, std::unique_ptr<VariableExpression>&& var);
    void remove_addend(std::map<VariableIndex_type, std::unique_ptr<Addend>>::iterator addend_it);
    void remove_addend(VariableIndex_type var_index);
    void add_to_scalar(const ConstantValueExpression& scalar_);
    void add(const LinearExpression& lin_expr);
    [[nodiscard]] bool is_addends_integer() const;

    // setter:
    inline void set_op(BinaryOpExpression::BinaryOp op_) { op = op_; }

    void set_scalar(std::unique_ptr<ConstantValueExpression>&& scalar_);
    [[nodiscard]] bool is_scalar_integer() const;

    friend class ToLinearIntExp; // for now only to be used by ToLinearIntExp internally
    explicit LinearExpression(BinaryOpExpression::BinaryOp linear_op);
public:
    ~LinearExpression() override;
    DELETE_CONSTRUCTOR(LinearExpression)

    static std::unique_ptr<Expression> construct_bound(std::unique_ptr<Expression>&& var, std::unique_ptr<Expression>&& value, BinaryOpExpression::BinaryOp op);

    // getter:
    [[nodiscard]] inline std::size_t get_number_addends() const { return addends.size(); }

    [[nodiscard]] inline ConstantValueExpression* get_factor(VariableIndex_type var_index) const {
        const auto* addend = get_addend(var_index);
        return addend ? addend->factor.get() : nullptr;
    }

    [[nodiscard]] inline BinaryOpExpression::BinaryOp get_op() const { return op; }

    [[nodiscard]] inline const ConstantValueExpression* get_scalar() const { return scalar.get(); }

    [[nodiscard]] inline bool is_integer() const { return is_addends_integer() and is_scalar_integer(); }

    [[nodiscard]] inline bool is_linear_sum() const { return op == BinaryOpExpression::BinaryOp::PLUS; }

    [[nodiscard]] inline bool is_linear_assignment() const { return is_linear_sum(); }

    [[nodiscard]] inline bool is_linear_constraint() const { return op != BinaryOpExpression::BinaryOp::PLUS; }

    [[nodiscard]] bool is_bound() const;

    /* iterator *******************************************************************************************************/

    template<typename AddendVec_t, typename Var_t>
    class AddendIterator {
        friend LinearExpression;

    private:
        std::map<VariableIndex_type, std::unique_ptr<Addend>>::const_iterator it;
        std::map<VariableIndex_type, std::unique_ptr<Addend>>::const_iterator itEnd;

        explicit AddendIterator(AddendVec_t& addends):
            it(addends.cbegin())
            , itEnd(addends.cend()) {}

    public:
        inline void operator++() { ++it; }

        [[nodiscard]] inline bool end() const { return it == itEnd; }

        [[nodiscard]] inline const Addend& addend() const { return *it->second; }

        [[nodiscard]] inline const ConstantValueExpression* factor() const { return addend().get_factor(); }

        [[nodiscard]] inline Var_t* var() const { return it->second->get_var_expr(); }

        [[nodiscard]] inline VariableID_type var_id() const { return addend().get_var_id(); }

        [[nodiscard]] inline VariableIndex_type var_index() const { return addend().get_var_index(); }

        [[nodiscard]] inline std::unique_ptr<Expression> to_standard() const { return addend().to_standard(); }

    };

    using AddendConstIterator = AddendIterator<const std::map<VariableIndex_type, std::unique_ptr<Addend>>, const VariableExpression>;

    [[nodiscard]] inline AddendConstIterator addendIterator() const { return AddendConstIterator(addends); }

    using AddendNonConstIterator = AddendIterator<std::map<VariableIndex_type, std::unique_ptr<Addend>>, VariableExpression>;

    [[nodiscard]] inline AddendNonConstIterator addendIterator() { return AddendNonConstIterator(addends); }

    /* iterator *******************************************************************************************************/

    void to_nf(bool as_predicate); // Rewrite to standard form.
    inline void to_nf() { to_nf(false); } // Rewrite to (complementary-)equivalent constraint.
    inline void to_predicate_nf() { to_nf(true); } // Rewrite to (complementary-)equivalent constraint.

    // override:
    [[nodiscard]] std::unique_ptr<Expression> to_standard() const override;
    PLAJA::integer evaluateInteger(const StateBase* state) const override;
    PLAJA::floating evaluateFloating(const StateBase* state) const override;
    [[nodiscard]] bool is_constant() const override;
    bool equals(const Expression* exp) const override;
    [[nodiscard]] std::size_t hash() const override;
    const DeclarationType* determine_type() override;
    void accept(AstVisitor* ast_visitor) override;
    void accept(AstVisitorConst* ast_visitor) const override;
    [[nodiscard]] std::unique_ptr<Expression> deepCopy_Exp() const override;
    //
    [[nodiscard]] std::unique_ptr<LinearExpression> deep_copy() const;
};

#endif //PLAJA_LINEAR_EXPRESSION_H
