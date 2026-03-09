set(UTILS_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/utils.cpp ${CMAKE_CURRENT_LIST_DIR}/utils.h
    ${CMAKE_CURRENT_LIST_DIR}/rng.cpp ${CMAKE_CURRENT_LIST_DIR}/rng.h
)

# fd imports
include(${CMAKE_CURRENT_LIST_DIR}/fd_imports/PlaJAFiles.cmake)
list(APPEND UTILS_SOURCES ${FD_IMPORTS_SOURCES})
