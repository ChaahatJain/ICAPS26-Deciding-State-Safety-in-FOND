//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_OPTION_PARSER_H
#define PLAJA_OPTION_PARSER_H

#include <list>
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include "../utils/utils.h"
#include "forward_option_parser.h"

namespace PLAJA {

// ISSUES
// #1 it is not checked whether specified options are used
// #2 values are not checked for validity: stream parsing is correct for correct values, but will generate any value for invalid values (implementation dependent?)

/**
 * Simple option parser.
 */
    class OptionParser {
        friend class Configuration; //
        friend struct ModelConstructorForTests; // to directly access options during testing

    private:
        void commandline_to_string(const char* arg_ex);
        static bool is_option(const std::string& arg);
        static std::string extract_option(const std::string& arg);
        static std::string extract_option_if(const std::string& arg);
        static std::string translate_if_bool(const std::string& arg);
        void parse_constants(const std::string& constants_arg);

        // commandline
        std::list<std::string> commandline;
        std::string commandlineStr;

        // known/supported options
        std::unordered_set<std::string> knownFlags;
        std::unordered_set<std::string> knownValueOptions;

        // options
        std::unordered_set<std::string> flags;
        std::unordered_map<std::string, std::string> valueOptions;
        std::unordered_map<std::string, std::string> constants;

        // type specific:
        std::unordered_map<std::string, std::string> strOptions;
        std::unordered_map<std::string, int> intOptions;
        std::unordered_map<std::string, double> doubleOptions;
        std::unordered_map<std::string, std::unique_ptr<PLAJA_OPTION::EnumOptionValuesSet>> enumOptions;

        void add_options();

    public:
        OptionParser();
        ~OptionParser();
        DELETE_CONSTRUCTOR(OptionParser)

        void add_flag(const std::string& flag);
        void add_value_option(const std::string& value_option);

        void load_commandline(int argc, char** argv);
        void pre_load_engine();
        void parse_options();

        [[nodiscard]] inline const std::string& get_commandline() const { return commandlineStr; }

        [[nodiscard]] const std::string& get_filename() const;
        [[nodiscard]] bool is_flag_set(const std::string& flag) const;
        [[nodiscard]] bool has_value_option(const std::string& option) const;
        [[nodiscard]] const std::string& get_value_option_string(const std::string& option) const;

        template<typename T>
        T get_option_value(const std::string& option, T default_opt) const {
            auto it = valueOptions.find(option);
            if (it == valueOptions.end()) { // default value
                return default_opt;
            } else {
                std::stringstream ss(it.operator*().second);
                T t;
                ss >> t;
                return t;
            }
        }

        static void handle_unknown_value_for_option(const std::string& option);

        template<typename T>
        T get_option_value(const std::unordered_map<std::string, T>& option_mapping, const std::string& option, T default_opt) const {
            if (has_value_option(option)) {
                const auto& value_option_str = get_value_option_string(option);
                auto it = option_mapping.find(value_option_str);
                if (it == option_mapping.end()) {
                    handle_unknown_value_for_option(option);
                    return default_opt;
                } else { return it->second; }
            } else {
                return default_opt;
            }
        }

        [[nodiscard]] std::vector<std::pair<int, int>> get_option_intervals(const std::string& option) const;

        [[nodiscard]] bool has_constant(const std::string& name) const;

        template<typename T>
        T get_constant(const std::string& option) const {
            std::stringstream ss(constants.at(option));
            T t;
            ss >> t;
            return t;
        }

        /** Only to be used for checking for unused constants at parse time. */
        [[nodiscard]] const std::unordered_map<std::string, std::string>& get_constants() const { return constants; }

        // type specific and defaults:

        void add_option(const std::string& option, const std::string& default_value);
        void add_int_option(const std::string& option, int default_value);
        void add_double_option(const std::string& option, double default_value);
        void add_enum_option(const std::string& option, std::unique_ptr<PLAJA_OPTION::EnumOptionValuesSet>&& value);

        [[nodiscard]] inline bool has_option(const std::string& option) const { return strOptions.count(option); }

        [[nodiscard]] inline const std::string& get_option(const std::string& option) const {
            PLAJA_ASSERT(has_option(option))
            return strOptions.at(option);
        }

        template<typename T>
        T get_option(const std::unordered_map<std::string, T>& option_mapping, const std::string& option) const {
            PLAJA_ASSERT(has_option(option))
            const auto& value_str = get_option(option);
            auto it = option_mapping.find(value_str);
            if (it == option_mapping.end()) {
                handle_unknown_value_for_option(option);
                PLAJA_ABORT
            }
            return it->second;
        }

        [[nodiscard]] inline bool has_int_option(const std::string& option) const { return intOptions.count(option); }

        [[nodiscard]] inline int get_int_option(const std::string& option) const {
            PLAJA_ASSERT(has_int_option(option))
            return intOptions.at(option);
        }

        [[nodiscard]] inline bool has_bool_option(const std::string& option) const { return has_int_option(option); }

        [[nodiscard]] inline bool get_bool_option(const std::string& option) const { return get_int_option(option); }

        [[nodiscard]] inline bool has_double_option(const std::string& option) const { return doubleOptions.count(option); }

        [[nodiscard]] inline double get_double_option(const std::string& option) const {
            PLAJA_ASSERT(has_double_option(option))
            return doubleOptions.at(option);
        }

        [[nodiscard]] bool has_enum_option(const std::string& option, bool ignore_none) const;

        [[nodiscard]] inline const PLAJA_OPTION::EnumOptionValuesSet& get_enum_option(const std::string& option) const {
            PLAJA_ASSERT(has_enum_option(option, false))
            return *enumOptions.at(option);
        }

        //

        static void get_help();

        // To be used by other classes to generate supported option string for helper output.
        template<typename T>
        static std::string print_supported_options(const std::unordered_map<std::string, T>& option_mapping) {
            std::string str;
            for (const auto& string_val: option_mapping) {
                str.append(string_val.first);
                str.append(PLAJA_UTILS::commaString);
                str.append(PLAJA_UTILS::spaceString);
            }
            str.replace(str.size() - 2, 1, PLAJA_UTILS::dotString); // last comma
            return str;
        }

    };

}

/** global option parser **********************************************************************************************/

namespace PLAJA_GLOBAL {

    extern const PLAJA::OptionParser* optionParser;

    [[nodiscard]] inline bool is_flag_set(const std::string& flag) { return PLAJA_GLOBAL::optionParser and PLAJA_GLOBAL::optionParser->is_flag_set(flag); }

    [[nodiscard]] inline bool has_value_option(const std::string& option) { return PLAJA_GLOBAL::optionParser and PLAJA_GLOBAL::optionParser->has_value_option(option); }

    [[nodiscard]] inline const std::string& get_value_option_string(const std::string& option) {
        PLAJA_ASSERT(PLAJA_GLOBAL::optionParser)
        return PLAJA_GLOBAL::optionParser->get_value_option_string(option);
    }

    template<typename T>
    inline T get_option_value(const std::string& option, T default_opt) { return PLAJA_GLOBAL::optionParser ? PLAJA_GLOBAL::optionParser->template get_option_value(option, default_opt) : default_opt; }

    template<typename T>
    inline T get_option_value(const std::unordered_map<std::string, T>& option_mapping, const std::string& option, T default_opt) { return PLAJA_GLOBAL::optionParser ? PLAJA_GLOBAL::optionParser->template get_option_value(option_mapping, option, default_opt) : default_opt; }

    [[nodiscard]] inline bool has_constant(const std::string& name) { return PLAJA_GLOBAL::optionParser and PLAJA_GLOBAL::optionParser->has_constant(name); }

    [[nodiscard]] inline bool has_option(const std::string& option) { return PLAJA_GLOBAL::optionParser and PLAJA_GLOBAL::optionParser->has_option(option); }

    [[nodiscard]] inline const std::string& get_option(const std::string& option) {
        PLAJA_ASSERT(PLAJA_GLOBAL::optionParser)
        return PLAJA_GLOBAL::optionParser->get_option(option);
    }

    template<typename T> inline T get_option(const std::unordered_map<std::string, T>& option_mapping, const std::string& option) {
        PLAJA_ASSERT(PLAJA_GLOBAL::optionParser)
        return PLAJA_GLOBAL::optionParser->template get_option(option_mapping, option);
    }

    [[nodiscard]] inline bool has_int_option(const std::string& option) { return PLAJA_GLOBAL::optionParser and PLAJA_GLOBAL::optionParser->has_int_option(option); }

    [[nodiscard]] inline int get_int_option(const std::string& option) {
        PLAJA_ASSERT(PLAJA_GLOBAL::optionParser)
        return PLAJA_GLOBAL::optionParser->get_int_option(option);
    }

    [[nodiscard]] inline bool has_bool_option(const std::string& option) { return PLAJA_GLOBAL::optionParser and PLAJA_GLOBAL::optionParser->has_bool_option(option); }

    [[nodiscard]] inline bool get_bool_option(const std::string& option) { return get_int_option(option); }

    [[nodiscard]] inline bool has_double_option(const std::string& option) { return PLAJA_GLOBAL::optionParser and PLAJA_GLOBAL::optionParser->has_double_option(option); }

    [[nodiscard]] inline double get_double_option(const std::string& option) {
        PLAJA_ASSERT(PLAJA_GLOBAL::optionParser)
        return PLAJA_GLOBAL::optionParser->get_double_option(option);
    }

}

#endif //PLAJA_OPTION_PARSER_H
