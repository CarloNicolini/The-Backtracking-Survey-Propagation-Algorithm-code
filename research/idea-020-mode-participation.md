# Idea 020 — variable participation in the unstable SP mode

## Hypothesis

The full Lyapunov solve is too expensive per candidate, but one solve at the
current checkpoint yields a dominant right tangent vector. Its spatial
support may predict which variable fixation changes stability.

For each free variable i record:

- incoming participation: the sum of squared dominant tangent components on
  clause-to-i messages;
- neighborhood participation: the tangent energy of every incident clause,
  including its messages to other variables.

The neighborhood quantity represents the part of the dominant mode touched
when fixing i modifies all incident clauses.

At the already measured normal and fatal checkpoints, correlate these
participations with the exact post-fix `rho` for every variable/direction.
Also compare the participation ranks of certainty, polarization, the
most-stable move, and the first stable move in retention order.

No policy is implemented unless participation predicts the signed post-fix
stability response. A large mode component alone is ambiguous: removing it
may stabilize the map, while shortening its clauses may destabilize it.

## Result

At the normal checkpoint, neighborhood participation has Spearman correlation
0.20 with post-fix rho; incoming participation has 0.14. At the fatal
checkpoint the correlations are -0.06 and -0.02. Degree is equally weak.

The fatal certainty variable has low participation, while both high- and
low-participation moves can be stable. Right-mode energy therefore does not
predict the signed effect of fixing a variable.

Decision: kill this proxy. The next mathematically justified quantity is the
biorthogonal left-right sensitivity `w_e * v_e`, or more generally the
adjoint response `w^T delta(J_i) v`.
