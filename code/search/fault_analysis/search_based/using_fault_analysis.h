
// Nice to have:
/*
    Datastructure to keep track of unsafe paths
    Datastructure to keep track of policy statistics when generating these unsafe paths
*/
#ifndef PLAJA_USING_FAULT_ANALYSIS_H
#define PLAJA_USING_FAULT_ANALYSIS_H

#include <vector>

namespace PolicySampled {
struct Path {
    std::vector<StateID_type> states;
    std::vector<ActionLabel_type> actions;
};

struct Stats { int goal_count; int avoid_count; int cycle_count; int terminal_count; int num_paths; };
}

#endif // PLAJA_USING_FAULT_ANALYSIS_H
