# Bibliography

This bibliography is kept to what the text refers to. The manual is a
companion to the SMS++ source code, Doxygen, and Wiki, which remain the
authoritative references for the software itself; the entries below are
the reference paper and the sources of the framework, the works on which
the algorithms implemented by SMS++, and described in the text, are
based, which the chapters cite as author and year, and the method, the
standard, the format and the project that the text names in words (the
last four sections).

## The framework

<span id="ref-smspp-paper"></span>
1. A. Frangioni, N. Iardella, R. Lobato. *A Structured Modelling System
   for Large-Scale Optimization.* Optimization Methods and Software, to
   appear.

<span id="ref-smspp-project"></span>
2. The SMS++ project: source repository
   <https://gitlab.com/smspp/smspp-project>, API reference (Doxygen)
   <https://smspp.gitlab.io/smspp-project/>, and project Wiki
   <https://gitlab.com/smspp/smspp-project/-/wikis/home>. Version
   covered by this manual: 0.8.0 (November 2026), with what has entered
   the development branch since, marked "since SMS++ 0.9.0".

## Decomposition and nonsmooth optimization

<span id="ref-baena2020"></span>
3. D. Baena, J. Castro, A. Frangioni. *Stabilized Benders methods for
   large-scale combinatorial optimization, with application to data
   privacy.* Management Science **66**(7) (2020), 3051–3068,
   <https://doi.org/10.1287/mnsc.2019.3341>.

<span id="ref-bahiense2002"></span>
4. L. Bahiense, N. Maculan, C. Sagastizábal. *The volume algorithm
   revisited: relation with bundle methods.* Mathematical Programming
   **94**(1) (2002), 41–69, <https://doi.org/10.1007/s10107-002-0357-3>.

<span id="ref-barahona2000"></span>
5. F. Barahona, R. Anbil. *The volume algorithm: producing primal
   solutions with a subgradient method.* Mathematical Programming
   **87**(3) (2000), 385–399, <https://doi.org/10.1007/s101070050002>.

<span id="ref-benders1962"></span>
6. J. F. Benders. *Partitioning procedures for solving mixed-variables
   programming problems.* Numerische Mathematik **4** (1962), 238–252,
   <https://doi.org/10.1007/BF01386316>. The original Benders
   decomposition, the scheme underlying `BendersBFunction` ([Chapter
   14](14-lag-benders-bfunction.md#ch-14)) and [Recipe
   R5](R5-cfl-benders.md#rec-R5).

<span id="ref-besancon2022"></span>
7. M. Besançon, A. Carderera, S. Pokutta. *FrankWolfe.jl: a
   high-performance and flexible toolbox for Frank-Wolfe algorithms and
   conditional gradients.* INFORMS Journal on Computing **34**(5)
   (2022), 2611–2620, <https://doi.org/10.1287/ijoc.2022.1191>.

<span id="ref-codato2006"></span>
8. G. Codato, M. Fischetti. *Combinatorial Benders' cuts for
   mixed-integer linear programming.* Operations Research **54**(4)
   (2006), 756–766, <https://doi.org/10.1287/opre.1060.0286>.

<span id="ref-daniilidis2005"></span>
9. A. Daniilidis, C. Lemaréchal. *On a primal-proximal heuristic in
   discrete optimization.* Mathematical Programming **104** (2005),
   105–128, <https://doi.org/10.1007/s10107-004-0571-2>. The heuristic
   implemented by `PrimalProximalHeur` ([Recipe
   R4](R4-cfl-lagrangian.md#rec-R4)).

<span id="ref-dantzig1960"></span>
10. G. B. Dantzig, P. Wolfe. *Decomposition principle for linear
    programs.* Operations Research **8**(1) (1960), 101–111,
    <https://doi.org/10.1287/opre.8.1.101>. The Dantzig–Wolfe
    reformulation dual to the Lagrangian view used in [Chapter
    14](14-lag-benders-bfunction.md#ch-14) and Recipes
    [R3](R3-cfl-three-ways.md#rec-R3)–[R4](R4-cfl-lagrangian.md#rec-R4).

<span id="ref-demelas2025"></span>
11. F. Demelas, J. Le Roux, A. Frangioni, M. Lacroix, E. Traversi, R.
    Wolfler Calvo. *Bundle Network: a machine learning-based bundle
    method.* arXiv:2509.24736 (2025),
    <https://doi.org/10.48550/arXiv.2509.24736>.

<span id="ref-deoliveira2016"></span>
12. W. de Oliveira, M. Solodov. *A doubly stabilized bundle method for
    nonsmooth convex optimization.* Mathematical Programming
    **156**(1–2) (2016), 125–159,
    <https://doi.org/10.1007/s10107-015-0873-6>.

<span id="ref-fischetti2010"></span>
13. M. Fischetti, D. Salvagnin, A. Zanette. *A note on the selection of
    Benders' cuts.* Mathematical Programming **124**(1–2) (2010),
    175–182, <https://doi.org/10.1007/s10107-010-0365-7>.

<span id="ref-frangioni1996"></span>
14. A. Frangioni. *Solving semidefinite quadratic problems within
    nonsmooth optimization algorithms.* Computers & Operations Research
    **23**(11) (1996), 1099–1118,
    <https://doi.org/10.1016/0305-0548(96)00006-8>.

<span id="ref-frangioni2002"></span>
15. A. Frangioni. *Generalized bundle methods.* SIAM Journal on
    Optimization **13**(1) (2002), 117–156,
    <https://doi.org/10.1137/S1052623498342186>.

<span id="ref-frangioni2005"></span>
16. A. Frangioni. *About Lagrangian methods in integer optimization.*
    Annals of Operations Research **139** (2005), 163–193,
    <https://doi.org/10.1007/s10479-005-3447-9>. Background for the
    Lagrangian dual computed by `LagBFunction` and
    `LagrangianDualSolver` ([Chapter
    14](14-lag-benders-bfunction.md#ch-14), [Recipe
    R4](R4-cfl-lagrangian.md#rec-R4)).

<span id="ref-frangioni2014"></span>
17. A. Frangioni, E. Gorgone. *Bundle methods for sum-functions with
    "easy" components: applications to multicommodity network design.*
    Mathematical Programming **145**(1–2) (2014), 133–161,
    <https://doi.org/10.1007/s10107-013-0642-3>.

<span id="ref-frangioni2017"></span>
18. A. Frangioni, B. Gendron, E. Gorgone. *On the computational
    efficiency of subgradient methods: a case study with Lagrangian
    bounds.* Mathematical Programming Computation **9**(4) (2017),
    573–604, <https://doi.org/10.1007/s12532-017-0120-7>.

<span id="ref-frangioni2020"></span>
19. A. Frangioni. *Standard bundle methods: untrusted models and
    duality.* In: A. M. Bagirov, M. Gaudioso, N. Karmitsa, M. M. Mäkelä,
    S. Taheri (eds.), *Numerical Nonsmooth Optimization: State of the Art
    Algorithms*, Springer,
    2020, pp. 61–116, <https://doi.org/10.1007/978-3-030-34910-3_3>. The
    bundle-method theory behind `BundleSolver` ([Chapter
    19](19-nondifferentiable.md#ch-19)).

<span id="ref-frank1956"></span>
20. M. Frank, P. Wolfe. *An algorithm for quadratic programming.* Naval
    Research Logistics Quarterly **3**(1–2) (1956), 95–110,
    <https://doi.org/10.1002/nav.3800030109>.

<span id="ref-fumero2001"></span>
21. F. Fumero. *A modified subgradient algorithm for Lagrangean
    relaxation.* Computers & Operations Research **28**(1) (2001),
    33–52, <https://doi.org/10.1016/S0305-0548(99)00085-4>.

<span id="ref-geoffrion1974"></span>
22. A. M. Geoffrion. *Lagrangean relaxation for integer programming.*
    Mathematical Programming Study **2** (1974), 82–114,
    <https://doi.org/10.1007/BFb0120690>. The equality of the Lagrangian
    dual and of the convexified relaxation ([Theorem
    14.1](14-lag-benders-bfunction.md#thm-14-1) and [Recipe
    R4](R4-cfl-lagrangian.md#rec-R4)).

<span id="ref-guelat1986"></span>
23. J. Guélat, P. Marcotte. *Some comments on Wolfe's "away step".*
    Mathematical Programming **35**(1) (1986), 110–119,
    <https://doi.org/10.1007/BF01589445>.

<span id="ref-jaggi2013"></span>
24. M. Jaggi. *Revisiting Frank-Wolfe: projection-free sparse convex
    optimization.* Proceedings of the 30th International Conference on
    Machine Learning (ICML 2013), PMLR **28**(1), 427–435.

<span id="ref-kelley1960"></span>
25. J. E. Kelley, Jr. *The cutting-plane method for solving convex
    programs.* Journal of the Society for Industrial and Applied
    Mathematics **8**(4) (1960), 703–712,
    <https://doi.org/10.1137/0108053>.

<span id="ref-larsson1999"></span>
26. T. Larsson, M. Patriksson, A.-B. Strömberg. *Ergodic, primal
    convergence in dual subgradient schemes for convex programming.*
    Mathematical Programming **86**(2) (1999), 283–312,
    <https://doi.org/10.1007/s101070050090>.

<span id="ref-lemarechal1995"></span>
27. C. Lemaréchal, A. Nemirovskii, Yu. Nesterov. *New variants of bundle
    methods.* Mathematical Programming **69**(1–3) (1995), 111–147,
    <https://doi.org/10.1007/BF01585555>.

<span id="ref-magnanti1981"></span>
28. T. L. Magnanti, R. T. Wong. *Accelerating Benders decomposition:
    algorithmic enhancement and model selection criteria.* Operations
    Research **29**(3) (1981), 464–484,
    <https://doi.org/10.1287/opre.29.3.464>. The Pareto-optimal Benders
    cuts, compared on the CFL in [Recipe R5](R5-cfl-benders.md#rec-R5).

<span id="ref-nesterov2009"></span>
29. Yu. Nesterov. *Primal-dual subgradient methods for convex problems.*
    Mathematical Programming **120**(1) (2009), 221–259,
    <https://doi.org/10.1007/s10107-007-0149-x>.

<span id="ref-papadakos2008"></span>
30. N. Papadakos. *Practical enhancements to the Magnanti-Wong method.*
    Operations Research Letters **36**(4) (2008), 444–449,
    <https://doi.org/10.1016/j.orl.2008.01.005>.

<span id="ref-polyak1969"></span>
31. B. T. Polyak. *Minimization of unsmooth functionals.* USSR
    Computational Mathematics and Mathematical Physics **9**(3) (1969),
    14–29, <https://doi.org/10.1016/0041-5553(69)90061-5>.

<span id="ref-sherali2013"></span>
32. H. D. Sherali, B. J. Lunday. *On generating maximal nondominated
    Benders cuts.* Annals of Operations Research **210**(1) (2013),
    57–72, <https://doi.org/10.1007/s10479-011-0883-6>.

<span id="ref-tsuji2022"></span>
33. K. Tsuji, K. Tanaka, S. Pokutta. *Pairwise conditional gradients
    without swap steps and sparser kernel herding.* Proceedings of the
    39th International Conference on Machine Learning (ICML 2022), PMLR
    **162**, 21864–21883.

<span id="ref-vanackooij2016"></span>
34. W. van Ackooij, A. Frangioni, W. de Oliveira. *Inexact stabilized
    Benders' decomposition approaches with application to
    chance-constrained problems with finite support.* Computational
    Optimization and Applications **65**(3) (2016), 637–669,
    <https://doi.org/10.1007/s10589-016-9851-z>.

<span id="ref-vanackooij2018"></span>
35. W. van Ackooij, A. Frangioni. *Incremental bundle methods using
    upper models.* SIAM Journal on Optimization **28**(1) (2018),
    379–410, <https://doi.org/10.1137/16M1089897>.

## Branch-and-bound and branching

<span id="ref-achterberg2005"></span>
36. T. Achterberg, T. Koch, A. Martin. *Branching rules revisited.*
    Operations Research Letters **33**(1) (2005), 42–54,
    <https://doi.org/10.1016/j.orl.2004.04.002>.

<span id="ref-achterberg2007"></span>
37. T. Achterberg. *Constraint Integer Programming.* PhD thesis,
    Technische Universität Berlin, 2007,
    <https://doi.org/10.14279/depositonce-1634>.

<span id="ref-archibald2018"></span>
38. B. Archibald, P. Maier, C. McCreesh, R. Stewart, P. Trinder.
    *Replicable parallel branch and bound search.* Journal of Parallel
    and Distributed Computing **113** (2018), 92–114,
    <https://doi.org/10.1016/j.jpdc.2017.10.010>.

<span id="ref-benichou1971"></span>
39. M. Bénichou, J. M. Gauthier, P. Girodet, G. Hentges, G. Ribière, O.
    Vincent. *Experiments in mixed-integer linear programming.*
    Mathematical Programming **1**(1) (1971), 76–94,
    <https://doi.org/10.1007/BF01584074>.

<span id="ref-gendron1994"></span>
40. B. Gendron, T. G. Crainic. *Parallel branch-and-bound algorithms:
    survey and synthesis.* Operations Research **42**(6) (1994),
    1042–1066, <https://doi.org/10.1287/opre.42.6.1042>.

<span id="ref-khalil2016"></span>
41. E. B. Khalil, P. Le Bodic, L. Song, G. Nemhauser, B. Dilkina.
    *Learning to branch in mixed integer programming.* Proceedings of
    the AAAI Conference on Artificial Intelligence **30**(1) (2016),
    <https://doi.org/10.1609/aaai.v30i1.10080>.

<span id="ref-lawler1966"></span>
42. E. L. Lawler, D. E. Wood. *Branch-and-bound methods: a survey.*
    Operations Research **14**(4) (1966), 699–719,
    <https://doi.org/10.1287/opre.14.4.699>.

<span id="ref-morrison2016"></span>
43. D. R. Morrison, S. H. Jacobson, J. J. Sauppe, E. C. Sewell.
    *Branch-and-bound algorithms: a survey of recent advances in
    searching, branching, and pruning.* Discrete Optimization **19**
    (2016), 79–102, <https://doi.org/10.1016/j.disopt.2016.01.005>.

## Knapsack and assignment problems

<span id="ref-cattrysse1992"></span>
44. D. G. Cattrysse, L. N. Van Wassenhove. *A survey of algorithms for
    the generalized assignment problem.* European Journal of Operational
    Research **60**(3) (1992), 260–272,
    <https://doi.org/10.1016/0377-2217(92)90077-M>.

<span id="ref-dantzig1957"></span>
45. G. B. Dantzig. *Discrete-variable extremum problems.* Operations
    Research **5**(2) (1957), 266–288,
    <https://doi.org/10.1287/opre.5.2.266>.

<span id="ref-kataoka2014"></span>
46. S. Kataoka, T. Yamada. *Upper and lower bounding procedures for the
    multiple knapsack assignment problem.* European Journal of
    Operational Research **237**(2) (2014), 440–447,
    <https://doi.org/10.1016/j.ejor.2014.02.014>.

<span id="ref-martello1990"></span>
47. S. Martello, P. Toth. *Knapsack Problems: Algorithms and Computer
    Implementations.* John Wiley & Sons, Chichester, 1990.

## Satisfiability, learning and support vector machines

<span id="ref-andres2012"></span>
48. B. Andres, B. Kaufmann, O. Matheis, T. Schaub.
    *Unsatisfiability-based optimization in clasp.* Technical
    Communications of the 28th International Conference on Logic
    Programming (ICLP 2012), LIPIcs **17** (2012), 211–221,
    <https://doi.org/10.4230/LIPIcs.ICLP.2012.211>.

<span id="ref-cao2007"></span>
49. Z. Cao, T. Qin, T.-Y. Liu, M.-F. Tsai, H. Li. *Learning to rank:
    from pairwise approach to listwise approach.* Proceedings of the
    24th International Conference on Machine Learning (ICML 2007),
    129–136, <https://doi.org/10.1145/1273496.1273513>.

<span id="ref-keerthi2001"></span>
50. S. S. Keerthi, S. K. Shevade, C. Bhattacharyya, K. R. K. Murthy.
    *Improvements to Platt's SMO algorithm for SVM classifier design.*
    Neural Computation **13**(3) (2001), 637–649,
    <https://doi.org/10.1162/089976601300014493>.

<span id="ref-kurin2020"></span>
51. V. Kurin, S. Godil, S. Whiteson, B. Catanzaro. *Can Q-learning with
    graph networks learn a generalizable branching heuristic for a SAT
    solver?* Advances in Neural Information Processing Systems **33**
    (NeurIPS 2020), 9608–9621.

<span id="ref-morgado2014"></span>
52. A. Morgado, C. Dodaro, J. Marques-Silva. *Core-guided MaxSAT with
    soft cardinality constraints.* Principles and Practice of Constraint
    Programming (CP 2014), Lecture Notes in Computer Science, Springer,
    2014, 564–573, <https://doi.org/10.1007/978-3-319-10428-7_41>.

<span id="ref-platt1998"></span>
53. J. C. Platt. *Sequential minimal optimization: a fast algorithm for
    training support vector machines.* Technical Report MSR-TR-98-14,
    Microsoft Research, 1998.

## Stochastic programming

<span id="ref-pereira1991"></span>
54. M. V. F. Pereira, L. M. V. G. Pinto. *Multi-stage stochastic
    optimization applied to energy planning.* Mathematical Programming
    **52** (1991), 359–375, <https://doi.org/10.1007/BF01582895>.
    Stochastic Dual Dynamic Programming, the archetype application of
    the polyhedral and Benders machinery, named in [Chapter
    1](01-introduction.md#ch-1) and in [Chapter
    13](13-function-family.md#ch-13).

## The programming language

<span id="ref-iso2020"></span>
55. ISO/IEC 14882:2020. *Programming languages: C++.* International
    Organization for Standardization, Geneva, 2020,
    <https://www.iso.org/standard/79358.html>. The rules of invalidation
    of the iterators of `std::vector` and `std::list`
    ([vector.modifiers], [list.modifiers]) cited in
    [§4.2](04-block.md#sec-4-2).

## Serialization

<span id="ref-rew1990"></span>
56. R. Rew, G. Davis. *NetCDF: an interface for scientific data access.*
    IEEE Computer Graphics and Applications **10**(4) (1990), 76–82,
    <https://doi.org/10.1109/38.56302>. The serialization format used
    throughout SMS++ ([Chapter 18](18-factories-netcdf.md#ch-18)).

## Funding acknowledgement

<span id="ref-resilient"></span>
57. RESILIENT, *Resilient Energy System Infrastructure Layouts for
    Industry, E-Fuels and Network Transition*, a project of the Clean
    Energy Transition Partnership (CETPartnership), which is co-funded by
    the European Union under the Horizon Europe research and innovation
    programme, grant agreement no. 101069750
    (<https://resilient-project.github.io>). Much of the recent work on
    the SMS++ `Block` library documented here was carried out in the
    framework of this project, in which SMS++ is interfaced with the
    energy planning tool PyPSA-Eur, as the umbrella project's
    [`README.md`](https://gitlab.com/smspp/smspp-project/-/blob/develop/README.md)
    states among the projects that use SMS++.
