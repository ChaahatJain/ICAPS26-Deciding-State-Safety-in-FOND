//
// Created by Chaahat Jain on 04/11/24.
//

#include <utility>
#include <vector>
#include <stack>
#include <unordered_map>
#include <algorithm>
#include "../../../../successor_generation/simulation_environment.h"
#include "../../../../fd_adaptions/state.h"
#include "../../../../../parser/ast/expression/non_standard/objective_expression.h"
#include <chrono>
#include "../../../../../stats/stats_base.h"


class TarjanSCC {
public:
    TarjanSCC(const StateID_type& root, const std::shared_ptr<SimulationEnvironment>& env, const ObjectiveExpression* obj, const Expression* av, std::shared_ptr<PLAJA::StatsBase> stats)
        : searchStatistics(std::move(stats)), root(root), simulatorEnv(env), index(0), objective(obj), avoid(av) {}

    std::vector<std::vector<StateID_type>> computeSCCs() {
        /* A recursive algorithm. This runs out of memory very easily. */
        if (nodeIndex.find(root) == nodeIndex.end()) {
            strongConnect(root);
        }
        return sccs;
    }

    std::vector<std::vector<StateID_type>> computeSCCIterative(std::chrono::time_point<std::chrono::steady_clock> start_time, int seconds_per_query) {
        /* An iterative version of Tarjans algorithm. This does not run out of memory. */
        return find_sccs(start_time, seconds_per_query);
    }

private:
    std::shared_ptr<PLAJA::StatsBase> searchStatistics;
    StateID_type root; // The root node to start the search
    std::shared_ptr<SimulationEnvironment> simulatorEnv; // Successor generator
    std::unordered_map<StateID_type, int> nodeIndex;     // DFS index of each node
    std::unordered_map<StateID_type, int> lowLink;       // Lowest index reachable from each node
    std::stack<StateID_type> stack;                      // Stack for Tarjan's algorithm
    std::unordered_map<StateID_type, bool> onStack;      // To check if a node is on the stack
    std::vector<std::vector<StateID_type>> sccs;         // List of SCCs
    int index;                                           // Current DFS index
    const ObjectiveExpression* objective;
    const Expression* avoid;

    bool check_avoid(const State& state) const {
        PLAJA_ASSERT(avoid)
        return avoid->evaluateInteger(&state);
    }

    bool check_goal(const State& state) const {
        auto* goal = objective->get_goal();
        return goal and goal->evaluateInteger(&state);
    }

    void strongConnect(StateID_type node) {
        // Initialize node index and low-link value
        nodeIndex[node] = lowLink[node] = index++;
        stack.push(node);
        onStack[node] = true;

        // Explore all neighbors from current state
        const auto currentState = simulatorEnv->get_state(node).to_ptr();
        const auto applicable_actions = simulatorEnv->extract_applicable_actions(*currentState, false);
        if (!check_avoid(*currentState) and !check_goal(*currentState) and !applicable_actions.empty()) {
            for (const auto& action: applicable_actions) {
                const auto successors = simulatorEnv->compute_successors(*currentState, action);
                for (const StateID_type& neighbor: successors) {
                    if (nodeIndex.find(neighbor) == nodeIndex.end()) {
                        // Neighbor has not been visited, recurse
                        strongConnect(neighbor);
                        lowLink[node] = std::min(lowLink[node], lowLink[neighbor]);
                    } else if (onStack[neighbor]) {
                        // Neighbor is in the stack, part of the current SCC
                        lowLink[node] = std::min(lowLink[node], nodeIndex[neighbor]);
                    }
                }
            }
        }

        // If node is a root node, pop the stack to form an SCC
        if (lowLink[node] == nodeIndex[node]) {
            std::vector<StateID_type> scc;
            StateID_type w;
            do {
                w = stack.top();
                stack.pop();
                onStack[w] = false;
                scc.push_back(w);
            } while (w != node);
            sccs.push_back(scc);
        }
    }


    std::vector<StateID_type> get_successors(const StateID_type node) const {
        // Get successors by iterating over all applicable actions
        const auto currentState = simulatorEnv->get_state(node).to_ptr();
        const auto applicable_actions = simulatorEnv->extract_applicable_actions(*currentState, false);
        if (check_avoid(*currentState) or check_goal(*currentState) or applicable_actions.empty()) {
            return {};
        }
        std::set<StateID_type> successors;
        for (const auto& action: applicable_actions) {
            const auto succs = simulatorEnv->compute_successors(*currentState, action);
            for (auto s: succs) {
                successors.insert(s);
            }
        }
        // convert std::set successors to std::vector
        std::vector v( successors.begin(), successors.end());
        return v;
    }

    static bool time_limit_reached(const std::chrono::time_point<std::chrono::steady_clock> start_time, const std::chrono::time_point<std::chrono::steady_clock> current_time, int seconds_per_query) { return std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count() >= seconds_per_query; }


    std::vector<std::vector<StateID_type>> find_sccs(std::chrono::time_point<std::chrono::steady_clock> start_time, int seconds_per_query) {
        // Initialize data structures
        std::stack<StateID_type> tarjan_stack;
        std::unordered_set<StateID_type> on_stack;
        std::unordered_map<StateID_type, int> indices;
        std::unordered_map<StateID_type, int> low_links;
        std::vector<std::vector<StateID_type>> sccs;

        std::stack<std::pair<StateID_type, int>> dfs_stack; // Simulates recursive DFS
        dfs_stack.emplace(root, 0);      // Start DFS from the root
        while (!dfs_stack.empty()) {
            const auto current_time = std::chrono::steady_clock::now();
            if (time_limit_reached(start_time, current_time, seconds_per_query)) {
                return {};
            }
            auto [node, neighbor_index] = dfs_stack.top();
            dfs_stack.pop();

            if (indices.find(node) == indices.end()) {
                // First time visiting the node
                indices[node] = low_links[node] = index++;
                tarjan_stack.push(node);
                on_stack.insert(node);
            }

            const auto& neighbors = get_successors(node);
            if (neighbor_index < neighbors.size()) {
                // Unfinished neighbors
                StateID_type neighbor = neighbors[neighbor_index];
                dfs_stack.emplace(node, neighbor_index + 1); // Save state for current node

                if (indices.find(neighbor) == indices.end()) {
                    // Neighbor is unvisited
                    dfs_stack.emplace(neighbor, 0); // Visit neighbor
                } else if (on_stack.find(neighbor) != on_stack.end()) {
                    // Neighbor is in the current SCC
                    low_links[node] = std::min(low_links[node], indices[neighbor]);
                }
            } else {
                // Finished processing all neighbors of `node`
                if (low_links[node] == indices[node]) {
                    // Found an SCC
                    std::vector<StateID_type> scc;
                    StateID_type w;
                    do {
                        w = tarjan_stack.top();
                        tarjan_stack.pop();
                        on_stack.erase(w);
                        scc.push_back(w);
                    } while (w != node);
                    sccs.push_back(scc);
                }

                // Update the parent's low-link value
                if (!dfs_stack.empty()) {
                    auto parent = dfs_stack.top().first;
                    low_links[parent] = std::min(low_links[parent], low_links[node]);
                }
            }
        }
        return sccs;
    }
};
