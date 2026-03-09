//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_CONTEXT_Z3_H
#define PLAJA_CONTEXT_Z3_H

#include <unordered_map>
#include <z3++.h>
#include "../../utils/utils.h"
#include "../using_search.h"
#include "base/smt_context.h"
#include "forward_smt_z3.h"
#include "using_smt.h"

namespace Z3_IN_PLAJA {

    /* Cast (to remove ambiguity of int_val() calls). */

    template<typename CastNumeric_t, typename BaseNumeric_t>
    [[nodiscard]] inline CastNumeric_t cast(BaseNumeric_t val) { return PLAJA_UTILS::cast_numeric<CastNumeric_t>(val); }

    // template<typename BaseNumeric_t>
    // [[nodiscard]] inline int signed cast_signed(BaseNumeric_t val) { return cast<int signed>(val); }
    // #define Z3_CAST_SIGNED(int_val) static_cast<int signed>(int_val)

    // template<typename BaseNumeric_t>
    // [[nodiscard]] inline int unsigned cast_unsigned(BaseNumeric_t val) { return cast<int unsigned>(val); }
    // #define Z3_CAST_UNSIGNED(int_val) static_cast<int unsigned>(int_val)

    template<typename BaseNumeric_t>
    [[nodiscard]] inline int64_t cast_long_signed(BaseNumeric_t val) { return cast<int64_t>(val); }
    // #define Z3_CAST_LONG_SIGNED(int_val) static_cast<int64_t>(int_val)

    // template<typename BaseNumeric_t>
    // [[nodiscard]] inline uint64_t cast_long_unsigned(BaseNumeric_t val) { return cast<uint64_t>(val); }
    // #define Z3_CAST_LONG_UNSIGNED(int_val) static_cast<uint64_t>(int_val)

    /* Int and real. */

    [[nodiscard]] inline z3::expr z3_to_int(z3::context& c, PLAJA::integer val) { return c.int_val(cast_long_signed(val)); }
    // #define Z3_TO_INT(context, value) context.int_val(Z3_CAST_LONG_SIGNED(value))

    [[nodiscard]] inline z3::expr z3_to_real(z3::context& c, PLAJA::floating val, unsigned int precision) { return c.real_val(PLAJA_UTILS::to_string_with_precision(val, precision).c_str()); }
    // #define Z3_TO_REAL(context, value) context.real_val(std::to_string(value).c_str())

    /* Bool. */

    [[nodiscard]] inline z3::expr bool_to_int(const z3::expr& condition) {
        z3::context& c = condition.ctx();
        return z3::ite(condition, c.int_val(1), c.int_val(0));
    }

    [[nodiscard]] inline z3::expr bool_to_int_if(const z3::expr& expr_in_z3) { return expr_in_z3.is_bool() ? bool_to_int(expr_in_z3) : expr_in_z3; }

    /* Check for null-expression. */

    [[nodiscard]] inline bool expr_is(const z3::expr& e) { return (e.operator bool()); }
    // #define Z3_EXPR_IS(e) ((e).operator bool())

    [[nodiscard]] inline bool expr_is_none(const z3::expr& e) { return not expr_is(e); }
    // #define Z3_EXPR_IS_NONE(e) (!(e).operator bool())

#define Z3_EXPR_ADD(e, e_add, OP)\
    if (Z3_IN_PLAJA::expr_is(e)) { (e) = (e) OP (e_add); } \
    else { (e) = (e_add); }

#define Z3_EXPR_ADD_IF(e, e_add, OP) if (Z3_IN_PLAJA::expr_is(e_add)) { Z3_EXPR_ADD(e, e_add, OP) }

    /* Extern. */
    extern const std::string intVarPrefix;
    extern const std::string boolVarPrefix;
    extern const std::string realVarPrefix;
    extern const std::string intArrayVarPrefix;
    extern const std::string boolArrayVarPrefix;
    extern const std::string realArrayVarPrefix;
    extern const std::string auxiliaryVarPrefix;
    extern const std::string freeVarPrefix;

    class Context final: public PLAJA::SmtContext {

    private:
        z3::context context;
        z3::sort IntSort;
        z3::sort BoolSort;
        z3::sort RealSort;
        z3::sort IntArraySort;
        z3::sort BoolArraySort;
        z3::sort RealArraySort;
        VarId_type nextVar;

        std::unordered_map<VarId_type, z3::expr> variableInformation;

        std::unordered_map<VarId_type, z3::expr> variableBounds;

    public:
        Context();
        ~Context() override;

        DELETE_CONSTRUCTOR(Context)

        inline const z3::context& operator()() const { return context; }

        inline z3::context& operator()() { return context; }

        inline bool operator==(const Context& other) const { return context == other(); }

        inline bool operator!=(const Context& other) const { return not operator==(other); }

        [[nodiscard]] inline std::size_t get_number_of_variables() { return variableInformation.size(); }

        [[nodiscard]] inline bool exists(VarId_type var) const {
            PLAJA_ASSERT(var < nextVar)
            return variableInformation.count(var);
        }

        inline VarId_type add_var(const z3::expr& var) {
            auto var_id = nextVar++;
            variableInformation.emplace(var_id, var);
            return var_id;
        }

        inline VarId_type add_int_var(const std::string& prefix = intVarPrefix) { return add_var(context.int_const((prefix + PLAJA_UTILS::underscoreString + std::to_string(nextVar)).c_str())); }

        inline VarId_type add_bool_var(const std::string& prefix = boolVarPrefix) { return add_var(context.bool_const((prefix + PLAJA_UTILS::underscoreString + std::to_string(nextVar)).c_str())); }

        inline VarId_type add_real_var(const std::string& prefix = realVarPrefix) { return add_var(context.real_const((prefix + PLAJA_UTILS::underscoreString + std::to_string(nextVar)).c_str())); }

        inline VarId_type add_int_array_var(const std::string& prefix = intArrayVarPrefix) { return add_var(context.constant((prefix + PLAJA_UTILS::underscoreString + std::to_string(nextVar)).c_str(), IntArraySort)); }

        inline VarId_type add_bool_array_var(const std::string& prefix = boolArrayVarPrefix) { return add_var(context.constant((prefix + PLAJA_UTILS::underscoreString + std::to_string(nextVar)).c_str(), BoolArraySort)); }

        inline VarId_type add_real_array_var(const std::string& prefix = realArrayVarPrefix) { return add_var(context.constant((prefix + PLAJA_UTILS::underscoreString + std::to_string(nextVar)).c_str(), RealArraySort)); }

        [[nodiscard]] inline const z3::expr& get_var(VarId_type var) const {
            PLAJA_ASSERT(exists(var))
            return variableInformation.at(var);
        }

        [[nodiscard]] inline z3::expr int_val(PLAJA::integer val) { return Z3_IN_PLAJA::z3_to_int(context, val); }

        [[nodiscard]] inline z3::expr bool_val(bool val) { return context.bool_val(val); }

        // [[nodiscard]] inline z3::expr real_val(PLAJA::floating val) { return context.real_val(std::to_string(val).c_str()); }

        [[nodiscard]] inline z3::expr real_val(PLAJA::floating val, unsigned int precision) { return Z3_IN_PLAJA::z3_to_real(context, val, precision); }

        std::unique_ptr<Z3_IN_PLAJA::Constraint> trivially_true();

        std::unique_ptr<Z3_IN_PLAJA::Constraint> trivially_false();

        /* Bounds. */

        inline void add_bound(VarId_type var, z3::expr bound) {
            PLAJA_ASSERT(exists(var))
            PLAJA_ASSERT(not variableBounds.count(var))
            variableBounds.emplace(var, std::move(bound));
        }

        [[nodiscard]] inline bool has_bound(VarId_type var) const {
            PLAJA_ASSERT(exists(var))
            return variableBounds.count(var);
        }

        inline void update_bound(VarId_type var, z3::expr bound) {
            PLAJA_ASSERT(has_bound(var))
            auto it = variableBounds.find(var);
            PLAJA_ASSERT(it != variableBounds.cend())
            it->second = std::move(bound);
        }

        inline void delete_bound(VarId_type var) {
            PLAJA_ASSERT(has_bound(var))
            variableBounds.erase(var);
        }

        [[nodiscard]] inline const z3::expr& get_bound(VarId_type var) const {
            PLAJA_ASSERT(has_bound(var))
            return variableBounds.at(var);
        }

        [[nodiscard]] inline const z3::expr* get_bound_if(VarId_type var) const {
            PLAJA_ASSERT(exists(var))
            const auto it = variableBounds.find(var);
            return it != variableBounds.cend() ? &it->second : nullptr;
        }

        /* Iterator. */

        class VariableIterator {
            friend Z3_IN_PLAJA::Context;
        private:
            std::unordered_map<VarId_type, z3::expr>::const_iterator it;
            std::unordered_map<VarId_type, z3::expr>::const_iterator itEnd;

            explicit VariableIterator(const std::unordered_map<VarId_type, z3::expr>& var_info):
                it(var_info.cbegin())
                , itEnd(var_info.cend()) {}

        public:
            ~VariableIterator() = default;
            DELETE_CONSTRUCTOR(VariableIterator)

            inline void operator++() { ++it; }

            [[nodiscard]] inline bool end() const { return it == itEnd; }

            [[nodiscard]] inline VarId_type var_id() const { return it->first; }

            [[nodiscard]] inline z3::expr var() const { return it->second; }

        };

        [[nodiscard]] inline VariableIterator variableIterator() const { return VariableIterator(variableInformation); }

    };

}

#endif //PLAJA_CONTEXT_Z3_H
