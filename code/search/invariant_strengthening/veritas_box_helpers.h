#ifndef VERITAS_BOX_HELPERS_H
#define VERITAS_BOX_HELPERS_H

#include "interval.hpp"
#include "../smt_ensemble/using_veritas.h"
#include "../../parser/ast/model.h"
#include "../../parser/ast/expression/expression.h"
#include "../information/model_information.h"
#include <iostream>

veritas::AddTree boxTree(const std::vector<veritas::Interval>& box, int num_actions);

std::unique_ptr<Expression> box_to_condition(const Model& model, const std::vector<veritas::Interval>& box);

int get_union_index(const std::vector<veritas::Interval>& box1, const std::vector<veritas::Interval>& box2);
int check_exact_overlap(const std::vector<veritas::Interval>& box1, const std::vector<veritas::Interval>& box2, const int union_index);
bool union_possible(const std::vector<veritas::Interval>& box1, const std::vector<veritas::Interval>& box2);
veritas::Interval unionize(veritas::Interval bound1, veritas::Interval bound2);
std::vector<veritas::Interval> compute_union(const std::vector<veritas::Interval> box1, const std::vector<veritas::Interval> box2);
std::vector<std::vector<veritas::Interval>> merge_boxes_once(std::vector<std::vector<veritas::Interval>> boxes);
std::vector<std::vector<veritas::Interval>> merge_recursive(std::vector<std::vector<veritas::Interval>> boxes);

void dump(std::vector<veritas::Interval> box);
bool areBoxesEqual(const std::vector<veritas::Interval>& box_a, const std::vector<veritas::Interval>& box_b);

#endif //VERITAS_BOX_HELPERS_H
