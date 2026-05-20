# Bibliography

This bibliography is deliberately small. The manual is a companion to
the SMS++ source code, Doxygen, and Wiki, which remain the
authoritative references for the software itself; the entries below
cover the reference paper for the framework and the foundational
methodological works for the decomposition and nonsmooth-optimization
algorithms that SMS++ implements and that are mentioned in the text.

## The framework

1. A. Frangioni, L. Galli, N. Iardella, R. Durbano Lobato.
   *SMS++: a Software Framework for Structured Optimization.*
   Reference paper for the SMS++ project, Dipartimento di Informatica,
   Università di Pisa.

2. The SMS++ project: source repository
   <https://gitlab.com/smspp/smspp-project>, API reference (Doxygen)
   <https://smspp.gitlab.io/smspp-project/>, and project Wiki
   <https://gitlab.com/smspp/smspp-project/-/wikis/home>. Version
   covered by this manual: 0.6.0 (December 2025).

## Decomposition and nonsmooth optimization

3. J. F. Benders. *Partitioning procedures for solving mixed-variables
   programming problems.* Numerische Mathematik **4** (1962), 238–252.
   The original Benders decomposition, the scheme underlying
   `BendersBFunction` (Chapter 14) and Recipe R5.

4. G. B. Dantzig, P. Wolfe. *Decomposition principle for linear
   programs.* Operations Research **8**(1) (1960), 101–111. The
   Dantzig–Wolfe reformulation dual to the Lagrangian view used in
   Chapter 14 and Recipes R3–R4.

5. A. Frangioni. *About Lagrangian Methods in Integer Optimization.*
   Annals of Operations Research **139** (2005), 163–193. Background
   for the Lagrangian dual computed by `LagBFunction` and
   `LagrangianDualSolver` (Chapter 14, Recipe R4).

6. A. Frangioni. *Standard Bundle Methods: Untrusted Models and
   Duality.* In: A. M. Bagirov, M. Gaudioso, N. Karmitsa, M. M.
   Mäkelä, S. Taheri (eds.), *Numerical Nonsmooth Optimization*,
   Springer, 2020, pp. 61–116. The bundle-method theory behind
   `BundleSolver`, the engine of the Lagrangian dual (Chapters 13–14).

7. M. V. F. Pereira, L. M. V. G. Pinto. *Multi-stage stochastic
   optimization applied to energy planning.* Mathematical Programming
   **52** (1991), 359–375. Stochastic Dual Dynamic Programming, the
   archetype application of the polyhedral / Benders machinery
   referenced in Chapter 13.

## Serialisation

8. R. Rew, G. Davis. *NetCDF: an interface for scientific data
   access.* IEEE Computer Graphics and Applications **10**(4) (1990),
   76–82. The serialisation format used throughout SMS++ (Chapter 18).

## Funding acknowledgement

9. **RESILIENT** — *Resilient Energy System Infrastructure Layouts for
   Industry, E-Fuels and Network Transitions* — funded by the European
   Union under the Horizon Europe research and innovation programme,
   grant agreement no. 101069750
   (<https://resilient-project.github.io>). Much of the recent work on
   the SMS++ `Block` library documented here was carried out in the
   framework of this project; the acknowledgement follows the form
   indicated in the umbrella project's `README.md`.
