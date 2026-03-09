""" A method to return the projection onto hyperplanes problem, with integer-valued solution. """
import cvxpy as cp
import torch
import numpy  # do it in torch directly for c++ compatibility
from query import ConstraintOp

numpy.random.seed(0)  # TODO keep this for debugging purposes; same random seeds ...


class ProjectionOperator:

    def __init__(self, A_, op_, b_, x_bounds, pts: int = 20, constraint_delta_dir: bool = True):
        self.num_pts = pts
        self.constraint_delta_dir = constraint_delta_dir
        # we cache the used indexes here once
        self.idxs = torch.sort(torch.unique((A_.flatten().nonzero()) % A_.shape[1])).values  # unique() to remove duplicates
        self.A = A_[:, self.idxs]
        self.op = op_
        self.b = b_
        self.x_low = x_bounds[0][self.idxs]  # np.hstack() for decomposed bounds
        self.x_high = x_bounds[1][self.idxs]

    def delta_dir_constraint(self, delta_dir, pt_idx: int, delta_var):
        if self.constraint_delta_dir:
            delta_dir_pt = delta_dir[pt_idx][self.idxs]
            delta_dir_pt_max_index = delta_dir_pt.max(dim=0).indices
            assert len(delta_dir_pt_max_index.shape) == 0
            delta_dir_pt_max_dir = torch.sign(delta_dir_pt[delta_dir_pt_max_index])
            if delta_dir_pt_max_dir < 0:
                return [delta_var[delta_dir_pt_max_index] <= 0]
            elif delta_dir_pt_max_dir > 0:
                return [0 <= delta_var[delta_dir_pt_max_index]]
        # else:
        return list()

    def project_hyplane(self, full_tensor, delta_dir):
        total_pts = min(full_tensor.shape[0], self.num_pts)  # randomly select at most self.num_pts points to start
        found_idx = []

        for pt_idx in range(total_pts):
            # Construct a CVXPY problem
            x = full_tensor[pt_idx][self.idxs]
            delta = cp.Variable(len(self.idxs), integer=True)
            constraints = self.delta_dir_constraint(delta_dir, pt_idx, delta)
            constraints += [self.x_low <= delta + x, delta + x <= self.x_high]
            for i in range(len(self.op)):
                if self.op[i] == ConstraintOp.LE:
                    constraints += [self.A[i]@(x+delta) <= self.b[i]]
                elif self.op[i] == ConstraintOp.EQ:
                    constraints += [self.A[i]@(x+delta) == self.b[i]]
                elif self.op[i] == ConstraintOp.GE:
                    constraints += [self.A[i]@(x+delta) >= self.b[i]]

            objective = cp.Minimize(cp.norm(delta)) 
            prob = cp.Problem(objective, constraints)
            prob.solve(solver=cp.GUROBI, reoptimize=True, TimeLimit=2)

            if delta.value is not None:
                found_idx.append(pt_idx)
                full_tensor[pt_idx][self.idxs] = x + torch.tensor(delta.value, dtype=torch.float)

        return torch.unique(full_tensor[found_idx], dim=0) if len(found_idx) != 0 else None
