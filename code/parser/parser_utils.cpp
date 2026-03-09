//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <fstream>
#include <json.hpp>
#include "parser_utils.h"
#include "../exception/not_supported_exception.h"
#include "../exception/parser_exception.h"
#include "../option_parser/option_parser.h"
#include "../utils/utils.h"
#include "ast/commentable.h"

namespace PLAJA_OPTION { extern const std::string ignore_unexpected_json; }

namespace PARSER {

    /******************************************************************************************************************/

    /* Unused for now. */

    struct StringJson: public nlohmann::json {
        explicit StringJson(const char* str) noexcept;
    };

    StringJson::StringJson(const char* str) noexcept try : // NOLINT(*-exception-escape)
        nlohmann::json(str) {} catch (...) {
        std::cout << "nlohmann::json constructor threw an exception" << std::endl;
        std::terminate();
    }

    /******************************************************************************************************************/

    const nlohmann::json emptyJson(""); // NOLINT(cert-err58-cpp)
    // strings used frequently, e.g. in parser exceptions
    const std::string expected_json_object { "expected JSON object" }; // NOLINT(cert-err58-cpp)
    const std::string expected_json_array { "expected JSON array" }; // NOLINT(cert-err58-cpp)
    const std::string expected_json_string { "expected JSON string" }; // NOLINT(cert-err58-cpp)
    const std::string duplicate_name { "duplicate name" }; // NOLINT(cert-err58-cpp)
    const std::string unknown_name { "unknown name" }; // NOLINT(cert-err58-cpp)
    const std::string unexpected_element { "unexpected element" }; // NOLINT(cert-err58-cpp)
    const std::string unmatched_jani_structure { "unmatched JANI structure" }; // NOLINT(cert-err58-cpp)
    const std::string not_supported_by_model_type { "not supported by model type" }; // NOLINT(cert-err58-cpp)
    const std::string at_least_one_element { "at least one element" }; // NOLINT(cert-err58-cpp)
    const std::string invalid { "invalid" }; // NOLINT(cert-err58-cpp)

    /******************************************************************************************************************/

    void throw_parser_exception(const std::string& input, const nlohmann::json& context, const std::string& remark) {
        throw ParserException(input, context.dump(), remark);
    }

    void throw_parser_exception(const nlohmann::json& input, const nlohmann::json& context, const std::string& remark) {
        throw ParserException(input.dump(), context.dump(), remark);
    }

    void throw_not_supported(const nlohmann::json& input, const nlohmann::json& context, const std::string& remark) {
        throw NotSupportedException(input.dump(), context.dump(), remark);
    }

    /******************************************************************************************************************/

    void check_object(const nlohmann::json& input, const nlohmann::json& context) {
        throw_parser_exception_if(not input.is_object(), input, context, PARSER::expected_json_object);
    }

    void check_array(const nlohmann::json& input, const nlohmann::json& context) {
        throw_parser_exception_if(not input.is_array(), input, context, PARSER::expected_json_array);
    }

    void check_string(const nlohmann::json& input, const nlohmann::json& context) {
        throw_parser_exception_if(not input.is_string(), input, context, PARSER::expected_json_string);
    }

    void check_contains(const nlohmann::json& input, const std::string& key) {
        throw_parser_exception_if(not input.contains(key), input, key);
    }

    void check_kind(const nlohmann::json& input, const std::string& kind_str) {
        PARSER::check_contains(input, JANI::KIND);
        const auto& kind = input.at(JANI::KIND);
        throw_parser_exception_if(not kind.is_string() or kind != kind_str, kind, input, kind_str);
    }

    void check_op(const nlohmann::json& input, const std::string& op_str) {
        check_contains(input, JANI::OP);
        const auto& op = input.at(JANI::OP);
        throw_parser_exception_if(not op.is_string() or op != op_str, op, input, op_str);
    }

    void check_for_unexpected_element(const nlohmann::json& input, std::size_t size) {
        throw_parser_exception_if(input.size() != size and not PLAJA_GLOBAL::is_flag_set(PLAJA_OPTION::ignore_unexpected_json), input, PARSER::unexpected_element);
    }

    /******************************************************************************************************************/

    int parse_integer(const nlohmann::json& input, const std::string& key) {
        PARSER::check_contains(input, key);
        const auto& value = input.at(key);
        throw_parser_exception_if(not value.is_number_integer(), value, input, "expected integer");
        return value;
    }

    bool parse_bool(const nlohmann::json& input, const std::string& key) {
        PARSER::check_contains(input, key);
        const auto& value = input.at(key);
        throw_parser_exception_if(not value.is_boolean(), value, input, "expected bool");
        return value;
    }

    std::string parse_string(const nlohmann::json& input, const std::string& key) {
        PARSER::check_contains(input, key);
        const auto& value = input.at(JANI::NAME);
        PARSER::check_string(value, input);
        return value;
    }

    void parse_comment_if(const nlohmann::json& input, Commentable& commentable, unsigned int& object_size) {
        if (input.contains(JANI::COMMENT)) {
            const auto& comment_j = input.at(JANI::COMMENT);
            PARSER::check_string(comment_j, input);
            ++object_size;
            commentable.set_comment(comment_j);
        }
    }

    /******************************************************************************************************************/

    nlohmann::json to_json(const std::string& key, const nlohmann::json& value) {
        nlohmann::json j;
        j.emplace(key, value);
        return j;
    }

    nlohmann::json name_to_json(const nlohmann::json& value) { return to_json(JANI::NAME, value); }

    /******************************************************************************************************************/

    int compute_length(const std::string& filename) {
        std::ifstream i(filename);
        if (i.fail()) { return -1; }

        char tmp;
        std::string result;
        bool inside_name = false;

        while (i.get(tmp)) {
            // inside names spaces are preserved
            if (tmp == '"') { inside_name = !inside_name; }

            if (inside_name) { result.append(1, tmp); }
            else {
                if (tmp != ' ' && tmp != '\n' && tmp != '\t' && tmp != '\r') {
                    result.append(1, tmp);
                }

            }

        }

        i.close();

        return PLAJA_UTILS::cast_numeric<int>(result.size());
    }

    bool parse_json_file(const std::string& filename, nlohmann::json& json_ref) {
        const int input_size = PARSER::compute_length(filename);
        if (input_size == -1) {
            std::cout << "File \"" << (filename) << "\" could not be opened ..." << std::endl;
            return false;
        }

        try {
            std::ifstream i(filename);
            i >> (json_ref);
        } catch (nlohmann::detail::exception& e) {
            std::cout << "Error while parsing: \"" << filename << "\" is not a valid JSON document." << std::endl;
            return false;
        }
        // check for potential duplicate elements:
        // json parsing will silently take the last value parsed, thus we compare string size
        if (input_size != (json_ref).dump().size()) {
            std::cout << "Warning: for \"" << filename << "\" size of the file (" << input_size << ") does not equal the size of the json object (" << json_ref.dump().size() << "), this may be due to duplicate elements in the file description." << std::endl;
            return false;
        }

        return true;
    }

}

