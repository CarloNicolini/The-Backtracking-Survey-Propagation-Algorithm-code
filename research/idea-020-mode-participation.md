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
