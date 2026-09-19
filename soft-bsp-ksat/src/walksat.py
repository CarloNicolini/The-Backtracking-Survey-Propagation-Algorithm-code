"""Compact WalkSAT for the paramagnetic residual formula.

This is not a line-by-line port of walksat.cpp (Selman–Kautz v35). It is the
same algorithm family: pick an unsatisfied clause, then with probability p flip
a random literal, otherwise flip the literal of that clause with smallest break
count. Frozen variables are never flipped.
"""

from __future__ import annotations

import numpy as np


def walksat(
    n: int,
    clauses: np.ndarray,
    signs: np.ndarray,
    assignment: np.ndarray,
    frozen: np.ndarray | None = None,
    max_flips: int = 100_000,
    p_random: float = 0.5,
    rng: np.random.Generator | None = None,
) -> tuple[bool, np.ndarray]:
    """Solve a CNF with optional frozen bits.

    Parameters
    ----------
    clauses, signs :
        Shape (M, K), -1 padded variable indices and ±1 signs.
    assignment :
        Bool array length N; frozen entries are kept, free entries are a start.
    frozen :
        Bool mask of variables that must not flip. None means nothing frozen.

    Returns
    -------
    (success, assignment)
    """
    if rng is None:
        rng = np.random.default_rng()
    assign = assignment.astype(np.bool_, copy=True)
    if frozen is None:
        frozen = np.zeros(n, dtype=np.bool_)
    m, kmax = clauses.shape

    def lit_true(a: int, pos: int) -> bool:
        v = int(clauses[a, pos])
        if v < 0:
            return False
        if signs[a, pos] > 0:
            return bool(assign[v])
        return not bool(assign[v])

    def clause_sat(a: int) -> bool:
        has_lit = False
        for pos in range(kmax):
            if clauses[a, pos] < 0:
                continue
            has_lit = True
            if lit_true(a, pos):
                return True
        return not has_lit

    unsat = [a for a in range(m) if not clause_sat(a)]
    if not unsat:
        return True, assign

    # Precompute occurrence lists of unfrozen vars.
    occ: list[list[int]] = [[] for _ in range(n)]
    for a in range(m):
        for pos in range(kmax):
            v = int(clauses[a, pos])
            if v >= 0:
                occ[v].append(a)

    def break_count(var: int) -> int:
        """How many currently-sat clauses would become unsat if var flips."""
        brk = 0
        new_val = not bool(assign[var])
        for a in occ[var]:
            if not clause_sat(a):
                continue
            still = False
            for pos in range(kmax):
                v = int(clauses[a, pos])
                if v < 0:
                    continue
                if v == var:
                    lit = (signs[a, pos] > 0 and new_val) or (signs[a, pos] < 0 and not new_val)
                else:
                    lit = lit_true(a, pos)
                if lit:
                    still = True
                    break
            if not still:
                brk += 1
        return brk

    for _ in range(max_flips):
        if not unsat:
            return True, assign
        a = int(unsat[int(rng.integers(0, len(unsat)))])
        lits = []
        for pos in range(kmax):
            v = int(clauses[a, pos])
            if v >= 0 and not frozen[v]:
                lits.append(v)
        if not lits:
            # Residual clause whose every variable is frozen unsat: unsolvable.
            return False, assign
        if rng.random() < p_random:
            var = int(lits[int(rng.integers(0, len(lits)))])
        else:
            best = None
            best_brk = 10**9
            rng.shuffle(lits)
            for v in lits:
                b = break_count(v)
                if b < best_brk:
                    best_brk = b
                    best = v
            var = int(best)
        assign[var] = not bool(assign[var])
        # Refresh unsat list locally.
        unsat = [c for c in range(m) if not clause_sat(c)]

    return False, assign
