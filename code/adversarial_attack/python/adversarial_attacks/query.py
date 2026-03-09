import enum
import torch
import json


class ConstraintOp(enum.Enum):
    LE = 0
    EQ = 1
    GE = 2

    def to_str(self):
        if self == ConstraintOp.LE:
            return "<="
        elif ConstraintOp.EQ:
            return "="
        else:
            return ">="

    @staticmethod
    def from_str(enum_str: str):
        if enum_str == "<=":
            return ConstraintOp.LE
        elif enum_str == "=":
            return ConstraintOp.EQ
        else:
            assert enum_str == ">="
            return ConstraintOp.GE


# [sum] [<=,=,=>] [scalar]
class Constraint:
    def __init__(self, op: int, scalar: int):
        self.variables = list()  # by index
        self.factors = list()
        self.op = ConstraintOp(op)
        self.scalar = scalar

    def add_addend(self, var: int, factor: int):
        assert(len(self.variables) == len(self.factors))
        self.variables.append(var)
        self.factors.append(factor)

    def dump(self):
        rlt = ""
        for var, factor in zip(self.variables, self.factors):
            rlt += "+ (" + str(factor) + " * " + "var" + str(var) + ") "
        rlt += self.op.to_str() + " " + str(self.scalar)
        print(rlt)


class Query:
    def __init__(self, input_size: int, num_constr_vars: int = 0):
        self.label = None
        self.constraints = list()
        self.inputSize = input_size
        # bounds by var index:
        self.lower_bounds = [1 for _ in range(0, input_size + num_constr_vars)]  # dummy value (invalid combination)
        self.upper_bounds = [-1 for _ in range(0, input_size + num_constr_vars)]  # dummy value (invalid combination)
        #
        self.solution_guess = None
        self.solution_vector = None
        self.expected_solution = None

    # aux

    @staticmethod
    def generate_bounds_as_vec(lower_bounds, upper_bounds):
        return torch.tensor([float(lower_bound) for lower_bound in lower_bounds]), torch.tensor([float(upper_bound) for upper_bound in upper_bounds])

    # construction

    def set_label(self, output_index: int):
        self.label = output_index

    def set_number_of_variables(self, num_vars: int):
        assert num_vars >= self.inputSize
        num_vars_old = self.get_number_of_variables()
        if num_vars <= num_vars_old:
            self.lower_bounds = self.lower_bounds[0:num_vars]
            self.upper_bounds = self.upper_bounds[0:num_vars]
        else:
            for i in range(0, num_vars - num_vars_old):
                self.lower_bounds.append(1)  # dummy value (invalid combination)
                self.upper_bounds.append(-1)  # dummy value (invalid combination)

    def set_bounds(self, index: int, lower_bound: int, upper_bound: int):
        assert index < self.get_number_of_variables()
        self.lower_bounds[index] = lower_bound
        self.upper_bounds[index] = upper_bound

    def add_constraint(self, constraint: Constraint):
        self.constraints.append(constraint)

    def clear_constraints(self):
        self.constraints.clear()

    def init_solution_guess(self):
        self.solution_guess = [val for val in self.lower_bounds]

    def set_solution_guess(self, index: int, value: int):
        assert 0 <= index <= len(self.solution_guess)
        assert self.lower_bounds[index] <= value <= self.upper_bounds[index]
        self.solution_guess[index] = value

    def reset(self):
        self.clear_constraints()
        self.solution_guess = None
        self.solution_vector = None

    # access

    def get_label(self):
        return self.label

    def get_number_of_variables(self):
        return len(self.lower_bounds)

    def get_input_size(self):
        return self.inputSize

    def get_input_bounds(self):
        return self.lower_bounds[0:self.inputSize], self.upper_bounds[0:self.inputSize]

    def generate_input_bound_vec(self):
        lower_bounds_, upper_bounds_ = self.get_input_bounds()
        return self.generate_bounds_as_vec(lower_bounds_, upper_bounds_)

    def get_number_of_constraint_vars(self):
        return len(self.lower_bounds) - self.inputSize

    def get_constraint_var_bounds(self):
        return self.lower_bounds[self.inputSize:], self.upper_bounds[self.inputSize:]

    def generate_constraint_var_bound_vec(self):
        lower_bounds_, upper_bounds_ = self.get_constraint_var_bounds()
        return self.generate_bounds_as_vec(lower_bounds_, upper_bounds_)

    def get_bounds(self):
        return self.lower_bounds, self.upper_bounds

    def generate_bound_vec(self):
        return self.generate_bounds_as_vec(self.lower_bounds, self.upper_bounds)

    def has_constraints(self) -> bool:
        return len(self.constraints) > 0

    def get_number_of_constraints(self):
        return len(self.constraints)

    def generate_factor_matrix(self):  # shape: #constraints x num variables
        factors = torch.zeros((self.get_number_of_constraints(), self.get_number_of_variables()))
        for i in range(0, self.get_number_of_constraints()):
            constraint = self.constraints[i]
            for (var, factor) in zip(constraint.variables, constraint.factors):
                factors[i, var] = factor
        return factors

    def generate_op_vec(self):
        return [constraint.op for constraint in self.constraints]

    def generate_scalar_vec(self):  # shape: #constraints x 1
        # scalars = torch.zeros((self.get_number_of_constraints(), 1))
        # for i in range(0, self.get_number_of_constraints()):
        #     scalars[i, 0] = self.constraints[i].scalar
        # return scalars
        #
        return torch.tensor([[float(constraint.scalar)] for constraint in self.constraints])

    def has_solution_guess(self) -> bool:
        return self.solution_guess is not None

    def generate_solution_guess(self):
        assert self.has_solution_guess()
        return torch.tensor([self.solution_guess])

    def has_solution(self) -> bool:
        return self.solution_vector is not None

    def extract_solution(self, index: int) -> int:
        assert self.has_solution()
        assert 0 <= index < len(self.solution_vector)
        return self.solution_vector[index]

    def has_expected_solution(self) -> bool:
        return self.expected_solution is not None

    def get_expected_solution(self) -> bool:
        return self.expected_solution

    ##

    # To be used for documentation and debugging purposes:
    def check_sanity(self, input_size: int, output_size: int):
        # input(-output) dimensions the query uses should match the expected ones
        assert self.inputSize == input_size
        # action sanity:
        assert self.label is not None
        assert 0 <= self.label < output_size  # action is an index in output range
        # Bound sanity:
        assert len(self.lower_bounds) >= input_size  # we need bounds at least for each NN input
        assert len(self.upper_bounds) >= input_size  # we need bounds at least for each NN input
        assert len(self.lower_bounds) == len(self.upper_bounds)
        # constraint sanity:
        num_variables = len(self.upper_bounds)
        # Constraints should only contain variables in range(num_variables),
        # i.e., for which there are bounds.
        # Moreover, non-input-variables are expected to occur *at most* once.
        # The latter is no actual requirement on the validity of the query,
        # however it is *currently* the expected for the queries that we consider.
        non_input_constraints = set(range(input_size, num_variables))
        for constraint in self.constraints:
            for var in constraint.variables:
                assert 0 <= var < num_variables
                if var >= input_size:  # non-input var
                    assert var in non_input_constraints
                    # non_input_constraints.remove(var)  # to trigger assert if it occurs again
        return True  # to encapsulate call to check_sanity within assert

    def dump(self):
        print("NN variables:")
        print(self.lower_bounds[:self.inputSize])
        print(self.upper_bounds[:self.inputSize])
        print("Constraint variables:")
        print(self.lower_bounds[self.inputSize:])
        print(self.upper_bounds[self.inputSize:])
        for constraint in self.constraints:
            constraint.dump()
        if self.solution_vector is not None:
            print("Found solution:")
            print(self.solution_vector)


# parsing from json ####################################################################################################

def parse_query(query_data: json, input_size: int):
    query = Query(input_size)
    # action
    query.set_label(query_data["label"])
    # bounds
    vars_data = query_data["vars"]
    query.set_number_of_variables(len(vars_data))
    for i in range(0, query.get_number_of_variables()):
        query.set_bounds(i, vars_data[i]["lower"], vars_data[i]["upper"])
    # constraints
    var_names = [var["name"] for var in query_data["vars"]]
    var_indexes = range(0, len(var_names))
    var_map = dict(zip(var_names, var_indexes))
    for constraint_data in query_data["constraints"]:
        constraint = Constraint(ConstraintOp.from_str(constraint_data["op"]).value, constraint_data["right(scalar)"])
        for addend in constraint_data["left(sum)"]:
            constraint.add_addend(var_map[addend["var"]], addend["factor"])
        query.add_constraint(constraint)
    # expected solution
    if "result" in query_data and query_data["result"] != "UNKNOWN":
        if query_data["result"] == "SAT":
            query.expected_solution = True
        else:
            assert query_data["result"] == "UNSAT"
            query.expected_solution = False
    #
    return query


def parse_queries(path_to_file: str, input_size: int):
    with open(path_to_file) as f:
        queries_data = json.load(f)
    return [parse_query(query_data, input_size) for query_data in queries_data["queries"]]
