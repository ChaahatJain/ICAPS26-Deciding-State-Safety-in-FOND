//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SHARABLE_STRUCTURE_H
#define PLAJA_SHARABLE_STRUCTURE_H

#include <memory>
#include "../../utils/default_constructors.h"

namespace PLAJA {

class SharableStructure {
public:
    SharableStructure();
    virtual ~SharableStructure() = 0;
    DELETE_CONSTRUCTOR(SharableStructure)

    virtual std::unique_ptr<SharableStructure> copy() = 0;
};

template<typename Object_t>
class SharableStructureShrPtr final: public SharableStructure {
private:
    std::shared_ptr<Object_t> object;
public:
    explicit SharableStructureShrPtr(std::shared_ptr<Object_t> object_): object(object_) {}
    ~SharableStructureShrPtr() override = default;
    DELETE_CONSTRUCTOR(SharableStructureShrPtr)

    inline std::shared_ptr<Object_t> operator()() { return object; }

    std::unique_ptr<SharableStructure> copy() override { return std::make_unique<SharableStructureShrPtr<Object_t>>(object); }
};

template<typename Object_t>
class SharableStructurePtr final: public SharableStructure {
private:
    Object_t* object;
public:
    explicit SharableStructurePtr(Object_t* object_): object(object_) {}
    ~SharableStructurePtr() override = default;
    DELETE_CONSTRUCTOR(SharableStructurePtr)

    inline Object_t* operator()() { return object; }

    std::unique_ptr<SharableStructure> copy() override { return std::make_unique<SharableStructurePtr<Object_t>>(object); }
};

}

#endif //PLAJA_SHARABLE_STRUCTURE_H
