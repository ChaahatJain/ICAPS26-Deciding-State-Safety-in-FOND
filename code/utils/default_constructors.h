//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_DEFAULT_CONSTRUCTORS_H
#define PLAJA_DEFAULT_CONSTRUCTORS_H

#define DEFAULT_CONSTRUCTOR(NAME)\
NAME(const NAME& other) = default;\
NAME(NAME&& other) = default;\
NAME& operator=(const NAME& other) = default;\
NAME& operator=(NAME&& other) = default;

#define DEFAULT_CONSTRUCTOR_ONLY(NAME)\
NAME(const NAME& other) = default;\
NAME(NAME&& other) = default;\

#define DELETE_CONSTRUCTOR(NAME)\
NAME(const NAME& other) = delete;\
NAME(NAME&& other) = delete;\
NAME& operator=(const NAME& other) = delete;\
NAME& operator=(NAME&& other) = delete;

#define DELETE_ASSIGNMENT(NAME)\
NAME& operator=(const NAME& other) = delete;\
NAME& operator=(NAME&& other) = delete;

#endif //PLAJA_DEFAULT_CONSTRUCTORS_H
