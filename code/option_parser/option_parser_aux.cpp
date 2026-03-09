//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <iostream>
#include "enum_option_values_set.h"
#include "option_parser_aux.h"
#include "option_parser.h"

namespace PLAJA_OPTION { extern const std::string local_backup; }

namespace OPTION_PARSER {

    /* Auxiliaries to construct supported options. */

    void add_flag(PLAJA::OptionParser& option_parser, const std::string& flag) { option_parser.add_flag(flag); }

    void add_value_option(PLAJA::OptionParser& option_parser, const std::string& option) { option_parser.add_value_option(option); }

    void add_option(PLAJA::OptionParser& option_parser, const std::string& option, const std::string& default_value) { option_parser.add_option(option, default_value); }

    void add_int_option(PLAJA::OptionParser& option_parser, const std::string& option, int default_value) { option_parser.add_int_option(option, default_value); }

    void add_bool_option(PLAJA::OptionParser& option_parser, const std::string& option, bool default_value) { option_parser.add_int_option(option, default_value); }

    void add_double_option(PLAJA::OptionParser& option_parser, const std::string& option, double default_value) { option_parser.add_double_option(option, default_value); }

    void add_enum_option(PLAJA::OptionParser& option_parser, const std::string& option, std::unique_ptr<PLAJA_OPTION::EnumOptionValuesSet>&& value) { option_parser.add_enum_option(option, std::move(value)); }

    bool has_option(const PLAJA::OptionParser& option_parser, const std::string& option) { return option_parser.has_option(option); }

    bool has_int_option(const PLAJA::OptionParser& option_parser, const std::string& option) { return option_parser.has_int_option(option); }

    bool has_bool_option(const PLAJA::OptionParser& option_parser, const std::string& option) { return option_parser.has_bool_option(option); }

    bool has_double_option(const PLAJA::OptionParser& option_parser, const std::string& option) { return option_parser.has_double_option(option); }

    const std::string& get_option(const PLAJA::OptionParser& option_parser, const std::string& option) { return option_parser.get_option(option); }

    int get_int_option(const PLAJA::OptionParser& option_parser, const std::string& option) { return option_parser.get_int_option(option); }

    bool get_bool_option(const PLAJA::OptionParser& option_parser, const std::string& option) { return option_parser.get_bool_option(option); }

    double get_double_option(const PLAJA::OptionParser& option_parser, const std::string& option) { return option_parser.get_double_option(option); }

    /* Auxiliaries for option printing. */

    const std::string valueStr("<value>"); // NOLINT(cert-err58-cpp)
    const std::string fileStr("<file>"); // NOLINT(cert-err58-cp
    const std::string dirStr("<dir>"); // NOLINT(cert-err58-cp)

    const std::string optionStr("--"); // NOLINT(cert-err58-cpp)

    void print_options_headline(const std::string& headline) {
        std::cout << "----------- " << headline << " -----------" << std::endl << std::endl;
    }

    void print_flag(const std::string& flag, const std::string& specification, bool additional_specification) {
        std::cout << OPTION_PARSER::optionStr << flag << std::endl;
        std::cout << PLAJA_UTILS::tabString << specification << std::endl;
        if (not additional_specification) { std::cout << std::endl; }
    }

    void print_option(const std::string& option, const std::string& value, const std::string& specification, bool additional_specification) {
        std::cout << OPTION_PARSER::optionStr << option << PLAJA_UTILS::spaceString << value << std::endl;
        std::cout << PLAJA_UTILS::tabString << specification << std::endl;
        if (not additional_specification) { std::cout << std::endl; }
    }

    void print_option(const std::string& option, const std::string& value, const std::string& default_value, const std::string& specification, bool additional_specification) {
        std::cout << OPTION_PARSER::optionStr << option << PLAJA_UTILS::spaceString << value << std::endl;
        std::cout << PLAJA_UTILS::tabString << specification << std::endl;
        std::cout << PLAJA_UTILS::tabString << "Default: " << default_value << PLAJA_UTILS::dotString << std::endl;
        if (not additional_specification) { std::cout << std::endl; }
    }

    extern void print_int_option(const std::string& option, int default_value, const std::string& specification, bool additional_specification) {
        const auto default_str = std::to_string(default_value);
        print_option(option, OPTION_PARSER::valueStr, default_str, specification, additional_specification);
    }

    extern void print_bool_option(const std::string& option, bool default_value, const std::string& specification, bool additional_specification) {
        const auto default_str = default_value ? "true" : "false";
        print_option(option, OPTION_PARSER::valueStr, default_str, specification, additional_specification);
    }

    extern void print_double_option(const std::string& option, double default_value, const std::string& specification, bool additional_specification) {
        const auto default_str = std::to_string(default_value);
        print_option(option, OPTION_PARSER::valueStr, default_str, specification, additional_specification);
    }

    void print_enum_option(const std::string& option, const PLAJA_OPTION::EnumOptionValuesSet& value, const std::string& specification, bool additional_specification) {
        print_option(option, OPTION_PARSER::valueStr, value.enum_option_value_to_str(value.get_value()), specification, true);
        print_additional_specification("Supported values: " + value.supported_values_to_str() + PLAJA_UTILS::dotString, additional_specification);
    }

    void print_additional_specification(const std::string& specification, bool additional_specification) {
        std::cout << PLAJA_UTILS::tabString << specification << std::endl;
        if (not additional_specification) { std::cout << std::endl; }
    }

    void print_options_footline() { std::cout << std::endl; }

    void print_local_backup() {
        OPTION_PARSER::print_flag(PLAJA_OPTION::local_backup, "Save local backups, i.e., in the execution directory, e.g., for CEGAR-generated predicate sets.");
    }

}