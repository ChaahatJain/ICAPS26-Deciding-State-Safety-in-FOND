//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

// #include <initializer_list>
#include <iostream>
#include <list>
#include <unordered_set>
#include "option_parser.h"
#include "../exception/option_parser_exception.h"
#include "../include/ct_config_const.h"
#include "../utils/rng.h"
#include "enum_option_values_set.h"
#include "option_parser_aux.h"
#include "plaja_options.h"

/* Extern. */
namespace SEARCH_ENGINE_FACTORY { extern void print_supported_engines(); }

namespace OPTION_PARSER {

    extern void set_suppress_known_option_check(bool value);

#ifdef NDEBUG

    void set_suppress_known_option_check(bool /*value*/) {}

    [[nodiscard]] inline bool get_suppress_known_option_check() { return false; }

#else

    bool suppressKnownOptionCheck(false);

    void set_suppress_known_option_check(bool value) { suppressKnownOptionCheck = value; }

    [[nodiscard]] inline bool get_suppress_known_option_check() { return suppressKnownOptionCheck; }

#endif

}

/**********************************************************************************************************************/


namespace PLAJA {

    OptionParser::OptionParser():
        commandlineStr(PLAJA_UTILS::emptyString) {
        add_options();
    }

    OptionParser::~OptionParser() = default;

    //

    void OptionParser::add_options() {

        add_flag(PLAJA_OPTION::help);
        // Engine:
        add_value_option(PLAJA_OPTION::engine);
        add_int_option(PLAJA_OPTION::max_time, PLAJA_OPTION_DEFAULT::max_time);
        add_value_option(PLAJA_OPTION::print_time);
        // Model & Properties
        add_value_option(PLAJA_OPTION::model_file);
        add_value_option(PLAJA_OPTION::prop);
        add_value_option(PLAJA_OPTION::additional_properties);
        add_flag(PLAJA_OPTION::allow_locals);
        add_value_option(PLAJA_OPTION::nn_interface);
        add_value_option(PLAJA_OPTION::nn_for_racetrack);
        add_value_option(PLAJA_OPTION::nn);
        add_value_option(PLAJA_OPTION::ensemble);
        add_value_option(PLAJA_OPTION::ensemble_interface);
        if constexpr (PLAJA_GLOBAL::useTorch) { add_value_option(PLAJA_OPTION::torch_model); }
        if constexpr (PLAJA_GLOBAL::enableApplicabilityFiltering) { 
            add_int_option(PLAJA_OPTION::applicability_filtering, PLAJA_OPTION_DEFAULT::applicability_filtering);
            add_value_option(PLAJA_OPTION::app_filter_ensemble);
        }
        add_flag(PLAJA_OPTION::ignore_nn);
        //
        add_int_option(PLAJA_OPTION::const_vars_to_consts, PLAJA_OPTION_DEFAULT::const_vars_to_consts);

        // Statistics
        add_flag(PLAJA_OPTION::print_stats);
        add_value_option(PLAJA_OPTION::stats_file);
        add_flag(PLAJA_OPTION::save_intermediate_stats);
        // Parsing
        add_flag(PLAJA_OPTION::ignore_unexpected_json);
        add_flag(PLAJA_OPTION::pa_objective);
        if constexpr (PLAJA_GLOBAL::enableTerminalStateSupport) { add_int_option(PLAJA_OPTION::set_pa_goal_objective_terminal, PLAJA_OPTION_DEFAULT::set_pa_goal_objective_terminal); }
        // MISC
        add_value_option(PLAJA_OPTION::seed);
    }

    void OptionParser::add_flag(const std::string& flag) {
        PLAJA_ASSERT(/* not knownFlags.count(flag) && */ not knownValueOptions.count(flag) and not strOptions.count(flag) and not intOptions.count(flag) and not doubleOptions.count(flag) and not enumOptions.count(flag)) // May be added by different engines.
        knownFlags.insert(flag);
    }

    void OptionParser::add_value_option(const std::string& value_option) {
        PLAJA_ASSERT(not knownFlags.count(value_option) /* && not knownValueOptions.count(value_option) */ and not strOptions.count(value_option) and not intOptions.count(value_option) and not doubleOptions.count(value_option) and not enumOptions.count(value_option))
        knownValueOptions.insert(value_option);
    }

    void OptionParser::load_commandline(int argc, char** argv) {
        PLAJA_ASSERT(commandline.empty())
        commandline = std::list<std::string>();

        // parse options:
        for (int i = 1; i < argc; ++i) {
            commandline.emplace_back(reinterpret_cast<const char*>(argv[i])); // NOLINT(*-pro-type-reinterpret-cast)
            /* if (commandline.back().empty()) { throw OptionParserException("empty argument"); } */
        }

        commandline_to_string(argv[0]);
        std::cout << PLAJA_UTILS::tabString << get_commandline() << std::endl;
    }

    void OptionParser::pre_load_engine() {
        const auto end = commandline.cend();
        for (auto it = commandline.cbegin(); it != end; ++it) {
            if (not is_option(*it)) { continue; }
            auto option = extract_option(*it);
            if (option == PLAJA_OPTION::engine && ++it != end) { // latter condition is re-detected and handled in parse_options, here just falling back to default engine ...
                valueOptions.insert(std::make_pair<std::string, std::string>(std::move(option), std::string(*it)));
            }
        }
    }

    void OptionParser::parse_options() {
        valueOptions.erase(PLAJA_OPTION::engine); // remove engine (if preloaded) to surpass duplicate detection

        std::unordered_set<std::string> loaded_options; // duplicate detection

        const auto end = commandline.cend();
        for (auto it = commandline.cbegin(); it != end; ++it) {

            if (is_option(*it)) {
                auto option = extract_option(*it);

                // constants via cmdline
                if (option == PLAJA_OPTION::consts) {
                    if (++it == end) { throw OptionParserException(PLAJA_OPTION::consts, "set --consts option without specifying any constant"); } // no arguments left
                    parse_constants(*it);
                    loaded_options.insert(option);
                    continue;
                }

                // normal option:

                // duplicate?
                if (loaded_options.count(option)) { throw OptionParserException(option, "duplicate option"); }
                loaded_options.insert(option);

                if (knownFlags.count(option)) { // flag:
                    flags.insert(option);
                } else {
                    if (++it == end) { throw OptionParserException(option, "value option without value"); } // no arguments left

                    if (knownValueOptions.count(option)) {
                        valueOptions.insert(std::make_pair<std::string, std::string>(std::move(option), translate_if_bool(*it)));
                    } else if (strOptions.count(option)) {
                        strOptions[option] = *it;
                    } else if (intOptions.count(option)) {
                        std::stringstream ss(translate_if_bool(*it));
                        ss >> intOptions[option];
                    } else if (doubleOptions.count(option)) {
                        std::stringstream ss(*it);
                        ss >> doubleOptions[option];
                    } else if (enumOptions.count(option)) {
                        enumOptions.at(option)->set_value(*it);
                    } else {
                        throw OptionParserException(option, "option unknown or not supported by engine");
                    }
                }

            } else if (not valueOptions.count(PLAJA_OPTION::model_file)) { // model file without explicit option
                valueOptions.emplace(PLAJA_OPTION::model_file, *it);
            } else if (not valueOptions.count(PLAJA_OPTION::additional_properties)) { // additional properties without explicit option
                valueOptions.emplace(PLAJA_OPTION::additional_properties, *it);
            } else {
                throw OptionParserException(*it, "unknown argument");
            }
        }

        // model file is needed
        if (not valueOptions.count(PLAJA_OPTION::model_file) and not flags.count(PLAJA_OPTION::help)) {
            throw OptionParserException("missing argument: model file"); // require model if not cmd help request
        }

        // maintain global RNG:
        if (has_value_option(PLAJA_OPTION::seed)) { PLAJA_GLOBAL::rng = std::make_unique<RandomNumberGenerator>(get_option_value(PLAJA_OPTION::seed, 42)); }
    }

    //

    void OptionParser::commandline_to_string(const char* arg_ex) {
        // program path
        commandlineStr.append(PLAJA_UTILS::quoteString);
        commandlineStr.append(arg_ex);
        commandlineStr.append(PLAJA_UTILS::quoteString);
        commandlineStr.append(PLAJA_UTILS::spaceString); // white space to next argument
        // commandline
        for (auto& arg: commandline) {
            if (arg.empty() or PLAJA_UTILS::contains(arg, PLAJA_UTILS::spaceString)) { // check if " is needed due to white spaces
                commandlineStr.append(PLAJA_UTILS::quoteString);
                commandlineStr.append(arg);
                commandlineStr.append(PLAJA_UTILS::quoteString);
            } else { // not needed ...
                commandlineStr.append(arg);
            }
            commandlineStr.append(PLAJA_UTILS::spaceString); // white space to next argument
        }
        commandlineStr.erase(commandlineStr.size() - 1); // last white space is not needed:
    }

    bool OptionParser::is_option(const std::string& arg) {
        constexpr char option_symbol = '-';
        return option_symbol == arg[0];
    }

    std::string OptionParser::extract_option(const std::string& arg) {
        PLAJA_ASSERT(is_option(arg))
        constexpr char option_symbol = '-';
        auto option(arg);
        // erase option symbol
        if (option_symbol == arg[1]) { option.erase(0, 2); } // "--"
        else { option.erase(0, 1); } // "-"
        //
        return option;
    }

    std::string OptionParser::extract_option_if(const std::string& arg) {
        if (is_option(arg)) { return extract_option(arg); }
        else { return arg; }
    }

    std::string OptionParser::translate_if_bool(const std::string& arg) {
        if (arg == "true") { return std::to_string(true); }
        if (arg == "false") { return std::to_string(false); }
        else { return arg; }
    }

    void OptionParser::parse_constants(const std::string& constants_arg) {
        // Remove white spaces/ line breaks/ taps:
        std::string constants_str = PLAJA_UTILS::erase_all(constants_arg, PLAJA_UTILS::spaceString);
        constants_str = PLAJA_UTILS::erase_all(std::move(constants_str), PLAJA_UTILS::lineBreakString);
        constants_str = PLAJA_UTILS::erase_all(std::move(constants_str), PLAJA_UTILS::tabString);

        // Split after comma
        const std::vector<std::string> constant_strs = PLAJA_UTILS::split(constants_str, PLAJA_UTILS::commaString);

        // Split after "="
        for (const std::string& constant_str: constant_strs) {
            auto name_value = PLAJA_UTILS::split_into(constant_str, PLAJA_UTILS::equalString, 1);
            if (name_value.size() != 2) { throw OptionParserException(constant_str, "illegal constant specification"); }
            name_value[1] = translate_if_bool(name_value[1]);
            if (constants.count(name_value[0])) { throw OptionParserException(constant_str, "duplicate constant specification"); }
            constants.insert(std::make_pair<std::string, std::string>(std::move(name_value[0]), std::move(name_value[1])));
        }

    }

    // get options:

    const std::string& OptionParser::get_filename() const { return valueOptions.at(PLAJA_OPTION::model_file); }

    bool OptionParser::is_flag_set(const std::string& flag) const {

        if constexpr (PLAJA_GLOBAL::debug) {
            if (not OPTION_PARSER::get_suppress_known_option_check() and not knownFlags.count(flag)) {
                if (knownValueOptions.count(flag)) { throw OptionParserException("Tried to use value option " + flag + " as a flag."); }
                else { throw OptionParserException("Tried to use unknown flag: " + flag); }
            }
        }

        return flags.count(flag);
    }

    bool OptionParser::has_value_option(const std::string& option) const {

        if constexpr (PLAJA_GLOBAL::debug) {
            if (not OPTION_PARSER::get_suppress_known_option_check() and not knownValueOptions.count(option)) {
                if (knownFlags.count(option)) { throw OptionParserException("Tried to use flag" + option + " as value option."); }
                else { throw OptionParserException("Tried to use unknown value option: " + option); }
            }
        }

        return valueOptions.count(option) > 0;
    }

    const std::string& OptionParser::get_value_option_string(const std::string& option) const {
        auto it = valueOptions.find(option);
        if (it == valueOptions.end()) { throw OptionParserException(option, "missing option"); }
        else { return it.operator*().second; }
    }

    void OptionParser::handle_unknown_value_for_option(const std::string& option) { throw OptionParserException(option, "unknown value"); }

    std::vector<std::pair<int, int>> OptionParser::get_option_intervals(const std::string& option) const {

        const auto sequences_split = PLAJA_UTILS::split(get_value_option_string(option), PLAJA_UTILS::commaString);

        std::vector<std::pair<int, int>> intervals;
        intervals.reserve(sequences_split.size());
        //
        for (const auto& sequence_str: sequences_split) {
            try {
                if (PLAJA_UTILS::contains(sequence_str, PLAJA_UTILS::colonString)) {
                    const auto index_split = PLAJA_UTILS::split_into(sequence_str, PLAJA_UTILS::colonString, 2);
                    if (index_split.size() != 2) { throw OptionParserException(sequence_str, "illegal interval specification"); }
                    intervals.emplace_back(std::stoi(index_split[0]), std::stoi(index_split[1]));
                } else {
                    const auto index = std::stoi(sequence_str);
                    intervals.emplace_back(index, index);
                }
            } catch (std::invalid_argument& e) {
                throw OptionParserException(sequence_str, "illegal interval specification");
            }
        }

        return intervals;
    }

    bool OptionParser::has_constant(const std::string& name) const { return constants.count(name); }

    //

    void OptionParser::add_option(const std::string& option, const std::string& default_value) {
        PLAJA_ASSERT(not knownFlags.count(option) and not knownValueOptions.count(option) and not intOptions.count(option) and not doubleOptions.count(option) and not enumOptions.count(option))
        auto it = strOptions.find(option);

        if (it == strOptions.cend()) { strOptions.emplace(option, default_value); }
        else {
            if (it->second != default_value) { throw OptionParserException(option, "conflicting default values"); }
        }
    }

    void OptionParser::add_int_option(const std::string& option, int default_value) {
        PLAJA_ASSERT(not knownFlags.count(option) and not knownValueOptions.count(option) and not strOptions.count(option) and not doubleOptions.count(option) and not enumOptions.count(option))
        auto it = intOptions.find(option);

        if (it == intOptions.cend()) { intOptions.emplace(option, default_value); }
        else {
            if (it->second != default_value) { throw OptionParserException(option, "conflicting default values"); }
        }
    }

    void OptionParser::add_double_option(const std::string& option, double default_value) {
        PLAJA_ASSERT(not knownFlags.count(option) and not knownValueOptions.count(option) and not strOptions.count(option) and not intOptions.count(option) and not enumOptions.count(option))
        auto it = doubleOptions.find(option);

        if (it == doubleOptions.cend()) { doubleOptions.emplace(option, default_value); }
        else {
            if (it->second != default_value) { throw OptionParserException(option, "conflicting default values"); }
        }
    }

    void OptionParser::add_enum_option(const std::string& option, std::unique_ptr<PLAJA_OPTION::EnumOptionValuesSet>&& value) {
        PLAJA_ASSERT(not knownFlags.count(option) and not knownValueOptions.count(option) and not strOptions.count(option) and not intOptions.count(option) and not doubleOptions.count(option))
        auto it = enumOptions.find(option);

        if (it == enumOptions.cend()) { enumOptions.emplace(option, std::move(value)); }
        else if (*it->second != *value) { throw OptionParserException(option, "conflicting default values"); }
    }

    bool OptionParser::has_enum_option(const std::string& option, bool ignore_none) const {
        return enumOptions.count(option) and not(ignore_none and enumOptions.at(option)->in_none());
    }

    // Help:

    void OptionParser::get_help() {
        const std::string file_str = "<file>";
        const std::string value_str = "<value>";

        std::cout << "Usage: plaja <model file> <options...>" << std::endl << std::endl;

        // Engines
        OPTION_PARSER::print_options_headline("Engines");
        OPTION_PARSER::print_option(PLAJA_OPTION::engine, value_str, "Set search engine to be used.", true);
        SEARCH_ENGINE_FACTORY::print_supported_engines();
        OPTION_PARSER::print_additional_specification("By default EXPLORER, to explore the reachable state space.");

        OPTION_PARSER::print_int_option(PLAJA_OPTION::max_time, PLAJA_OPTION_DEFAULT::max_time, "Maximal search time in seconds.", true);
        OPTION_PARSER::print_additional_specification("Remaining time is checked after each search \"step\" of the underlying engine(s),", true);
        OPTION_PARSER::print_additional_specification("which may result in a violation of the maximal time bound.");

        OPTION_PARSER::print_option(PLAJA_OPTION::print_time, value_str, "Time after which the current statistics will be printed, by default 180s.", true);
        OPTION_PARSER::print_additional_specification("Remaining time is checked after each search \"step\" of the underlying engine(s),", true);
        OPTION_PARSER::print_additional_specification("which may result in a violation of the time bound.");

        OPTION_PARSER::print_options_footline();

        // Properties
        OPTION_PARSER::print_options_headline("Model & Properties");

        OPTION_PARSER::print_option(PLAJA_OPTION::model_file, file_str, "Path to model file.", true);
        OPTION_PARSER::print_additional_specification("Can also be provided as the first positional argument.");
        OPTION_PARSER::print_option(PLAJA_OPTION::prop, value_str, "Only check the property of the given index.");
        OPTION_PARSER::print_option(PLAJA_OPTION::additional_properties, file_str, "Specify additional properties for the model via an external <file>.", true);
        OPTION_PARSER::print_additional_specification("Can also be provided as the second positional argument.");
        OPTION_PARSER::print_flag(PLAJA_OPTION::allow_locals, "Flag to indicate that properties in additional-properties-file may read unique local variables.");

        OPTION_PARSER::print_option(PLAJA_OPTION::nn_interface, file_str, "Path to the neural network (interface).");
        OPTION_PARSER::print_option(PLAJA_OPTION::nn_for_racetrack, "\"<x_var_name>,<y_var_name>\"", "Indicates that a hardcoded racetrack jani2nnet-interface is used.", true);
        OPTION_PARSER::print_additional_specification("All interface file paths will be interpreted as nnet file paths.", true);
        OPTION_PARSER::print_additional_specification("The value specifies the names of the used position variables.");
        OPTION_PARSER::print_option(PLAJA_OPTION::nn, file_str, "Path to the neural network (in nnet format).");
        if constexpr (PLAJA_GLOBAL::useTorch) { OPTION_PARSER::print_option(PLAJA_OPTION::torch_model, OPTION_PARSER::fileStr, "Path to the neural network (as PyTorch model)."); }
        if constexpr (PLAJA_GLOBAL::enableApplicabilityFiltering) {
            OPTION_PARSER::print_int_option(PLAJA_OPTION::applicability_filtering, PLAJA_OPTION_DEFAULT::applicability_filtering, "Suppress applicability-filtering flag in interface file.", true);
            OPTION_PARSER::print_additional_specification("0 -> use interface flag, positive -> enabled, Negative -> disabled.");
        }
        OPTION_PARSER::print_flag(PLAJA_OPTION::ignore_nn, "Ignore NN (policy) directly specified in properties.");
        OPTION_PARSER::print_option(PLAJA_OPTION::ensemble_interface, file_str, "Path to the tree ensemble interface");
        OPTION_PARSER::print_option(PLAJA_OPTION::ensemble, file_str, "Path to the tree ensemble (in JSON format).");
        if constexpr (PLAJA_GLOBAL::enableApplicabilityFiltering) {
            OPTION_PARSER::print_option(PLAJA_OPTION::app_filter_ensemble, file_str, "Path to a (compressed) naively encoded applicability filter. Internal encoding too memory intensive.");
            OPTION_PARSER::print_additional_specification("Veritas supports .gzip and .json formats to be loaded.");
        }

        OPTION_PARSER::print_options_footline();

        // Constants
        OPTION_PARSER::print_options_headline("Constants");
        OPTION_PARSER::print_option(PLAJA_OPTION::consts, "\"<c_1>=<v_1>, ..., <c_n>=<v_n>\"", "Specify constants unspecified in model file,", true);
        OPTION_PARSER::print_additional_specification("\te.g., \"C1=2, C2= true, C3=4.0\"");
        OPTION_PARSER::print_bool_option(PLAJA_OPTION::const_vars_to_consts, PLAJA_OPTION_DEFAULT::const_vars_to_consts, "Transform variables constant as per transition model to constants.");
        OPTION_PARSER::print_options_footline();
        OPTION_PARSER::print_options_footline();

        // Statistics
        OPTION_PARSER::print_options_headline("Statistics");
        OPTION_PARSER::print_flag(PLAJA_OPTION::print_stats, "Print out statistics.");
        OPTION_PARSER::print_option(PLAJA_OPTION::stats_file, file_str, "Save statistics as CSVs to file.");
        OPTION_PARSER::print_flag(PLAJA_OPTION::save_intermediate_stats, "Save intermediate statistics as well.");
        OPTION_PARSER::print_option(PLAJA_OPTION::savePath, file_str, "Save path, e.g., to goal (if applicable).", true);
        OPTION_PARSER::print_additional_specification("The empty filename is reserved to indicate standard output.", true);
        OPTION_PARSER::print_additional_specification("Some engine may always print to standard output.");
        OPTION_PARSER::print_option(PLAJA_OPTION::saveStateSpace, file_str, "Save constructed state space (if applicable).");
        OPTION_PARSER::print_options_footline();

        // Parsing
        OPTION_PARSER::print_options_headline("Parsing");
        OPTION_PARSER::print_flag(PLAJA_OPTION::ignore_unexpected_json, "Flag to indicate that unknown elements in a json object shall be ignored.");
        OPTION_PARSER::print_flag(PLAJA_OPTION::pa_objective, "Flag to indicate that objective expression in PA expressions is to be considered as goal rather than reach expression.");
        if constexpr (PLAJA_GLOBAL::enableTerminalStateSupport) { OPTION_PARSER::print_bool_option(PLAJA_OPTION::set_pa_goal_objective_terminal, PLAJA_OPTION_DEFAULT::set_pa_goal_objective_terminal, "Set goal objective states to be terminal."); }
        OPTION_PARSER::print_options_footline();

        // Misc
        OPTION_PARSER::print_options_headline("MISC");
        OPTION_PARSER::print_option(PLAJA_OPTION::seed, OPTION_PARSER::valueStr, "Integer seed used for random generators.");

        OPTION_PARSER::print_options_footline();

    }

}
