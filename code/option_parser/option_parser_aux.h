//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_OPTION_PARSER_AUX_H
#define PLAJA_OPTION_PARSER_AUX_H

#include <string>
#include "forward_option_parser.h"

namespace OPTION_PARSER {

    // auxiliaries to construct supported options

    extern void add_flag(PLAJA::OptionParser& option_parser, const std::string& flag);
    extern void add_value_option(PLAJA::OptionParser& option_parser, const std::string& option);

    extern void add_option(PLAJA::OptionParser& option_parser, const std::string& option, const std::string& default_value);
    extern void add_int_option(PLAJA::OptionParser& option_parser, const std::string& option, int default_value);
    extern void add_bool_option(PLAJA::OptionParser& option_parser, const std::string& option, bool default_value);
    extern void add_double_option(PLAJA::OptionParser& option_parser, const std::string& option, double default_value);
    extern void add_enum_option(PLAJA::OptionParser& option_parser, const std::string& option, std::unique_ptr<PLAJA_OPTION::EnumOptionValuesSet>&& value);

    // auxiliaries to access option parser
    [[nodiscard]] extern bool has_option(const PLAJA::OptionParser& option_parser, const std::string& option);
    [[nodiscard]] extern bool has_int_option(const PLAJA::OptionParser& option_parser, const std::string& option);
    [[nodiscard]] extern bool has_bool_option(const PLAJA::OptionParser& option_parser, const std::string& option);
    [[nodiscard]] extern bool has_double_option(const PLAJA::OptionParser& option_parser, const std::string& option);

    [[nodiscard]] extern const std::string& get_option(const PLAJA::OptionParser& option_parser, const std::string& option);
    [[nodiscard]] extern int get_int_option(const PLAJA::OptionParser& option_parser, const std::string& option);
    [[nodiscard]] extern bool get_bool_option(const PLAJA::OptionParser& option_parser, const std::string& option);
    [[nodiscard]] extern double get_double_option(const PLAJA::OptionParser& option_parser, const std::string& option);

    // auxiliaries for option printing

    extern const std::string valueStr;
    extern const std::string fileStr;
    extern const std::string dirStr;

    extern void print_options_headline(const std::string& headline);
    extern void print_flag(const std::string& flag, const std::string& specification, bool additional_specification = false);
    extern void print_option(const std::string& option, const std::string& value, const std::string& specification, bool additional_specification = false);
    extern void print_option(const std::string& option, const std::string& value, const std::string& default_value, const std::string& specification, bool additional_specification = false);
    extern void print_int_option(const std::string& option, int default_value, const std::string& specification, bool additional_specification = false);
    extern void print_bool_option(const std::string& option, bool default_value, const std::string& specification, bool additional_specification = false);
    extern void print_double_option(const std::string& option, double default_value, const std::string& specification, bool additional_specification = false);
    extern void print_enum_option(const std::string& option, const PLAJA_OPTION::EnumOptionValuesSet& value, const std::string& specification, bool additional_specification = false);
    extern void print_additional_specification(const std::string& specification, bool additional_specification = false);
    extern void print_options_footline();

    // option prints used by more than a single engine:
    extern void print_local_backup();
}

#endif //PLAJA_OPTION_PARSER_AUX_H
