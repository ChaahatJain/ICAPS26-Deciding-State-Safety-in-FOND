#ifndef PLAJA_REDUCTION_FACTORY_H
#define PLAJA_REDUCTION_FACTORY_H

#include "../search_engine_factory.h"

class ReductionFactory: public SearchEngineFactory {

private:
    [[nodiscard]] bool supports_property_sub(const PropertyInformation& prop_info) const override;
    
public:
    ReductionFactory();
    ~ReductionFactory() override;
    DELETE_CONSTRUCTOR(ReductionFactory)

    void add_options(PLAJA::OptionParser& option_parser) const override;
    void print_options() const override;

    [[nodiscard]] std::unique_ptr<SearchEngine> construct(const PLAJA::Configuration& config) const override;
};

#endif
