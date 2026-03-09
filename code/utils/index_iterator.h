//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_INDEX_ITERATOR_H
#define PLAJA_INDEX_ITERATOR_H

#define ITERATE_INDEX(IT_VAR, SIZE_VAR, START, SIZE, CORE) \
{\
const std::size_t SIZE_VAR = SIZE;\
for (std::size_t IT_VAR = START; (IT_VAR) < (SIZE_VAR); ++(IT_VAR)) { CORE }\
}

#endif //PLAJA_INDEX_ITERATOR_H
