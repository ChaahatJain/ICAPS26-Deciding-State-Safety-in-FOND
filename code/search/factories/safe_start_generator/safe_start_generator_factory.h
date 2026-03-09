//
// Created by Daniel Sherbakov in 2025.
//

#ifndef SAFE_START_GENERATOR_FACTORY_H
#define SAFE_START_GENERATOR_FACTORY_H

#include "../smt_base/smt_base_factory.h"


class SafeStartGeneratorFactory final: public SMTBaseFactory {
public:
    SafeStartGeneratorFactory();
    ~SafeStartGeneratorFactory() override;
    [[nodiscard]] bool supports_property_sub(const PropertyInformation&) const override;
    DELETE_CONSTRUCTOR(SafeStartGeneratorFactory)

    void add_options(PLAJA::OptionParser& option_parser) const override;
    void print_options() const override;

    void add_pa_cegar_options(PLAJA::OptionParser& option_parser) const;

    [[nodiscard]] std::unique_ptr<SearchEngine> construct(const PLAJA::Configuration& config) const override;

};



#endif //SAFE_START_GENERATOR_FACTORY_H
