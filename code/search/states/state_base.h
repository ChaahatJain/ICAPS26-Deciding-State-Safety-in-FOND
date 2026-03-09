//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_STATE_BASE_H
#define PLAJA_STATE_BASE_H

#include <memory>
#include <string>
#include <vector>
#include "../../parser/ast/forward_ast.h"
#include "../../parser/using_parser.h"
#include "../../assertions.h"

// namespace STATE {
//    typedef std::size_t (*size_type)(const StateBase*);
//    typedef integer (*get_type)(const StateBase*, VariableIndex_type);
//    typedef void (*set_type)(StateBase*, VariableIndex_type, integer);
// }

class StateBase {

private:
    // important function pointers:
    // STATE::size_type sizeFun;
    // STATE::get_type getFun;
    // STATE::set_type setFun;

protected:
    FIELD_IF_RUNTIME_CHECKS(std::vector<bool> writtenTo;) // To check for double assignments in one atomic sequence at runtime.
    FIELD_IF_RUNTIME_CHECKS(static bool suppressWrittenTo;)
    FIELD_IF_DEBUG(static bool suppressIsValid;)

    StateBase& operator=(const StateBase& other);
    StateBase& operator=(StateBase&& other) = default;
    //
    explicit StateBase(std::size_t state_size);
    StateBase(const StateBase& state);
    StateBase(StateBase&& values_) = default;
public:
    virtual ~StateBase() = 0;

#ifdef RUNTIME_CHECKS

    inline static void suppress_written_to(bool suppress) { suppressWrittenTo = suppress; }

    /** Reset writtenTo to check for double assignments, e.g., when sequence is finished */
    inline void refresh_written_to() { writtenTo.assign(writtenTo.size(), false); }

    /** Reset writtenTo to check for double assignments, e.g., when sequence is finished */
    inline void refresh_written_to(VariableIndex_type state_index) {
        PLAJA_ASSERT(state_index < writtenTo.size())
        writtenTo[state_index] = false;
    }

    void update_written_to(VariableIndex_type index);

#else

    inline static void suppress_written_to(bool /*suppress*/) {}

    inline void refresh_written_to() {}

    inline void refresh_written_to(VariableIndex_type /*state_index*/) {}

    void update_written_to(VariableIndex_type /*index*/) {}

#endif

    FCT_IF_DEBUG(inline static void suppress_is_valid(bool suppress) { suppressIsValid = suppress; })

    FCT_IF_DEBUG([[nodiscard]] static bool is_valid(VariableIndex_type state_index, PLAJA::integer value);)

    FCT_IF_DEBUG([[nodiscard]] static bool is_valid(VariableIndex_type state_index, PLAJA::floating value);)

    FCT_IF_DEBUG([[nodiscard]] bool is_valid(VariableIndex_type state_index);)

    FCT_IF_DEBUG([[nodiscard]] bool is_valid() const;)

    /* interface */

    [[nodiscard]] virtual std::size_t get_int_state_size() const = 0;

    [[nodiscard]] virtual std::size_t get_floating_state_size() const = 0;

    [[nodiscard]] std::size_t size() const { return get_int_state_size() + get_floating_state_size(); }

    [[nodiscard]] virtual PLAJA::integer get_int(VariableIndex_type var) const = 0;

    [[nodiscard]] virtual PLAJA::floating get_float(VariableIndex_type var) const = 0;

    [[nodiscard]] std::unique_ptr<class Expression> get_value(const class LValueExpression& var) const;

    virtual void set_int(VariableIndex_type var, PLAJA::integer val) = 0;

    virtual void set_float(VariableIndex_type var, PLAJA::floating val) = 0;

    template<bool upd_writ_to = true>
    inline void assign_int(VariableIndex_type var, PLAJA::integer val) {
        if constexpr (upd_writ_to) { update_written_to(var); }
        set_int(var, val);
    }

    template<bool upd_writ_to = true>
    inline void assign_float(VariableIndex_type var, PLAJA::floating val) {
        if constexpr (upd_writ_to) { update_written_to(var); }
        set_float(var, val);
    }

    /* legacy */

    [[nodiscard]] PLAJA::floating operator[](VariableIndex_type var) const { return var < get_int_state_size() ? get_int(var) : get_float(var); }

    template<bool update_written_to = true>
    inline void assign(VariableIndex_type var, PLAJA::floating val) { if (var < get_int_state_size()) { assign_int<update_written_to>(var, static_cast<PLAJA::integer>(val)); } else { assign_float<update_written_to>(var, val); } }

    /* */

    bool operator==(const StateBase& other) const;

    inline bool operator!=(const StateBase& other) const { return not operator==(other); }

    [[nodiscard]] bool equal(const StateBase& other, VariableIndex_type state_index) const;

    [[nodiscard]] inline bool non_equal(const StateBase& other, VariableIndex_type state_index) const { return not equal(other, state_index); }

    [[nodiscard]] std::size_t hash() const;

    [[nodiscard]] bool lte(const StateBase& other, VariableIndex_type state_index) const;

    [[nodiscard]] bool lt(const StateBase& other, VariableIndex_type state_index) const;

    [[nodiscard]] virtual class StateValues to_state_values() const = 0;

    /******************************************************************************************************************/

    /* (debug) output */
private:

    /* Output auxiliaries. */
    [[nodiscard]] std::unique_ptr<Expression> loc_to_expr(const Automaton& instance) const;
    [[nodiscard]] std::unique_ptr<Expression> var_to_expr(const VariableDeclaration& var, const std::size_t* offset_index = nullptr) const;

public:

    [[nodiscard]] std::unique_ptr<Expression> to_condition(bool do_locs, const Model& model) const;

    /**
     * Generate AST value array for this state.
     * @param model if provided, the array contains variable names.
     */
    [[nodiscard]] std::unique_ptr<Expression> to_array_value(const Model* model = nullptr) const;
    [[nodiscard]] std::unique_ptr<Expression> locs_to_array_value(const Model& model) const; // Since also used by pa states.

    void dump(const Model* model = nullptr) const;
    std::string to_str(const Model* model = nullptr) const;
    std::string save_as_str() const;
#ifndef NDEBUG

    void dump(bool use_global_model) const;
    [[nodiscard]] std::string to_str(bool use_global_model) const;

    [[nodiscard]] std::unique_ptr<Expression> to_array_value(const std::vector<std::string>& loc_pattern, const std::vector<std::string>& var_pattern) const;
    void dump(const std::vector<std::string>& loc_pattern, const std::vector<std::string>& var_pattern) const;
    void dump(const std::vector<unsigned int>& tab_pattern) const;

    [[nodiscard]] PLAJA::integer loc_value_by_name(const std::string& var_name) const;
    [[nodiscard]] const std::string& loc_by_name(const std::string& var_name) const;
    [[nodiscard]] PLAJA::floating operator[](const std::string& var_name) const;

#endif

};

#endif //PLAJA_STATE_BASE_H
