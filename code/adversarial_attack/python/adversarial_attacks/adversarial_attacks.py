from os_env import *  # to set thread limits
import torch
from networks import NetworkOld
from networks import Network
from query import ConstraintOp, Query, parse_queries
from projection_cvxpy import ProjectionOperator
import numpy as np
import time
from typing import Tuple

torch.manual_seed(0)
torch.set_num_threads(1)
torch.set_num_interop_threads(1)


class AdversarialAttacks(object):

    def __init__(self, path_to_network: str, input_size: int, hidden_layer_sizes: list, output_size: int,
                 attack_starts: int, pgd_steps: int, random_step_bound: int = 0, scale_step: int = 0, constraint_delta_dir: bool = False):
        if path_to_network.endswith(".pth"):
            self.nn = NetworkOld(input_size, hidden_layer_sizes, output_size)
            self.nn.load_state_dict(torch.load(path_to_network))
        else:
            self.nn = Network(input_size, hidden_layer_sizes, output_size)
            self.nn.load_from_pt(torch.jit.load(path_to_network))

        # input and output dimensions for this attack instance (fixed for all queries)
        self.inputSize = input_size
        self.outputSize = output_size
        #
        self.proj_points = attack_starts
        self.pgd_steps = pgd_steps
        self.random_step_bound = random_step_bound  # random step enabled if > 0 -> random step in [-random_step_bound, random_step_bound)
        self.scale_step = scale_step  # scale at least one delta component to scale step (i.e., no effect for 0)
        self.only_project_at_success = True  # we only project if the current solution solves the NN selection constraint
        self.constraint_delta_dir = constraint_delta_dir
        # self.proj_rate = 2  # pgd steps per projection (deprecated)
        self.proj_steps = self.pgd_steps  # // self.proj_rate

        # stats
        self.query_solved_a_start = False
        self.num_attack_steps = 0
        self.num_projections = 0
        # advanced stats:
        self.do_advanced_stats = False
        self.max_delta = 0
        self.duplicates = 0
        self.queries_solved_with_duplicates = 0
        self.seen_duplicate = False
        self.seen_vectors = None

    # noinspection PyAttributeOutsideInit
    def set_query(self, query: Query):
        assert query.check_sanity(self.inputSize, self.outputSize)
        # action
        self.outputIndex = query.get_label()
        # vars
        self.numVars = query.get_number_of_variables()  # also referred to as dimension?
        self.lowerBounds, self.upperBounds = query.generate_bound_vec()  # tensors for all bounds
        self.n_l, self.n_h = query.generate_input_bound_vec()  # tensors for input bounds
        # sub-domains for random starts:
        self.subdomainT = [torch.arange(lower_b, upper_b + 1).tolist() for lower_b, upper_b in zip(self.lowerBounds, self.upperBounds)]
        # constraints
        self.hasConstr = query.has_constraints()
        self.A_vec = query.generate_factor_matrix()
        self.op_vec = (query.generate_op_vec())  # this is an enum type list
        self.b_vec = query.generate_scalar_vec()
        # create CVXPY-projection class object:
        self.proj_obj = ProjectionOperator(self.A_vec, self.op_vec, self.b_vec, [self.lowerBounds, self.upperBounds], pts=self.proj_points, constraint_delta_dir=self.constraint_delta_dir) if self.hasConstr else None
        #
        self.reset_stats()

    def set_solution(self, query: Query, full_tensor: torch.Tensor, solution_indexes) -> None:
        query.solution_vector = full_tensor[solution_indexes[0]].clone().detach()
        assert self.check_all(query.solution_vector)
        assert isinstance(query.solution_vector, torch.Tensor)
        assert self.is_1_dim_vec(query.solution_vector)  # expect single vector

########################################################################################################################

    # stats

    def is_query_solved_at_start(self) -> bool:
        return self.query_solved_a_start

    def get_num_attack_steps(self) -> int:
        return self.num_attack_steps

    def get_num_projections(self) -> int:
        return self.num_projections

    def reset_stats(self):
        # stats
        self.query_solved_a_start = False
        self.num_attack_steps = 0
        self.num_projections = 0
        # advanced stats:
        if self.do_advanced_stats:
            # self.max_delta = 0  # TODO for now keep stats over queries
            # self.duplicates = 0
            # self.queries_solved_with_duplicates = 0
            self.seen_duplicate = False
            self.seen_vectors = torch.empty(0, self.numVars)

    def print_advanced_stats(self):
        if self.do_advanced_stats:
            print("Max delta: " + str(float(self.max_delta)))
            print("Duplicates: " + str(self.duplicates))
            print("Queries solved with duplicates: " + str(self.queries_solved_with_duplicates))

    def add_to_seen(self, full_tensor: torch.Tensor):
        if self.do_advanced_stats:
            old_num_tensors = self.seen_vectors.shape[0]
            self.seen_vectors = torch.cat((self.seen_vectors, full_tensor), dim=0).unique(dim=0).detach()
            num_duplicates = full_tensor.shape[0] - (self.seen_vectors.shape[0] - old_num_tensors)
            self.duplicates += num_duplicates
            self.seen_duplicate = True if num_duplicates > 0 else False
            return

    def set_result_stats(self, rlt: bool):
        if self.do_advanced_stats and rlt:
            self.queries_solved_with_duplicates += (1 if self.seen_duplicate else 0)

########################################################################################################################

    @staticmethod
    def is_1_dim_vec(tensor: torch.Tensor) -> bool:
        return len(tensor.shape) == 1

    # condition checks

    def is_integer(self, x: torch.Tensor) -> bool:
        assert self.is_1_dim_vec(x)  # support for single vector
        return (x == torch.round(x)).all()

    def is_bounded(self, x, integer: bool = True) -> bool:
        assert self.is_1_dim_vec(x)  # support for single vector
        if integer and not self.is_integer(x):
            return False
        return torch.less_equal(self.lowerBounds, x).all() and torch.less_equal(x, self.upperBounds).all()

    def is_bounded_constraint(self, x: torch.Tensor, integer: bool = True) -> bool:
        assert self.is_1_dim_vec(x)  # support for single vector
        if not self.is_bounded(x, integer):
            return False
        if self.hasConstr:
            for i in range(len(self.op_vec)):
                if self.op_vec[i] == ConstraintOp.LE and not (self.A_vec[i] @ x <= self.b_vec[i]):
                    return False
                elif self.op_vec[i] == ConstraintOp.EQ and not (self.A_vec[i] @ x == self.b_vec[i]):
                    return False
                elif self.op_vec[i] == ConstraintOp.GE and not (self.A_vec[i] @ x >= self.b_vec[i]):
                    return False
        return True

    def label_is_selected(self, x: torch.Tensor) -> bool:  # i.e., target class found
        assert self.is_1_dim_vec(x)  # support for single vector
        return torch.argmax(self.nn(x[:self.inputSize])) == self.outputIndex

    def check_all(self, x: torch.Tensor, integer: bool = True) -> bool:
        # Affirm target class found + integer within dimension bounds + inequalities satisfied
        return self.is_bounded_constraint(x, integer) and self.label_is_selected(x)

    # simple bound projections

    @staticmethod
    def proj_bounds(x: torch.Tensor, lower_b: torch.Tensor, upper_b: torch.Tensor, integer: bool) -> torch.Tensor:
        assert x.shape[1] == lower_b.shape[0] == upper_b.shape[0]
        x = torch.maximum(torch.minimum(x, upper_b), lower_b)
        return torch.round(x) if integer else x

    def proj_input_bounds(self, x: torch.Tensor, integer: bool):
        return self.proj_bounds(x, self.n_l, self.n_h, integer)

    def proj_full_bounds(self, x: torch.Tensor, integer: bool):
        return self.proj_bounds(x, self.lowerBounds, self.upperBounds, integer)

########################################################################################################################

    def sample_starts(self) -> torch.Tensor:
        # shuffle(subdomainT) create a self.numVars x self.proj_points*10 matrix (*10 is used to account for potential repetitions)
        arr_init = np.empty((self.numVars, self.proj_points*10))
        for i in range(self.numVars):
            arr_init[i, :] = np.random.choice(self.subdomainT[i],  self.proj_points*10)
        full_tensor_gt = torch.as_tensor(np.unique(arr_init, axis=1).transpose()).float()  # i.p. dropping duplicates
        indices = torch.randperm(full_tensor_gt.shape[0])
        # return tensor with at most self.proj_points points (after accounting for duplicates)
        return full_tensor_gt[indices[: min(full_tensor_gt.shape[0], self.proj_points)]]

    # gradient step

    @staticmethod
    def alpha_sched(step, tot_steps=10):
        # Schedule on the step-size for PGD steps - > decaying step-size
        return (5 - 0.5) * ((1 - (step / tot_steps)) ** 1.2) + 0.5

    def scale_delta(self, delta: torch.Tensor) -> torch.Tensor:
        delta_max = torch.abs(delta).max(dim=1)
        if self.scale_step > 0:
            delta_max_value = max(delta_max.values)  # for simplicity, we scale only over all attack point; considering a single one most likely anyway
            assert delta_max_value > 0
            if delta_max_value <= self.scale_step:
                delta_scale = 1 / delta_max_value
                delta = delta_scale * delta
        if self.do_advanced_stats:
            self.max_delta = max(self.max_delta, max(delta_max.values))  # stats
        # delta_dir = [(index, torch.sign(delta[point, index])) for point, index in zip(range(0, delta.shape[0]), delta_max.indices)] # delta sign/direction can be used to mitigate "back"-projections
        return delta  # , delta_dir # do sign operations within projection instead ...

    def loss(self, inp: torch.Tensor, output_target: torch.Tensor) -> torch.Tensor:
        # margin loss and pointwise
        output = self.nn(inp)
        ll = output[:, output_target].sum()  # here we add each entry n times, where n = attack points # should divide by "/ inp.shape[0]" ???
        # ??? # ll = output.gather(dim=1, index=output_target.unsqueeze(1)).sum().detach() # <- without detach there is an issue with gradient computation
        output[:, output_target] = -float('inf')
        loss_cal = ll - output.max(dim=1).values.sum()

        return loss_cal

    # step: disable for now
    # idx: torch.ones(output_target.shape, dtype=torch.bool) and y_target != torch.argmax(self.nn(x_adv), dim=1)
    # -> we stop once we have a solution so no need to restrict the set of updated indexes
    def grad_steps(self, input_tensor: torch.Tensor, output_target: torch.Tensor, alpha):
        # baseline ####################
        if self.random_step_bound > 0:
            delta = (2 * self.random_step_bound) * torch.rand(input_tensor.size()) - self.random_step_bound  # random delta in [-self.random_step_bound, self.random_step_bound)
            delta = self.scale_delta(delta)
            return self.proj_input_bounds(input_tensor + delta, integer=False), delta
        ###############################
        # computes the gradient steps
        delta = torch.zeros_like(input_tensor, requires_grad=True)
        loss_cal = self.loss(input_tensor + delta, output_target)
        loss_cal.backward()
        delta.grad /= torch.norm(delta.grad)
        delta.data = (delta + alpha*delta.grad)  # "[idx] = [idx]" (disabled index restriction; see above); step * (disabled)
        delta.grad.zero_()
        delta = self.scale_delta(delta)
        return self.proj_input_bounds(input_tensor + delta, integer=False), delta  # disabled integer rounding after each step (as rounding keeps attack point at start)

    #

    def check_success(self, x: torch.Tensor, round_integer: bool):
        # check if any point has successfully landed in outputIndex class
        input_tensor = torch.round(x[:, :self.inputSize]) if round_integer else x[:, :self.inputSize]
        labels = torch.argmax(self.nn(input_tensor), dim=1)
        suc_indexes = (labels == self.outputIndex).nonzero(as_tuple=True)[0]  # indexes for which the desired output is max
        return len(suc_indexes) > 0, suc_indexes

    def proj_or_noproj(self, full_tensor: torch.Tensor, delta_dir: torch.Tensor, force_projection: bool = False):
        # project only at success option
        if self.only_project_at_success and not force_projection:
            tully, suc_idx = self.check_success(full_tensor, round_integer=True)
            if not tully:
                return tully, suc_idx, full_tensor
            elif not self.hasConstr:  # special case for successful unconstrained queries
                return tully, suc_idx, self.proj_full_bounds(full_tensor, integer=True)
        #
        assert all([self.is_bounded(x, integer=False) for x in full_tensor])  # bounds should hold throughout ...
        full_tensor = self.proj_full_bounds(full_tensor, integer=True)  # ... but still need to round to integer (i.p. projection assumes integer tensor)
        #
        if self.hasConstr and not all([self.is_bounded_constraint(x, integer=False) for x in full_tensor]):
            # TODO might move check *per* point into projection class, then skip projection for point; however only minor gain in runtime expected (gurobi encoding offset) + currently only consider single point anyway
            ################################
            full_tensor = self.proj_obj.project_hyplane(full_tensor, delta_dir)  # .from_numpy()
            self.num_projections += (1 if not force_projection else 0)  # stats
            if full_tensor is None:
                return False, None, None
            # sanity: the constraints must be satisfied after a successful projection call
            assert all([self.is_bounded_constraint(x, integer=True) for x in full_tensor])
            #
            return self.check_success(full_tensor, round_integer=False) + tuple([full_tensor])
        else:
            # unconstrained or constraints already sat
            return self.check_success(full_tensor, round_integer=False) + tuple([full_tensor])

    ########################################################################################################################

    def check_query(self, query: Query) -> bool:
        # check whether there exists (respectively try to find) an input state x_in within the input bounds such that the output for output_index is maximal,
        # i.e., NN(x_in)(output_index) >= NN(x_in)(other_output_index) for all other_output_index != output_index
        # and additionally there exists x_constr within the bounds of the additional constraint variables
        # such that A (x_in, x_constraints) o b, i.e., the query constraints are satisfied.
        self.set_query(query)

        full_tensor = query.generate_solution_guess() if query.has_solution_guess() else self.sample_starts()
        delta_dir = torch.zeros_like(full_tensor)

        # project if necessary and check success
        tully, suc_idxs, full_tensor = self.proj_or_noproj(full_tensor, delta_dir, True)
        if full_tensor is None:  # if no feasible point end and return False
            self.query_solved_a_start += 1
            return False
        if tully:  # already successful
            self.query_solved_a_start += 1
            self.set_solution(query, full_tensor, suc_idxs)
            return True
        #
        input_tensor = full_tensor[:, :self.inputSize]
        output_target = torch.ones((input_tensor.shape[0])).type(torch.LongTensor) * self.outputIndex

        # PGD steps
        for tw in range(self.proj_steps):
            self.num_attack_steps += 1  # stats
            self.add_to_seen(full_tensor)  # stats

            alpha = (self.n_h - self.n_l) / self.alpha_sched(tw, tot_steps=self.proj_steps)

            # for i in range(self.proj_rate):
            # step = 0.2/(i+1)
            x_adv, delta_dir[:, :self.inputSize] = self.grad_steps(input_tensor, output_target, alpha=alpha)

            # update full tensor
            full_tensor[:, :self.inputSize] = x_adv.clone().detach()

            # projection on polytope (linear constraints as well as both box_constraints)
            # however, box constraints are also enforced at each grad step
            tully, suc_idx, full_tensor = self.proj_or_noproj(full_tensor, delta_dir)
            if full_tensor is None:  # if no feasible point end and return False
                self.set_result_stats(False)  # stats
                return False
            if tully:
                self.set_result_stats(True)  # stats
                self.set_solution(query, full_tensor, suc_idx)
                return True
            input_tensor = full_tensor[:, :self.inputSize].clone()

        self.set_result_stats(False)  # stats
        return False

########################################################################################################################


if __name__ == '__main__':
    from python_utils import PythonUtils

    queries_per_nn = list()
    nn_paths = list()
    hidden_sizes = [[16, 16], [32, 32], [64, 64]]
    for nn_postfix in ["16_16", "32_32", "64_64"]:
        dir_patch1 = os.getcwd() + "/../networks/blocksworld_4_3/pa_compact_starts_blocksworld_4_3_" + nn_postfix + "/"
        nn_paths.append(dir_patch1 + "blocksworld_4_3_" + nn_postfix + ".pt")
        queries_of_nn = list()
        for query_cache in ["1", "2", "3", "4"]:
            for addendum in ["selection", "applicability", "transition"][2:]:
                for file in PythonUtils.extract_files(dir_patch1 + "query_cache_" + query_cache + "/" + addendum + "_tests/"):
                    print(file)
                    # for query in parse_queries(file, 10):
                    #    if query.get_number_of_constraints() > 0:
                    #        queries_of_nn.append(query)
                    queries_of_nn += parse_queries(file, 10)
        queries_per_nn.append(queries_of_nn)
    print([len(queries) for queries in queries_per_nn])
    print(sum([len(queries) for queries in queries_per_nn]))

    # sample old
    # queries_per_nn = [parse_queries("../networks/blocksworld_4_3/pa_compact_starts_blocksworld_4_3_64_64/query_cache_1/transition_tests/query_5.json", 10)]
    # nn_paths = ["../networks/blocksworld_4_3/pa_compact_starts_blocksworld_4_3_64_64/blocksworld_4_3_64_64.pt"]
    # hidden_sizes = [[64, 64]]

    # sample new
    queries_per_nn = [parse_queries("../networks/blocks_4_3_new/transition_tests/query_2.json", 10)]
    nn_paths = ["/Users/mv/PlaJA/benchmarks/blocksworld/networks/blocksworld_4_3/learned_no_cost/blocksworld_4_3_16_16.pt"]

    in_size = 10  # size of input to the neural networks
    # hidd_size = [64, 64]  # hidden layer size for NN
    num_pgd_steps = 25  # No. of PGD steps to do
    num_attack_starts = 1  # No. of Projection points to start with
    out_size = 5

    strt_time = time.time()
    num_queries = 0
    successful = 0
    sat_queries = 0
    sat_queries_successful = 0
    unsat_queries = 0
    queries_solved_at_start = 0
    attack_steps = 0
    projections = 0
    time_for_suc = 0
    time_for_non_suc = 0
    time_for_sat_successful = 0
    time_for_sat_non_suc = 0
    time_for_unsat = 0
    for queries_of_nn, nn_path, hidden_size in zip(queries_per_nn, nn_paths, hidden_sizes):
        attack = AdversarialAttacks(nn_path, in_size, hidden_size, out_size, attack_starts=num_attack_starts, pgd_steps=num_pgd_steps)
        for query_of_nn in queries_of_nn:
            num_queries += 1
            if num_queries % 100 == 0:
                print(num_queries)
            time_offset = time.time()
            found = attack.check_query(query_of_nn)
            queries_solved_at_start += (1 if attack.is_query_solved_at_start() else 0)
            attack_steps += attack.get_num_attack_steps()
            projections += attack.get_num_projections()
            time_diff = time.time() - time_offset
            if found:
                assert not attack.hasConstr or not query_of_nn.has_expected_solution() or query_of_nn.get_expected_solution()
                time_for_suc += time_diff
                successful += 1
            else:
                time_for_non_suc += time_diff
            if query_of_nn.has_expected_solution():
                if query_of_nn.get_expected_solution():
                    sat_queries += 1
                    sat_queries_successful += (1 if found else 0)
                    if found:
                        time_for_sat_successful += time_diff
                    else:
                        time_for_sat_non_suc += time_diff
                else:
                    unsat_queries += 1
                    time_for_unsat += time_diff
        print("Total time: " + str(time.time() - strt_time))
        attack.print_advanced_stats()

    total_time = time.time() - strt_time
    print("Total number queries: " + str(num_queries))
    print("Successful queries: " + str(successful))
    print("SR: " + str(successful / num_queries))
    print("Total number satisfiable queries: " + str(sat_queries))
    print("Successful satisfiable queries: " + str(sat_queries_successful))
    print("SR: " + str(sat_queries_successful / sat_queries))
    print("Total number unsatisfiable queries: " + str(unsat_queries))
    print("Queries solved at start: " + str(queries_solved_at_start))
    print("Number of attack steps: " + str(attack_steps))
    print("Number of projections: " + str(projections))
    #
    print("Total time: " + str(total_time))
    print("Total time successful queries: " + str(time_for_suc))
    print("Total time for unsuccessful queries: " + str(time_for_non_suc))
    print("Time per successful query: " + str(time_for_suc / successful))
    print("Time per unsuccessful query: " + str(time_for_non_suc / (num_queries - successful)))
    #
    print("Total time successful satisfiable queries: " + str(time_for_sat_successful))
    print("Total time unsuccessful satisfiable queries: " + str(time_for_sat_non_suc))
    print("Total time unsatisfiable queries: " + str(time_for_unsat))
    print("Time per successful satisfiable query: " + str(time_for_sat_successful / sat_queries_successful))
    print("Time per non-successful satisfiable query: " + str(time_for_sat_non_suc / (sat_queries - sat_queries_successful)))
    print("Time per unsatisfiable query: " + str(time_for_unsat / unsat_queries))
    #
