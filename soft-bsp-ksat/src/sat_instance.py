"""CNF generation/parsing and residual factor-graph state for BSP.

The residual graph is the single mutable structure used by survey
propagation and by the SID/BSP loop. Freeze/unfreeze match the C++
Clause::clean / Clause::build behaviour in Graph.cpp.
"""

from __future__ import annotations

from dataclasses import dataclass

import numpy as np

# Sentinel: this clause-position message has not been frozen for restore.
_NOT_FROZEN = -1.0


class BSPError(Exception):
    """Base class for solver failures."""


class ContradictionError(BSPError):
    """Empty unsatisfied clause."""


class SPNoConvergence(BSPError):
    """SP did not reach a fixed point in t_max iterations."""


class NegativeComplexityError(BSPError):
    """Complexity fell below the C++ abort threshold."""


def clause_count(n: int, alpha: float) -> int:
    """Match C++ Graph::write_on_file_graph: M = ceil(N * alpha) via truncate-then-increment."""
    m_float = float(n) * float(alpha)
    m = int(m_float)
    while m < m_float:
        m += 1
    return m


def generate_random_ksat(
    n: int,
    alpha: float,
    k: int = 3,
    seed: int | None = None,
) -> "SATFormula":
    """Random K-SAT in the Kautz–Selman makewff style used by the C++ solver.

    Each clause has K distinct variables; each literal is negated independently
    with probability 1/2. Python's RNG is not glibc random(), so instances are
    statistically the same ensemble, not bit-identical to a C++ --seed= run.
    """
    if n < k:
        raise ValueError(f"N={n} is smaller than K={k}")
    if k < 1:
        raise ValueError(f"K must be >= 1, got {k}")
    rng = np.random.default_rng(seed)
    m = clause_count(n, alpha)
    clauses = np.empty((m, k), dtype=np.int32)
    signs = np.empty((m, k), dtype=np.int8)
    for a in range(m):
        clauses[a] = rng.choice(n, size=k, replace=False)
        signs[a] = rng.choice(np.array([1, -1], dtype=np.int8), size=k)
    real_alpha = m / float(n)
    return SATFormula(n=n, k=k, alpha=real_alpha, clauses=clauses, signs=signs, seed=seed)


def dumps_dimacs(formula: "SATFormula", seed: int | None = None) -> str:
    """DIMACS CNF text, 1-based literals, compatible with the C++ reader."""
    n = formula.n
    m = formula.m
    k = formula.k
    seed_out = formula.seed if seed is None else seed
    if seed_out is None:
        seed_out = 0
    lines = [f"c seed={seed_out}", f"p cnf {n} {m}"]
    for a in range(m):
        lits = []
        for p in range(k):
            v = int(formula.clauses[a, p])
            if v < 0:
                continue
            s = int(formula.signs[a, p])
            lits.append(str(s * (v + 1)))
        lines.append(" ".join(lits) + " 0")
    return "\n".join(lines) + "\n"


def loads_dimacs(text: str) -> "SATFormula":
    """Parse DIMACS CNF (comments + p cnf header). Infers K as max clause length."""
    seed = None
    n = m = None
    raw: list[list[int]] = []
    for line in text.splitlines():
        s = line.strip()
        if not s:
            continue
        if s[0] in ("c", "C"):
            parts = s.split()
            for tok in parts:
                if tok.startswith("seed="):
                    try:
                        seed = int(tok[5:])
                    except ValueError:
                        pass
            continue
        if s[0] in ("p", "P"):
            bits = s.split()
            # p cnf N M
            if len(bits) >= 4 and bits[1].lower() == "cnf":
                n = int(bits[2])
                m = int(bits[3])
            continue
        if s[0] == "%":
            break
        vals = [int(x) for x in s.split()]
        clause: list[int] = []
        for x in vals:
            if x == 0:
                if clause:
                    raw.append(clause)
                    clause = []
            else:
                clause.append(x)
        if clause:
            # line without trailing 0: still accept
            raw.append(clause)
    if n is None:
        raise ValueError("DIMACS file missing 'p cnf' header")
    if m is None:
        m = len(raw)
    if not raw:
        k = 1
        clauses = np.full((0, k), -1, dtype=np.int32)
        signs = np.zeros((0, k), dtype=np.int8)
        return SATFormula(n=n, k=k, alpha=0.0, clauses=clauses, signs=signs, seed=seed)
    k = max(len(c) for c in raw)
    clauses = np.full((len(raw), k), -1, dtype=np.int32)
    signs = np.zeros((len(raw), k), dtype=np.int8)
    for a, cl in enumerate(raw):
        seen: set[int] = set()
        p = 0
        for lit in cl:
            v = abs(lit) - 1
            if v < 0 or v >= n:
                raise ValueError(f"literal {lit} out of range for N={n}")
            if v in seen:
                continue
            seen.add(v)
            clauses[a, p] = v
            signs[a, p] = 1 if lit > 0 else -1
            p += 1
    alpha = len(raw) / float(n) if n else 0.0
    return SATFormula(n=n, k=k, alpha=alpha, clauses=clauses, signs=signs, seed=seed)


def load_dimacs(path: str) -> "SATFormula":
    with open(path, encoding="utf-8") as f:
        return loads_dimacs(f.read())


def save_dimacs(formula: "SATFormula", path: str, seed: int | None = None) -> None:
    with open(path, "w", encoding="utf-8") as f:
        f.write(dumps_dimacs(formula, seed=seed))


@dataclass
class SATFormula:
    n: int
    k: int
    alpha: float
    clauses: np.ndarray  # (M, K) int32, -1 padded
    signs: np.ndarray  # (M, K) int8, +1 / -1
    seed: int | None = None

    @property
    def m(self) -> int:
        return int(self.clauses.shape[0])


def _build_csr(n: int, clauses: np.ndarray, signs: np.ndarray) -> tuple[
    np.ndarray, np.ndarray, np.ndarray, np.ndarray, np.ndarray, np.ndarray,
    list[list[tuple[int, int]]],
]:
    """CSR of plus/minus edges and per-variable incident (clause, pos) lists."""
    m, kmax = clauses.shape
    plus_lists: list[list[tuple[int, int]]] = [[] for _ in range(n)]
    minus_lists: list[list[tuple[int, int]]] = [[] for _ in range(n)]
    incident: list[list[tuple[int, int]]] = [[] for _ in range(n)]
    for a in range(m):
        for p in range(kmax):
            v = int(clauses[a, p])
            if v < 0:
                continue
            incident[v].append((a, p))
            if signs[a, p] > 0:
                plus_lists[v].append((a, p))
            else:
                minus_lists[v].append((a, p))

    def flatten(lists: list[list[tuple[int, int]]]) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
        ptr = np.zeros(n + 1, dtype=np.int32)
        for i, lst in enumerate(lists):
            ptr[i + 1] = ptr[i] + len(lst)
        a_idx = np.empty(ptr[n], dtype=np.int32)
        p_idx = np.empty(ptr[n], dtype=np.int32)
        for i, lst in enumerate(lists):
            for j, (a, p) in enumerate(lst):
                a_idx[ptr[i] + j] = a
                p_idx[ptr[i] + j] = p
        return ptr, a_idx, p_idx

    plus_ptr, plus_a, plus_p = flatten(plus_lists)
    minus_ptr, minus_a, minus_p = flatten(minus_lists)
    return plus_ptr, plus_a, plus_p, minus_ptr, minus_a, minus_p, incident


class FactorGraph:
    """Residual K-SAT factor graph with cavity messages on every original edge."""

    def __init__(self, formula: SATFormula, rng: np.random.Generator | None = None):
        if rng is None:
            rng = np.random.default_rng()
        self.formula = formula
        self.n = formula.n
        self.m = formula.m
        self.kmax = int(formula.clauses.shape[1])
        self.clauses = np.ascontiguousarray(formula.clauses, dtype=np.int32)
        self.signs = np.ascontiguousarray(formula.signs, dtype=np.int8)

        self.clause_len_init = np.zeros(self.m, dtype=np.int32)
        for a in range(self.m):
            self.clause_len_init[a] = int(np.sum(self.clauses[a] >= 0))
        self.clause_len = self.clause_len_init.copy()

        self.go_forward = np.zeros((self.m, self.kmax), dtype=np.bool_)
        for a in range(self.m):
            self.go_forward[a, : self.clause_len_init[a]] = True

        self.u = rng.random((self.m, self.kmax), dtype=np.float64)
        self.u[self.clauses < 0] = 0.0
        self.update = self.u.copy()
        self.old_s = self.u.copy()
        self.div_s = np.ones((self.m, self.kmax), dtype=np.float64)
        np.divide(1.0, 1.0 - self.u, out=self.div_s, where=self.u < 1.0)
        self.div_s[self.u >= 1.0] = 1.0e300

        self.u_frozen = np.full((self.m, self.kmax), _NOT_FROZEN, dtype=np.float64)
        self.vb = np.zeros((self.m, self.kmax), dtype=np.bool_)
        self.clause_sat = np.zeros(self.m, dtype=np.bool_)
        self.unsat_mask = np.ones(self.m, dtype=np.bool_)

        self.fixed = np.zeros(self.n, dtype=np.bool_)
        self.value = np.zeros(self.n, dtype=np.bool_)
        self.forced_by_up = np.zeros(self.n, dtype=np.bool_)
        self.fixed_list: list[int] = []

        (
            self.plus_ptr,
            self.plus_a,
            self.plus_p,
            self.minus_ptr,
            self.minus_a,
            self.minus_p,
            self.incident,
        ) = _build_csr(self.n, self.clauses, self.signs)

        self.degree = np.zeros(self.n, dtype=np.int32)
        for i in range(self.n):
            self.degree[i] = len(self.incident[i])

        self.prod_plus = np.ones(self.n, dtype=np.float64)
        self.prod_minus = np.ones(self.n, dtype=np.float64)
        self.sT = np.zeros(self.n, dtype=np.float64)
        self.sF = np.zeros(self.n, dtype=np.float64)
        self.sI = np.ones(self.n, dtype=np.float64)
        self.sC = np.zeros(self.n, dtype=np.float64)
        self.sC_at_freeze = np.zeros(self.n, dtype=np.float64)
        self.var_complexity = np.ones(self.n, dtype=np.float64)

        self.complexity = 0.0
        self.complexity_clauses = 0.0
        self.complexity_variables = 0.0
        self.eta = 0
        self.unit_prop_count = 0

    def copy(self) -> "FactorGraph":
        """Deep copy of residual state (messages, masks, assignment)."""
        other = FactorGraph.__new__(FactorGraph)
        other.formula = self.formula
        other.n = self.n
        other.m = self.m
        other.kmax = self.kmax
        other.clauses = self.clauses  # immutable
        other.signs = self.signs
        other.clause_len_init = self.clause_len_init.copy()
        other.clause_len = self.clause_len.copy()
        other.go_forward = self.go_forward.copy()
        other.u = self.u.copy()
        other.update = self.update.copy()
        other.old_s = self.old_s.copy()
        other.div_s = self.div_s.copy()
        other.u_frozen = self.u_frozen.copy()
        other.vb = self.vb.copy()
        other.clause_sat = self.clause_sat.copy()
        other.unsat_mask = self.unsat_mask.copy()
        other.fixed = self.fixed.copy()
        other.value = self.value.copy()
        other.forced_by_up = self.forced_by_up.copy()
        other.fixed_list = list(self.fixed_list)
        other.plus_ptr = self.plus_ptr
        other.plus_a = self.plus_a
        other.plus_p = self.plus_p
        other.minus_ptr = self.minus_ptr
        other.minus_a = self.minus_a
        other.minus_p = self.minus_p
        other.incident = self.incident
        other.degree = self.degree.copy()
        other.prod_plus = self.prod_plus.copy()
        other.prod_minus = self.prod_minus.copy()
        other.sT = self.sT.copy()
        other.sF = self.sF.copy()
        other.sI = self.sI.copy()
        other.sC = self.sC.copy()
        other.sC_at_freeze = self.sC_at_freeze.copy()
        other.var_complexity = self.var_complexity.copy()
        other.complexity = self.complexity
        other.complexity_clauses = self.complexity_clauses
        other.complexity_variables = self.complexity_variables
        other.eta = self.eta
        other.unit_prop_count = self.unit_prop_count
        return other

    @property
    def n_t(self) -> int:
        """L: number of unassigned variables (Parisi Figs. 1–2 abscissa)."""
        return self.n - len(self.fixed_list)

    @property
    def m_t(self) -> int:
        return int(np.count_nonzero(self.unsat_mask))

    def assignment_array(self) -> np.ndarray:
        """+1 / -1 / 0 for True / False / unassigned, length N, 0-based."""
        out = np.zeros(self.n, dtype=np.int8)
        for i in range(self.n):
            if self.fixed[i]:
                out[i] = 1 if self.value[i] else -1
        return out

    def freeze(self, var: int, value: bool, forced_by_up: bool = False) -> None:
        """Assign `var` and clean the residual graph (C++ decimate_one + clean)."""
        if self.fixed[var]:
            return
        self.fixed[var] = True
        self.value[var] = bool(value)
        self.forced_by_up[var] = bool(forced_by_up)
        self.sC_at_freeze[var] = float(self.sC[var])
        self.fixed_list.append(int(var))
        self.degree[var] = 0
        self._clean_var(var)

    def unfreeze(self, var: int) -> None:
        """Release a previously assigned variable (C++ reset + build)."""
        if not self.fixed[var]:
            return
        if var in self.fixed_list:
            self.fixed_list.remove(var)
        self.fixed[var] = False
        self.value[var] = False
        self.forced_by_up[var] = False
        self._build_var(var)

    def _clean_var(self, var: int) -> None:
        who = bool(self.value[var])
        for c, pos in self.incident[var]:
            # C++ stores the current survey, then overwrites it with 0/1.
            self.u_frozen[c, pos] = self.u[c, pos]
            if self.signs[c, pos] > 0:
                self.u[c, pos] = 0.0 if who else 1.0
                self.vb[c, pos] = who
            else:
                self.u[c, pos] = 0.0 if (not who) else 1.0
                self.vb[c, pos] = not who
            self.go_forward[c, pos] = False
            if self.clause_len[c] > 0:
                self.clause_len[c] -= 1
            was_sat = bool(self.clause_sat[c])
            self.clause_sat[c] = bool(np.any(self.vb[c, : self.clause_len_init[c]]))
            if (not was_sat) and self.clause_sat[c]:
                klen = int(self.clause_len_init[c])
                for p in range(klen):
                    if self.u_frozen[c, p] == _NOT_FROZEN:
                        self.u_frozen[c, p] = self.u[c, p]
                    self.u[c, p] = 0.0
                for p in range(klen):
                    if self.go_forward[c, p]:
                        v = int(self.clauses[c, p])
                        self.degree[v] -= 1
                self.unsat_mask[c] = False

    def _build_var(self, var: int) -> None:
        for c, pos in self.incident[var]:
            self.vb[c, pos] = False
            self.clause_sat[c] = bool(np.any(self.vb[c, : self.clause_len_init[c]]))
            self.go_forward[c, pos] = True
            self.clause_len[c] += 1
            if not self.clause_sat[c]:
                frozen_u = self.u_frozen[c, pos]
                if frozen_u != _NOT_FROZEN:
                    self.u[c, pos] = frozen_u
                if not self.unsat_mask[c]:
                    klen = int(self.clause_len_init[c])
                    for p in range(klen):
                        if self.go_forward[c, p] and self.u_frozen[c, p] != _NOT_FROZEN:
                            self.u[c, p] = self.u_frozen[c, p]
                            self.update[c, p] = self.u[c, p]
                            self.u_frozen[c, p] = _NOT_FROZEN
                    for p in range(klen):
                        if self.go_forward[c, p]:
                            v = int(self.clauses[c, p])
                            self.degree[v] += 1
                    self.unsat_mask[c] = True
                else:
                    if frozen_u != _NOT_FROZEN:
                        self.u[c, pos] = frozen_u
                        self.update[c, pos] = frozen_u
                        self.u_frozen[c, pos] = _NOT_FROZEN
                    self.degree[var] += 1
            else:
                self.u[c, pos] = 0.0

    def check_empty_unsat(self) -> None:
        empty = self.unsat_mask & (self.clause_len == 0)
        if np.any(empty):
            raise ContradictionError("empty unsatisfied clause")

    def unit_clauses(self) -> list[int]:
        return [int(a) for a in np.flatnonzero(self.unsat_mask & (self.clause_len == 1))]

    def freeze_to_satisfy_clause(self, clause: int) -> int:
        """Unit-propagate a length-1 unsat clause. Returns the fixed variable."""
        klen = int(self.clause_len_init[clause])
        pos = None
        for p in range(klen):
            if self.go_forward[clause, p]:
                pos = p
                break
        if pos is None:
            raise ContradictionError("unit propagation on empty clause")
        var = int(self.clauses[clause, pos])
        value = bool(self.signs[clause, pos] > 0)
        self.sT[var] = 1.0 if value else 0.0
        self.sF[var] = 0.0 if value else 1.0
        self.sI[var] = 0.0
        self.sC[var] = 1.0
        self.unit_prop_count += 1
        self.freeze(var, value, forced_by_up=True)
        return var

    def unit_propagate(self) -> int:
        """Cascade unit propagation. Returns number of variables fixed."""
        n_fixed = 0
        while True:
            self.check_empty_unsat()
            units = self.unit_clauses()
            if not units:
                break
            self.freeze_to_satisfy_clause(units[0])
            n_fixed += 1
        return n_fixed

    def residual_formula(self) -> SATFormula:
        """Unsatisfied clauses on currently unfixed variables, DIMACS-ready."""
        rows_v: list[list[int]] = []
        rows_s: list[list[int]] = []
        kmax = 1
        for a in range(self.m):
            if not self.unsat_mask[a]:
                continue
            vs: list[int] = []
            ss: list[int] = []
            for p in range(int(self.clause_len_init[a])):
                if not self.go_forward[a, p]:
                    continue
                vs.append(int(self.clauses[a, p]))
                ss.append(int(self.signs[a, p]))
            if vs:
                kmax = max(kmax, len(vs))
                rows_v.append(vs)
                rows_s.append(ss)
        m = len(rows_v)
        clauses = np.full((m, kmax), -1, dtype=np.int32)
        signs = np.zeros((m, kmax), dtype=np.int8)
        for a in range(m):
            L = len(rows_v[a])
            clauses[a, :L] = rows_v[a]
            signs[a, :L] = rows_s[a]
        alpha = m / float(self.n) if self.n else 0.0
        return SATFormula(
            n=self.n, k=kmax, alpha=alpha, clauses=clauses, signs=signs, seed=self.formula.seed,
        )

    def full_assignment_from_residual(self, residual_values: np.ndarray) -> np.ndarray:
        """Merge frozen values with a residual WalkSAT assignment (bool length N)."""
        out = residual_values.copy()
        for i in range(self.n):
            if self.fixed[i]:
                out[i] = bool(self.value[i])
        return out

    def formula_satisfied(self, assignment: np.ndarray) -> bool:
        """True if every original clause is satisfied by a complete bool assignment."""
        for a in range(self.m):
            ok = False
            for p in range(int(self.clause_len_init[a])):
                v = int(self.clauses[a, p])
                if v < 0:
                    continue
                lit_true = bool(assignment[v]) if self.signs[a, p] > 0 else (not bool(assignment[v]))
                if lit_true:
                    ok = True
                    break
            if not ok:
                return False
        return True
