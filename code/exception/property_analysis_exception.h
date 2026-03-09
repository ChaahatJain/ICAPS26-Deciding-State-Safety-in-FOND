//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PROPERTY_ANALYSIS_EXCEPTION_H
#define PLAJA_PROPERTY_ANALYSIS_EXCEPTION_H

#include "plaja_exception.h"

// extern:
namespace PLAJA_UTILS { extern const std::string emptyString; }

class PropertyAnalysisException: public PlajaException {
public:
    explicit PropertyAnalysisException(const std::string& property, const std::string& reason);
    explicit PropertyAnalysisException(const std::string& property, const std::string& engine, const std::string& reason);
    ~PropertyAnalysisException() override;
};

#endif //PLAJA_PROPERTY_ANALYSIS_EXCEPTION_H
