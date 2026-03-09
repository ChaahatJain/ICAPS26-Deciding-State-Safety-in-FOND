//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_VARIABLE_DECLARATION_H
#define PLAJA_VARIABLE_DECLARATION_H

#include <memory>
#include "ast_element.h"
#include "commentable.h"
#include "expression/expression.h"

// forward declaration:
class DeclarationType;

class VariableDeclaration: public AstElement, public Commentable {
private:
    std::string name;
    std::unique_ptr<DeclarationType> declarationType;
    bool transient;
    std::unique_ptr<Expression> initialValue;

    VariableID_type id;
    VariableIndex_type index; // the model dependent state index
    std::size_t size; // the size of the elements in the domain, i.e., the size of the initial value array and otherwise 1 for scalars.

    // AutomatonIndex_type automatonIndex; // system index of corresponding automaton or AUTOMATON::invalid iff global (deprecated)

public:
    explicit VariableDeclaration(VariableID_type var_id);
    ~VariableDeclaration() override;

    /* Setter. */
    inline void set_name(std::string name_r) { name = std::move(name_r); }

    std::unique_ptr<DeclarationType> set_declarationType(std::unique_ptr<DeclarationType>&& decl_type);

    inline void set_transient(bool transient_v) { transient = transient_v; }

    std::unique_ptr<Expression> set_initialValue(std::unique_ptr<Expression>&& init_val);

    inline void set_id(VariableID_type id_v) { id = id_v; }

    inline void set_index(VariableIndex_type index_v) { index = index_v; }

    inline void set_size(std::size_t size_v) { size = size_v; }

    // inline void set_automatonIndex(AutomatonIndex_type automaton_index) { automatonIndex = automaton_index; }

    /* Getter. */
    [[nodiscard]] inline const std::string& get_name() const { return name; }

    inline DeclarationType* get_type() { return declarationType.get(); }

    [[nodiscard]] inline const DeclarationType* get_type() const { return declarationType.get(); }

    [[nodiscard]] inline bool is_transient() const { return transient; }

    inline Expression* get_initialValue() { return initialValue.get(); }

    [[nodiscard]] inline const Expression* get_initialValue() const { return initialValue.get(); }

    [[nodiscard]] inline VariableID_type get_id() const { return id; }

    [[nodiscard]] inline VariableIndex_type get_index() const { return index; }

    [[nodiscard]] inline std::size_t get_size() const { return size; }
    // inline AutomatonIndex_type get_automatonIndex() const { return automatonIndex; }

    [[nodiscard]] std::unique_ptr<VariableDeclaration> deep_copy() const;

    /* Override. */
    void accept(AstVisitor* ast_visitor) override;
    void accept(AstVisitorConst* ast_visitor) const override;

    [[nodiscard]] bool is_array() const;
    [[nodiscard]] bool is_scalar() const;
};

#endif //PLAJA_VARIABLE_DECLARATION_H
