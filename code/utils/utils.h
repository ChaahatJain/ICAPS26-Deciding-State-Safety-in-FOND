//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_UTILS_H
#define PLAJA_UTILS_H

#include <iostream>
#include <memory>
#include <typeinfo>
#include <vector>
#include <string>
#include "../assertions.h"

namespace PLAJA_UTILS {

    extern const std::hash<bool> hashBool;
    extern const std::hash<int> hashInteger;
    extern const std::hash<double> hashFloating;
    extern const std::hash<std::string> hashString;

    /******************************************************************************************************************/

    template<typename TargetType_t, typename NumericType_t> [[nodiscard]] inline bool is_non_narrowing_conversion(NumericType_t value) { return value <= std::numeric_limits<TargetType_t>::max(); }

    template<typename NumericType_t> [[nodiscard]] inline bool is_non_negative(NumericType_t value) { return std::numeric_limits<NumericType_t>::is_signed ? 0 <= value : true; }

    template<typename Type1_t, typename Type2_t> [[nodiscard]] inline constexpr bool is_static_type() { return std::is_same_v<Type1_t, Type2_t>; }

    template<typename DynamicType_t, typename StaticType_t> [[nodiscard]] inline bool is_dynamic_type(const StaticType_t& object) { return typeid(object) == typeid(DynamicType_t); }

    template<typename DynamicType_t, typename StaticType_t> [[nodiscard]] inline bool is_dynamic_ptr_type(const StaticType_t* object) { return object and is_dynamic_type<DynamicType_t, StaticType_t>(*object); }

    template<typename DynamicType_t, typename StaticType_t> [[nodiscard]] inline bool is_dynamic_type_or_null(const StaticType_t* object) { return not object or is_dynamic_type<DynamicType_t, StaticType_t>(*object); }

    template<typename DynamicType_t, typename StaticType_t> [[nodiscard]] inline bool is_derived_type(const StaticType_t& object) { 
        if constexpr(std::is_base_of_v<DynamicType_t, StaticType_t>) {
            return true;
        } else {
            return dynamic_cast<const DynamicType_t*>(&object) != nullptr;
        }
    }

    template<typename DynamicType_t, typename StaticType_t> [[nodiscard]] inline bool is_derived_ptr_type(const StaticType_t* object) { return dynamic_cast<const DynamicType_t*>(object); }

    template<typename DynamicType_t, typename StaticType_t> [[nodiscard]] inline bool is_derived_type_or_null(const StaticType_t* object) { return not object or is_derived_type<DynamicType_t, StaticType_t>(*object); }

    template<typename Type1_t, typename Type2_t> [[nodiscard]] inline bool is_null_iff(const Type1_t* ptr1, const Type2_t* ptr2) { return static_cast<bool>(ptr1) == static_cast<bool>(ptr2); }

    /* https://stackoverflow.com/questions/37972394/static-assert-for-unique-ptr-of-any-type [January 31, 2024]: */

    template<class T> struct is_unique_ptr: std::false_type {};

    template<class T, class D> struct is_unique_ptr<std::unique_ptr<T, D>>: std::true_type {};

    template<class T> constexpr bool is_ptr_type() { return std::is_pointer<T>::value or PLAJA_UTILS::is_unique_ptr<T>::value; }

    /******************************************************************************************************************/

    template<typename CastNumeric_t, typename BaseNumeric_t> [[nodiscard]] inline CastNumeric_t cast_numeric(BaseNumeric_t number) {
        PLAJA_ASSERT(is_non_narrowing_conversion<CastNumeric_t>(number))
        return static_cast<CastNumeric_t>(number);
    }

    template<typename Cast_t, typename Base_t> [[nodiscard]] inline const Cast_t& cast_ref(const Base_t& object) {
        PLAJA_ASSERT(is_derived_type<Cast_t>(object))
        return static_cast<const Cast_t&>(object);
    }

    template<typename Cast_t, typename Base_t> [[nodiscard]] inline Cast_t& cast_ref(Base_t& object) {
        PLAJA_ASSERT(is_derived_type<Cast_t>(object))
        return static_cast<Cast_t&>(object);
    }

    template<typename Cast_t, typename Base_t> [[nodiscard]] inline const Cast_t* cast_ptr(const Base_t* object) {
        PLAJA_ASSERT(is_derived_type_or_null<Cast_t>(object))
        return static_cast<const Cast_t*>(object);
    }

    template<typename Cast_t, typename Base_t> [[nodiscard]] inline Cast_t* cast_ptr(Base_t* object) {
        PLAJA_ASSERT(is_derived_type_or_null<Cast_t>(object))
        return static_cast<Cast_t*>(object);
    }

    template<typename Cast_t, typename Base_t> [[nodiscard]] inline std::unique_ptr<Cast_t> cast_unique(std::unique_ptr<Base_t>&& object) {
        PLAJA_ASSERT(is_derived_type_or_null<Cast_t>(object.get()))
        return std::unique_ptr<Cast_t>(static_cast<Cast_t*>(object.release()));
    }

    template<typename Cast_t, typename Base_t> [[nodiscard]] inline std::shared_ptr<Cast_t> cast_shared(std::shared_ptr<Base_t> object) {
        PLAJA_ASSERT(is_derived_type_or_null<Cast_t>(object.get()))
        return std::static_pointer_cast<Cast_t>(object);
    }

    template<typename Cast_t, typename Base_t> [[nodiscard]] inline const Cast_t* cast_ptr_if(const Base_t* object) { return dynamic_cast<const Cast_t*>(object); }

    template<typename Cast_t, typename Base_t> [[nodiscard]] inline Cast_t* cast_ptr_if(Base_t* object) { return dynamic_cast<Cast_t*>(object); }

    template<typename Cast_t, typename Base_t> [[nodiscard]] inline const Cast_t* cast_ref_if(const Base_t& object) { return cast_ptr_if<Cast_t>(&object); }

    template<typename Cast_t, typename Base_t> [[nodiscard]] inline Cast_t* cast_ref_if(Base_t& object) { return cast_ptr_if<Cast_t>(&object); }

    /******************************************************************************************************************/

    extern std::string extract_directory_path(const std::string& path_to_file);
    extern std::string extract_filename(const std::string& path_to_file, bool keep_extension);
    extern std::string extract_extension(const std::string& path_to_file);
    extern std::string remove_extension(const std::string& path_to_file);

    extern std::string join_path(const std::string& prefix, const std::vector<std::string>& sub_paths);

    extern bool is_relative_path(const std::string& path_to_file);
    extern std::string generate_relative_path(const std::string& path_to_file, const std::string& path_to_base);

    extern void make_directory(const std::string& path_to_dir);

    extern bool file_exists(const std::string& file_name);
    extern void copy_file(const std::string& path_to_file, const std::string& path_to_copy);
    extern void write_to_file(const std::string& file_name, const std::string& content, bool trunc);

    extern std::stringstream file_to_ss(const std::string& file_name);

    /******************************************************************************************************************/

    /** Convert floating precision/tolerance (1.0e^-n) to integer precision n and vice versa. */
    extern double precision_to_tolerance(unsigned int precision);
    extern unsigned int tolerance_to_precision(double tolerance);

    extern std::string to_string_with_precision(float value, unsigned int precision, bool trailing_zeros = true);
    extern std::string to_string_with_precision(float value, double precision, bool trailing_zeros = true);

    extern std::string to_string_with_precision(double value, unsigned int precision, bool trailing_zeros = true);
    extern std::string to_string_with_precision(double value, double precision, bool trailing_zeros = true);

    extern std::string to_red_string(const std::string& str);
    extern std::string to_green_string(const std::string& str);

    extern bool contains(const std::string& str, const std::string& sub_str);

    extern std::string erase(std::string str, const std::string& sub_str, int max_number_of_erase);
    extern std::string erase_all(std::string str, const std::string& sub_str);

    extern std::string replace(std::string str, const std::string& sub_str_old, const std::string& sub_str_new, int max_number_of_subs);
    extern std::string replace_all(std::string str, const std::string& sub_str_old, const std::string& sub_str_new);

    extern std::vector<std::string> split_into(const std::string& str, const std::string& delimiter, int max_number_of_splits);
    extern std::vector<std::string> split(const std::string& str, const std::string& delimiter);

    extern std::string string_f(const char* format, ...);

    extern char** strings_to_char_array(const std::vector<std::string>& str_vec);
    extern std::vector<std::string> char_array_to_strings(std::size_t size, char** char_array);
    extern void delete_char_array(std::size_t size, char** char_array);

    // struct String: public std::string { explicit String(const char* str, const std::string::allocator_type &alloc = std::string::allocator_type{}) noexcept; };

    // Frequently used strings
    extern const std::string emptyString;
    extern const std::string spaceString;
    extern const std::string lineBreakString;
    extern const std::string tabString;
    extern const std::string commaString;
    extern const std::string dotString;
    extern const std::string semicolonString;
    extern const std::string colonString;
    extern const std::string slashString;
    extern const std::string dashString;
    extern const std::string underscoreString;
    extern const std::string quoteString;
    extern const std::string equalString;
    extern const std::string lParenthesisString;
    extern const std::string rParenthesisString;

    extern const std::string resetColorStr;
    extern const std::string greenColorStr;
    extern const std::string redColorStr;

    /******************************************************************************************************************/

    extern void print(const std::string& str);
    extern void print_line(const std::string& str);

    template<typename t>
    inline void log(const t& log_content) { std::cout << log_content << std::endl; }

    template<typename t>
    inline void log_fast(const t& log_content) { std::cout << log_content << '\n'; }

#define PLAJA_FLOG_IF(FLAG, STR) if (FLAG) { PLAJA_UTILS::log_fast(STR); }
#define PLAJA_LOG(STR) (PLAJA_UTILS::log(STR));
#define PLAJA_LOG_IF(FLAG, STR) if (FLAG) { PLAJA_LOG(STR) }
#define PLAJA_LOG_IF_CONST(FLAG, STR) if constexpr (FLAG) { PLAJA_LOG(STR) }

#ifndef NDEBUG
#define PLAJA_LOG_DEBUG(STR) PLAJA_LOG(STR)
#define PLAJA_LOG_DEBUG_IF(FLAG, STR) PLAJA_LOG_IF(FLAG, STR)
#else
#define PLAJA_LOG_DEBUG(STR)
#define PLAJA_LOG_DEBUG_IF(FLAG, STR)
#endif

#ifdef RUNTIME_CHECKS
#define PLAJA_LOG_RUNTIME(STR) PLAJA_LOG(STR)
#define PLAJA_LOG_RUNTIME_IF(FLAG, STR) PLAJA_LOG_IF(FLAG, STR)
#else
#define PLAJA_LOG_RUNTIME(STR)
#define PLAJA_LOG_RUNTIME_IF(FLAG, STR)
#endif

}

#endif //PLAJA_UTILS_H
