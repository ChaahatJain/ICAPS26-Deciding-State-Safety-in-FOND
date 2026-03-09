//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_CONFIGURATION_H
#define PLAJA_CONFIGURATION_H

#include <memory>
#include <sstream>
#include <unordered_map>
#include "../../option_parser/option_parser.h"
#include "sharable_structure.h"

namespace PLAJA {

/**********************************************************************************************************************/

    enum class SharableKey {
        MODEL,
        PROP_INFO,
        SUCC_GEN,
        STATS,
        // PA:
        PA_MT,
        PREDICATE_RELATIONS,
        HEURISTIC_TYPE,
        STATS_HEURISTIC,
        SEARCH_SPACE_PA,
        // SMT:
        MODEL_SMT,
        MODEL_Z3,
        MODEL_MARABOU,
        MODEL_VERITAS,
    };

/**********************************************************************************************************************/

    class Configuration {
        friend struct ModelConstructorForTests; // to directly access options during testing

    private:
        const OptionParser* optionParser;

    protected:
        std::unordered_set<std::string> flags;
        std::unordered_set<std::string> disabledFlags;
        std::unordered_map<std::string, std::string> options;
        std::unordered_map<std::string, int> optionsInt;
        std::unordered_map<std::string, double> optionsDouble;
        std::unordered_map<std::string, std::unique_ptr<PLAJA_OPTION::EnumOptionValuesSet>> optionsEnum;
        std::unordered_set<std::string> disabledOptions;

        // shared pre-constructs
        mutable std::unordered_map<SharableKey, std::unique_ptr<SharableStructure>> sharableStructures;
        mutable std::unordered_map<SharableKey, std::unique_ptr<SharableStructure>> sharableStructuresPtr;
        mutable std::unordered_map<SharableKey, std::unique_ptr<SharableStructure>> sharableStructuresConst;

    public:
        explicit Configuration(const OptionParser& option_parser);
        Configuration();
        Configuration(const Configuration& other);
        virtual ~Configuration();

        Configuration(Configuration&& other) = delete;
        DELETE_ASSIGNMENT(Configuration)

        [[nodiscard]] inline const PLAJA::OptionParser* share_option_parser() const { return optionParser; }

        // options:

        void set_flag(const std::string& flag, bool value);
        void set_value_option(const std::string& option, const std::string& value);
        void set_int_option(const std::string& option, int value);
        void set_bool_option(const std::string& option, bool value);
        void set_double_option(const std::string& option, double value);
        void set_enum_option(const std::string& option, std::unique_ptr<PLAJA_OPTION::EnumOptionValuesSet> value);
        void disable_value_option(const std::string& option);

        [[nodiscard]] bool is_flag_set(const std::string& flag) const;
        [[nodiscard]] bool has_value_option(const std::string& option) const;
        [[nodiscard]] bool has_option(const std::string& option) const;
        [[nodiscard]] bool has_int_option(const std::string& option) const;
        [[nodiscard]] bool has_bool_option(const std::string& option) const;
        [[nodiscard]] bool has_double_option(const std::string& option) const;
        [[nodiscard]] bool has_enum_option(const std::string& option, bool ignore_none) const;
        [[nodiscard]] const std::string& get_value_option_string(const std::string& option) const;

        template<typename T>
        [[nodiscard]] inline T get_option_value(const std::string& option, T default_opt) const {
            if (disabledOptions.count(option)) { return default_opt; }
            //
            auto it = options.find(option);
            if (it == options.end()) { // default value
                return optionParser->get_option_value(option, default_opt);
            } else {
                std::stringstream ss(it.operator*().second);
                T t;
                ss >> t;
                return t;
            }
        }

        template<typename T>
        inline T get_option_value(const std::unordered_map<std::string, T>& option_mapping, const std::string& option, T default_opt) const {
            PLAJA_ASSERT(not disabledOptions.count(option));
            auto it = options.find(option);
            if (it == options.end()) { // default value
                return optionParser->get_option_value(option_mapping, option, default_opt);
            } else {
                auto it_enum = option_mapping.find(it->second);
                if (it_enum == option_mapping.end()) {
                    OptionParser::handle_unknown_value_for_option(option);
                    return default_opt;
                } else { return it_enum->second; }
            }
        }

        [[nodiscard]] const std::string& get_option(const std::string& option) const;

        template<typename T>
        inline T get_option(const std::unordered_map<std::string, T>& option_mapping, const std::string& option) const {
            PLAJA_ASSERT(not disabledOptions.count(option));
            auto it = options.find(option);
            if (it == options.end()) { // default value
                return optionParser->get_option(option_mapping, option);
            } else {
                auto it_enum = option_mapping.find(it->second);
                if (it_enum == option_mapping.end()) {
                    OptionParser::handle_unknown_value_for_option(option);
                    PLAJA_ABORT
                }
                return it_enum->second;
            }
        }

        [[nodiscard]] int get_int_option(const std::string& option) const;

        [[nodiscard]] bool get_bool_option(const std::string& option) const;

        [[nodiscard]] double get_double_option(const std::string& option) const;

        [[nodiscard]] const PLAJA_OPTION::EnumOptionValuesSet& get_enum_option(const std::string& option) const;

        // shared structures:

        FCT_IF_DEBUG([[nodiscard]] inline std::size_t size_sharable() const { return sharableStructures.size(); })

        FCT_IF_DEBUG([[nodiscard]] inline std::size_t size_sharable_ptr() const { return sharableStructuresPtr.size(); })

        FCT_IF_DEBUG([[nodiscard]] inline std::size_t size_sharable_const() const { return sharableStructuresConst.size(); })

        template<typename SharedStructure_t>
        inline std::shared_ptr<SharedStructure_t> set_sharable(SharableKey key, std::shared_ptr<SharedStructure_t> shared) const {
            sharableStructures.emplace(key, std::make_unique<SharableStructureShrPtr<SharedStructure_t>>(shared));
            return shared;
        }

        template<typename SharedStructure_t>
        inline std::shared_ptr<SharedStructure_t> set_sharable(SharableKey key, std::shared_ptr<SharedStructure_t> shared) {
            sharableStructures[key] = std::make_unique<SharableStructureShrPtr<SharedStructure_t>>(shared);
            return shared;
        }

        template<typename SharedStructure_t>
        inline SharedStructure_t* set_sharable_ptr(SharableKey key, SharedStructure_t* shared) const {
            sharableStructuresPtr.emplace(key, std::make_unique<SharableStructurePtr<SharedStructure_t>>(shared));
            return shared;
        }

        template<typename SharedStructure_t>
        inline SharedStructure_t* set_sharable_ptr(SharableKey key, SharedStructure_t* shared) {
            sharableStructuresPtr[key] = std::make_unique<SharableStructurePtr<SharedStructure_t>>(shared);
            return shared;
        }

        template<typename SharedStructure_t>
        inline const SharedStructure_t* set_sharable_const(SharableKey key, const SharedStructure_t* shared) const {
            sharableStructuresConst.emplace(key, std::make_unique<SharableStructurePtr<const SharedStructure_t>>(shared));
            return shared;
        }

        template<typename SharedStructure_t>
        inline const SharedStructure_t* set_sharable_const(SharableKey key, const SharedStructure_t* shared) {
            sharableStructuresConst[key] = std::make_unique<SharableStructurePtr<const SharedStructure_t>>(shared);
            return shared;
        }

        void delete_sharable(SharableKey key);
        void delete_sharable_ptr(SharableKey key);
        void delete_sharable_const(SharableKey key);

        void load_sharable_from(const PLAJA::Configuration& other) const;
        void load_sharable(SharableKey key, const Configuration& other);
        void load_sharable_ptr(SharableKey key, const Configuration& other);
        void load_sharable_const(SharableKey key, const Configuration& other);

        [[nodiscard]] bool has_sharable(SharableKey key) const;

        template<typename SharedStructure_t>
        [[nodiscard]] inline std::shared_ptr<SharedStructure_t> get_sharable(SharableKey key) const {
            auto it = sharableStructures.find(key);
            if (it == sharableStructures.end()) { return nullptr; }
            else { return PLAJA_UTILS::cast_ref<SharableStructureShrPtr<SharedStructure_t>>(*it->second).operator()(); }
        }

        template<typename SharedStructure_t, typename Cast_t>
        [[nodiscard]] inline std::shared_ptr<Cast_t> get_sharable_cast(SharableKey key) const {
            std::shared_ptr<SharedStructure_t> rlt = get_sharable<SharedStructure_t>(key);
            if (not rlt) { return nullptr; }
            return std::static_pointer_cast<Cast_t>(rlt);
        }

        template<typename SharableStructure_t = SharableStructure>
        [[nodiscard]] inline std::shared_ptr<const SharableStructure_t> get_sharable_as_const(SharableKey key) const {
            return std::const_pointer_cast<SharableStructure_t>(get_sharable<SharableStructure_t>(key));
        }

        [[nodiscard]] bool has_sharable_ptr(SharableKey key) const;

        template<typename SharedStructure_t>
        [[nodiscard]] inline SharedStructure_t* get_sharable_ptr(SharableKey key) const {
            auto it = sharableStructuresPtr.find(key);
            if (it == sharableStructuresPtr.end()) { return nullptr; }
            else { return PLAJA_UTILS::cast_ref<SharableStructurePtr<SharedStructure_t>>(*it->second).operator()(); }
        }

        [[nodiscard]] bool has_sharable_const(SharableKey key) const;

        template<typename SharedStructure_t>
        [[nodiscard]] inline const SharedStructure_t* get_sharable_const(SharableKey key) const {
            auto it = sharableStructuresConst.find(key);
            if (it == sharableStructuresConst.end()) { return nullptr; }
            else { return PLAJA_UTILS::cast_ref<SharableStructurePtr<const SharedStructure_t>>(*it->second).operator()(); }
        }

        template<typename SharedStructure_t>
        [[nodiscard]] inline const SharedStructure_t* require_sharable_const(SharableKey key) const {
            PLAJA_ASSERT(has_sharable_const(key))
            return get_sharable_const<SharedStructure_t>(key);
        }

    };

}

#endif //PLAJA_CONFIGURATION_H
