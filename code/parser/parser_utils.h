//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PARSER_UTILS_H
#define PLAJA_PARSER_UTILS_H

// minimal include, include in source files as required
#include <iostream>
#include <json.hpp> // Need to include in all parsing files anyways.
#include "ast/forward_ast.h"
#include "jani_words.h"

namespace PARSER {

    /*
     * Allow to parse substructure independent.
     * If so, some correctness implications no longer hold and must be checked by the method for the substructure.
     */
    constexpr bool decoupledParsing = false;

    extern const nlohmann::json emptyJson;
    // strings used frequently, e.g. in parser exceptions
    extern const std::string duplicate_name;
    extern const std::string unknown_name;
    extern const std::string unexpected_element;
    extern const std::string unmatched_jani_structure;
    extern const std::string not_supported_by_model_type;
    extern const std::string at_least_one_element;
    extern const std::string invalid;

}

namespace PARSER {

    [[noreturn]] extern void throw_parser_exception(const std::string& input, const nlohmann::json& context, const std::string& remark);

    inline void throw_parser_exception_if(bool condition, const std::string& input, const nlohmann::json& context, const std::string& remark) { if (condition) { throw_parser_exception(input, context, remark); } }

    [[noreturn]] inline void throw_parser_exception(const std::string& input, const std::string& remark) { throw_parser_exception(input, PARSER::emptyJson, remark); }

    inline void throw_parser_exception_if(bool condition, const std::string& input, const std::string& remark) { throw_parser_exception_if(condition, input, PARSER::emptyJson, remark); }

    [[noreturn]] extern void throw_parser_exception(const nlohmann::json& input, const nlohmann::json& context, const std::string& remark);

    inline void throw_parser_exception_if(bool condition, const nlohmann::json& input, const nlohmann::json& context, const std::string& remark) { if (condition) { throw_parser_exception(input, context, remark); } }

    [[noreturn]] inline void throw_parser_exception(const nlohmann::json& input, const std::string& remark) { throw_parser_exception(input, PARSER::emptyJson, remark); }

    inline void throw_parser_exception_if(bool condition, const nlohmann::json& input, const std::string& remark) { throw_parser_exception_if(condition, input, PARSER::emptyJson, remark); }

    /******************************************************************************************************************/

    [[noreturn]] extern void throw_not_supported(const nlohmann::json& input, const nlohmann::json& context, const std::string& remark);

    inline void throw_not_supported_if(bool condition, const nlohmann::json& input, const nlohmann::json& context, const std::string& remark) { if (condition) { throw_not_supported(input, context, remark); } }

    [[noreturn]] inline void throw_not_supported(const nlohmann::json& input, const std::string& remark) { throw_not_supported(input, PARSER::emptyJson, remark); }

    inline void throw_not_supported_if(bool condition, const nlohmann::json& input, const std::string& remark) { throw_not_supported_if(condition, input, PARSER::emptyJson, remark); }

    /******************************************************************************************************************/

    extern void check_object(const nlohmann::json& input, const nlohmann::json& context);

    inline void check_object(const nlohmann::json& input) { check_object(input, PARSER::emptyJson); }

    inline void check_object_decoupled(const nlohmann::json& input) { if constexpr (decoupledParsing) { check_object(input); } }

    extern void check_array(const nlohmann::json& input, const nlohmann::json& context);

    inline void check_array(const nlohmann::json& input) { check_array(input, PARSER::emptyJson); }

    extern void check_string(const nlohmann::json& input, const nlohmann::json& context);

    inline void check_string(const nlohmann::json& input) { check_string(input, PARSER::emptyJson); }

    extern void check_contains(const nlohmann::json& input, const std::string& key);

    extern void check_kind(const nlohmann::json& input, const std::string& kind_str);

    inline void check_kind_decoupled(const nlohmann::json& input, const std::string& kind_str) { if constexpr (decoupledParsing) { check_kind(input, kind_str); } }

    extern void check_op(const nlohmann::json& input, const std::string& op_str);

    inline void check_op_decoupled(const nlohmann::json& input, const std::string& op_str) { if constexpr (decoupledParsing) { check_op(input, op_str); } }

    extern void check_for_unexpected_element(const nlohmann::json& input, std::size_t size);

    /******************************************************************************************************************/

    extern int parse_integer(const nlohmann::json& input, const std::string& key);

    extern bool parse_bool(const nlohmann::json& input, const std::string& key);

    extern std::string parse_string(const nlohmann::json& input, const std::string& key);

    inline std::string parse_name(const nlohmann::json& input) { return parse_string(input, JANI::NAME); }

    extern void parse_comment_if(const nlohmann::json& input, Commentable& commentable, unsigned int& object_size);

    /******************************************************************************************************************/

    template<typename OpEnum_t>
    inline OpEnum_t check_op_enum_decoupled(const nlohmann::json& input, std::unique_ptr<OpEnum_t> (* str_to_op)(const std::string& op)) {
        if constexpr (PARSER::decoupledParsing) {
            const auto op_str = PARSER::parse_string(input, JANI::OP);
            const auto op_ptr = str_to_op(op_str);
            PARSER::throw_parser_exception_if(not op_ptr, op_str, input, JANI::OP);
            return *op_ptr;
        } else {
            return *str_to_op(input.at(JANI::OP));
        }
    }

    /******************************************************************************************************************/

    /** Dump json object composed of key and value. */
    extern nlohmann::json to_json(const std::string& key, const nlohmann::json& value);

    extern nlohmann::json name_to_json(const nlohmann::json& value);

    /******************************************************************************************************************/

    /**
    * Compute the length of the string contained in a file removing all whitespaces, linebreaks and tabs.
    * Exception: whitespaces, linebreaks, tabs within strings ("") are preserved.
    * @param filename, the name of the file
    * @return length, or -1 in case of an error.
    */
    extern int compute_length(const std::string& filename);

    /**
     * Parse a json file.
     * @return true if successful, otherwise false.
     */
    extern bool parse_json_file(const std::string& filename, nlohmann::json& json_ref);

}

#endif //PLAJA_PARSER_UTILS_H
