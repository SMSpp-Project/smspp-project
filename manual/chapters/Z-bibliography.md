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
   covered by this manual: 0.8.0 (November 2026).

## Decomposition and nonsmooth optimization

3. J. F. Benders. *Partitioning procedures for solving mixed-variables
   programming problems.* Numerische Mathematik **4** (1962), 238–252,
   doi:10.1007/BF01386316.
   The original Benders decomposition, the scheme underlying
   `BendersBFunction` ([Chapter 14](14-lag-benders-bfunction.md#ch-14)) and [Recipe R5](R5-cfl-benders.md#rec-R5).

4. A. Daniilidis, C. Lemaréchal. *On a primal-proximal heuristic in
   discrete optimization.* Mathematical Programming **104** (2005),
   105–128, doi:10.1007/s10107-004-0571-2. The heuristic implemented
   by `PrimalProximalHeur` ([Recipe R4](R4-cfl-lagrangian.md#rec-R4)).

5. G. B. Dantzig, P. Wolfe. *Decomposition principle for linear
   programs.* Operations Research **8**(1) (1960), 101–111,
   doi:10.1287/opre.8.1.101. The
   Dantzig–Wolfe reformulation dual to the Lagrangian view used in
   [Chapter 14](14-lag-benders-bfunction.md#ch-14) and Recipes [R3](R3-cfl-three-ways.md#rec-R3)–[R4](R4-cfl-lagrangian.md#rec-R4).

6. A. Frangioni. *About Lagrangian Methods in Integer Optimization.*
   Annals of Operations Research **139** (2005), 163–193,
   doi:10.1007/s10479-005-3447-9. Background
   for the Lagrangian dual computed by `LagBFunction` and
   `LagrangianDualSolver` ([Chapter 14](14-lag-benders-bfunction.md#ch-14), [Recipe R4](R4-cfl-lagrangian.md#rec-R4)).

7. A. Frangioni. *Standard Bundle Methods: Untrusted Models and
   Duality.* In: A. M. Bagirov, M. Gaudioso, N. Karmitsa, M. M.
   Mäkelä, S. Taheri (eds.), *Numerical Nonsmooth Optimization*,
   Springer, 2020, pp. 61–116, doi:10.1007/978-3-030-34910-3_3. The
   bundle-method theory behind
   `BundleSolver`, the engine of the Lagrangian dual (Chapters [13](13-function-family.md#ch-13)–[14](14-lag-benders-bfunction.md#ch-14)).

8. T. L. Magnanti, R. T. Wong. *Accelerating Benders decomposition:
   algorithmic enhancement and model selection criteria.* Operations
   Research **29**(3) (1981), 464–484, doi:10.1287/opre.29.3.464. The
   Pareto-optimal Benders cuts, compared on the CFL in [Recipe R5](R5-cfl-benders.md#rec-R5).

9. M. V. F. Pereira, L. M. V. G. Pinto. *Multi-stage stochastic
   optimization applied to energy planning.* Mathematical Programming
   **52** (1991), 359–375, doi:10.1007/BF01582895. Stochastic Dual
   Dynamic Programming, the
   archetype application of the polyhedral / Benders machinery
   referenced in [Chapter 13](13-function-family.md#ch-13).

## Serialisation

10. R. Rew, G. Davis. *NetCDF: an interface for scientific data
    access.* IEEE Computer Graphics and Applications **10**(4) (1990),
    76–82, doi:10.1109/38.56302. The serialisation format used
    throughout SMS++ ([Chapter 18](18-factories-netcdf.md#ch-18)).

## Funding acknowledgement

11. **RESILIENT** — *Resilient Energy System Infrastructure Layouts for
    Industry, E-Fuels and Network Transition* — a project of the Clean
    Energy Transition (CET) Partnership, co-funded by the European
    Union under the Horizon Europe research and innovation programme,
    grant agreement no. 101069750
    (<https://resilient-project.github.io>). Much of the recent work on
    the SMS++ `Block` library documented here was carried out in the
    framework of this project; the acknowledgement follows the form
    indicated in the umbrella project's `README.md`.
