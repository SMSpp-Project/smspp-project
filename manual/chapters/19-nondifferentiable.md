# 19. Nondifferentiable optimization solvers {#ch-19}

[Source: [`SMS++/include/MasterProblemBlock.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/MasterProblemBlock.h);
[`BundleSolver/include/BundleSolver.h`](https://gitlab.com/smspp/bundlesolver/-/blob/develop/include/BundleSolver.h),
[`ParallelBundleSolver.h`](https://gitlab.com/smspp/bundlesolver/-/blob/develop/include/ParallelBundleSolver.h),
[`BundleSolverML.h`](https://gitlab.com/smspp/bundlesolver/-/blob/develop/include/BundleSolverML.h);
[`SubGradSolver/include/SubGradSolver.h`](https://gitlab.com/smspp/subgradsolver/-/blob/develop/include/SubGradSolver.h),
[`Stepsize.h`](https://gitlab.com/smspp/subgradsolver/-/blob/develop/include/Stepsize.h),
[`Deflection.h`](https://gitlab.com/smspp/subgradsolver/-/blob/develop/include/Deflection.h);
[`FrankWolfeSolver/include/FrankWolfeSolver.h`](https://gitlab.com/smspp/frankwolfesolver/-/blob/develop/include/FrankWolfeSolver.h),
[`IntegralityBarrierSolver.h`](https://gitlab.com/smspp/frankwolfesolver/-/blob/develop/include/IntegralityBarrierSolver.h),
[`IntegralityBarrierFunction.h`](https://gitlab.com/smspp/frankwolfesolver/-/blob/develop/include/IntegralityBarrierFunction.h);
`BDMinQuadSolver/include/BDMinQuadSolver.h`; the configurations of
[`tests`](https://gitlab.com/smspp/tests/-/tree/develop) named in
[§19.8](19-nondifferentiable.md#sec-19-8)]

We are concerned in this chapter with the `:CDASolver`s that minimize a
convex nondifferentiable function given as a sum of `C05Function`s
([§13.2](13-function-family.md#sec-13-2)), i.e., with the solvers that
the `LagrangianDualSolver` and the `BendersDecompositionSolver` of
[Chapter 14](14-lag-benders-bfunction.md#ch-14) run on the functions
built by `LagBFunction` and `BendersBFunction`, and with the
Frank-Wolfe method, which works on a similar structure from the primal
side. We first state the problem and the place of these solvers in a
decomposition ([§19.1](19-nondifferentiable.md#sec-19-1)). Then we
describe the master problem of the stabilized methods, which is a
`Block` of the core library ([§19.2](19-nondifferentiable.md#sec-19-2)),
the generalized bundle method of `BundleSolver` with its integer
variant ([§19.3](19-nondifferentiable.md#sec-19-3)), its parallel and
learned variants ([§19.4](19-nondifferentiable.md#sec-19-4)) and the
specialized solver of its master problem
([§19.5](19-nondifferentiable.md#sec-19-5)). Subgradient methods are
the subject of [§19.6](19-nondifferentiable.md#sec-19-6), and
`FrankWolfeSolver`, together with the heuristic built on it, that of
[§19.7](19-nondifferentiable.md#sec-19-7). In
[§19.8](19-nondifferentiable.md#sec-19-8) we collect the
configurations of the tests that use each solver, and
[§19.9](19-nondifferentiable.md#sec-19-9) gives the references. We
state the convergence results without proof, referring to the papers;
only the short facts on which the description relies are proved here.

## 19.1 The problem and the place of the solvers {#sec-19-1}

`BundleSolver` and `SubGradSolver` solve

$$
\min \Big\{\, f(x) = f_0(x) + \sum_{k \in K} f_k(x) \;:\; x \in X
\,\Big\}
\tag{19.1}
$$

where each *component* $f_k$ is a convex `C05Function`, accessed as a "black
box" that, given $x$, returns $f_k(x)$ (possibly only within an accuracy) and
a linearization, i.e., a pair $(g, \alpha)$ with $f_k(y) \ge g y + \alpha$ for
all $y$, while the *0-th component* $f_0$ is a `LinearFunction` or a separable
quadratic `DQuadFunction` (in `BundleSolver`, a quadratic $f_0$ has to be
convex, and it requires the primal MP with the proximal, the trust region or
no stabilization, [§19.2](19-nondifferentiable.md#sec-19-2)). If the
components are all concave, (19.1) is instead a maximization, which both
solvers turn internally into the minimization of the opposite. Either the
`Block` has no sub-`Block` and an `FRealObjective` with a `C05Function`
inside, or its `Objective` holds the 0-th component (possibly empty) and each
component is in a sub-`Block` with no `Variable` or `Constraint`, whose
`FRealObjective` contains it; in both cases the $x$ are the `ColVariable`s of
the `Block`. For `SubGradSolver`, $X$ is a box given by `OneVarConstraint`s,
possibly unbounded on either side. For `BundleSolver`, $X$ is a box $[L, U]$
given by the bounds of the `ColVariable`s and by `OneVarConstraint`s. With the
primal MP, which hosts the `Block`, it may be cut by any `Constraint` that the
`:Solver` of the MP can read, also on other `Variable`s of the `Block`; with
the dual MP, only by `FRowConstraint`s with a `LinearFunction` of the $x$
inside, which it has as multipliers. Furthermore, `BundleSolver` requires that
all the components have the same active `Variable`s, in the same order, at all
times
([`BundleSolver.h`](https://gitlab.com/smspp/bundlesolver/-/blob/develop/include/BundleSolver.h),
general notes of the class), while `SubGradSolver` does not.

This is the shape of the `LagrangianDualBlock` built by a
`LagrangianDualSolver` ([§14.2](14-lag-benders-bfunction.md#sec-14-2)),
whose components are the `LagBFunction`s of the sub-`Block`s and whose
$x$ are the multipliers of the relaxed constraints. It is also the
shape of the master of the convex regime of `BendersDecompositionSolver`
([§14.6](14-lag-benders-bfunction.md#sec-14-6)), whose components are
`BendersBFunction`s. In both cases the solver is the inner `:Solver` of
the decomposition, named by a parameter of its `ComputeConfig` (e.g.,
`str_LDSlv_ISName`), and its parameters are written in the same
`ComputeConfig` as those of the decomposition
([§19.8](19-nondifferentiable.md#sec-19-8)). It returns a point
$\bar x$, the value $f(\bar x)$, a lower bound when one can be
certified, and a convex combination of linearizations of each component,
which the component keeps as its *important* linearization
([§13.2](13-function-family.md#sec-13-2)); for a `LagBFunction` this is
the convexified primal solution, whose meaning is given by
[Theorem 14.1](14-lag-benders-bfunction.md#thm-14-1).

`FrankWolfeSolver` solves a different problem, i.e., the minimization
of a differentiable function over a product of convex hulls
([§19.7](19-nondifferentiable.md#sec-19-7)). It needs no decomposition
`:Solver` around it, since it attaches directly to a `Block` whose
sub-`Block`s hold all the `Variable`s.

## 19.2 The master problem: `MasterProblemBlock` {#sec-19-2}

A stabilized method keeps, for each component $k$ of (19.1), a set
$B^k$ of linearizations (the *bundle*) and the cutting-plane model
$\check f_k(x) = \max \{ g_i x + \alpha_i : i \in B^k \} \le f_k(x)$.
Its *master problem* (MP) chooses the next trial point by minimizing
the model plus a stabilizing term around the current *stability center*
$\bar x$. In SMS++ the MP is the `Block`
[`MasterProblemBlock`](https://smspp.gitlab.io/smspp-project/d0/d34/class_s_m_spp__di__unipi__it_1_1_master_problem_block.html)
of the core library, which any `:Solver` registered to it can solve
(typically a `:MILPSolver` able to handle convex quadratic programs).
Its proximal *primal* form reads

$$
\min \Big\{\, b d + \sum_{k} v^k + \frac{1}{2t} \| d \|_2^2 \;:\;
v^k \ge g^k_i d + \alpha^k_i \;\; i \in B^k ,\;
v^k \ge LB^k \,\Big\}
\tag{19.2}
$$

where $d = x - \bar x$ is the step, $b$ the gradient of a linear 0-th
component, $v^k$ the epigraphic variable of component $k$, $\alpha^k_i$
the value at $\bar x$ of its $i$-th linearization and $LB^k$ a lower
bound on it. Its *dual* form is the Lagrangian dual of (19.2), whose
main variables are the convex multipliers $\theta^k_i$ of the
linearizations
([`MasterProblemBlock.h:10-52`](https://gitlab.com/smspp/smspp/-/blob/develop/include/MasterProblemBlock.h#L10-52)).
Each *hard* component, i.e., one known only through its oracle, is a
`PolyhedralFunctionBlock` sub-`Block` holding $B^k$ in the form of the
MP. An *easy* component has instead a compact convex description (in
`BundleSolver`, it is a `LagBFunction` whose inner `Block` is a linear
program in continuous variables), and its inner `Block` is put in the
dual MP in place of an inner approximation, as in
[Frangioni and Gorgone 2014](Z-bibliography.md#ref-frangioni2014); such
a component is never evaluated, and its inner `Block` needs no
`:Solver`. The `MasterProblemBlock` also holds the variables and the
constraints that couple the components, and the `Objective` with the
stabilizing term, which is one of the values of the enumerator
`MasterProblemBlock::stabilization_type`, as follows.

- `kProximal` gives the term $\frac{1}{2t} \| d \|_2^2$ of (19.2).
- `kLevel` gives the constraint that the model be at most a *level*
  $f_{lev}$, the objective being (a multiple of) the squared distance
  from $\bar x$
  ([Lemaréchal et al. 1995](Z-bibliography.md#ref-lemarechal1995)).
- `kDoublyStabilized` gives both of them
  ([de Oliveira and Solodov 2016](Z-bibliography.md#ref-deoliveira2016)),
  and it is the default of the class.
- `kNone` gives no stabilization, i.e., the cutting-plane method of
  [Kelley 1960](Z-bibliography.md#ref-kelley1960) over the box of the
  variables, which is unbounded as long as the cuts do not bound the
  model there.
- `kTrustRegion` gives the box $\| d \|_\infty \le t$ with a linear
  objective, which $t = +\infty$ removes (hence `kNone` is
  `kTrustRegion` with an infinite $t$).
- `kUpperLower` would give a pair of upper and lower centers; it is a
  reserved enumerator, for which the construction of the MP throws
  `std::logic_error`.

Note that the two forms do not offer the same features: in the primal one, the
MP can host the `Block` of the user, and thus carry its linear `Constraint`s
and its other `Variable`s, and it can keep integer coordinates integer, which
makes the MP a mixed-integer program (a linear one with the trust region or
with no stabilization, when the 0-th component is linear). Only the dual one,
instead, can have easy components. Both can carry a global lower bound on the
function: in the dual one as the coefficient of the multiplier of a
horizontal linearization at the bound, in the primal one as the left-hand
side of the row of the value. Only the primal one carries a separable
quadratic 0-th component: inside the stabilizing term with the proximal
stabilization, and as it is in the `Objective` with the trust region or with
no stabilization (these two since SMS++ 0.9.0); with the dual form, and with
the level or the doubly stabilized one, the construction of the MP throws
`std::logic_error`.

We remark that more than an optimal value is required of the `:Solver`
of the MP
([`MasterProblemBlock.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/MasterProblemBlock.h),
general notes of the class). It has to reoptimize from the base of the
previous call, a few new columns at a time, and its multipliers have to
be accurate, since the method builds out of them the aggregate
linearization and the stopping test; hence an active-set method is
usually appropriate, and an interior point one rather less so, since it
cannot restart and its multipliers are only accurate to about $10^{-7}$
(relative). Also, it has to satisfy the rows of the MP more tightly than
the oracle satisfies its own, for otherwise a point that violates a row by
less than the accuracy of the oracle is answered with the cut that is
already there. Its tolerances must therefore be well below the accuracy
required of the method that drives it.

We now derive what a bundle method reads from the MP out of the duality
of (19.2), which we state for one component with no 0-th
component and no bounds. We write each linearization as
$f(\bar x) + g_i d - \sigma_i$, where $\sigma_i \ge 0$ is its
*linearization error* at $\bar x$.

<span id="prop-19-1"></span>**Proposition 19.1.** *Let $d^*$ be optimal
for $\min \{ v + \frac{1}{2t} \| d \|^2 : v \ge g_i d - \sigma_i ,\;
i \in B \}$ and $\theta^*$ optimal for its dual
$\max \{ - \frac{t}{2} \| \sum_i \theta_i g_i \|^2 - \sum_i \theta_i
\sigma_i : \theta \ge 0 ,\; \sum_i \theta_i = 1 \}$, and let
$z^* = \sum_i \theta^*_i g_i$ and $\sigma^* = \sum_i \theta^*_i
\sigma_i$. Then (i) $d^* = - t z^*$, and the decrease predicted by the
model is $v^* = - t \| z^* \|^2 - \sigma^*$; (ii) $z^*$ is a
$\sigma^*$-subgradient of $f$ at $\bar x$, i.e., $f(x) \ge f(\bar x) -
\sigma^* + z^* (x - \bar x)$ for all $x$; (iii) for every $t^* > 0$
and every $x$,*

$$
f(x) \;\ge\; f(\bar x) - \Big( \frac{t^*}{2} \| z^* \|^2 + \sigma^*
\Big) - \frac{1}{2 t^*} \| x - \bar x \|^2 .
$$

*Proof.* (i) The Lagrangian of the primal problem is
$v + \frac{1}{2t} \| d \|^2 + \sum_i \theta_i ( g_i d - \sigma_i - v )$;
its minimum over $v$ is finite only if $\sum_i \theta_i = 1$, and its
minimum over $d$ is attained at $d = - t \sum_i \theta_i g_i$, which
gives the stated dual. Since the primal problem is a convex quadratic
program with linear constraints and a feasible point (for $v$ large),
there is no duality gap, and $d^* = - t z^*$ for any dual optimal
$\theta^*$. Equating the optimal values gives $v^* + \frac{t}{2} \| z^*
\|^2 = - \frac{t}{2} \| z^* \|^2 - \sigma^*$. (ii) Each linearization is
a lower bound on $f$, i.e., $f(x) \ge f(\bar x) + g_i (x - \bar x) -
\sigma_i$, and the convex combination with weights $\theta^*$ gives the
claim. (iii) From (ii) and $z^* (x - \bar x) \ge - \| z^* \| \, \| x -
\bar x \| \ge - \frac{t^*}{2} \| z^* \|^2 - \frac{1}{2 t^*} \| x - \bar
x \|^2$, by the Cauchy-Schwarz inequality and by $a b \le \frac{t^*}{2}
a^2 + \frac{1}{2 t^*} b^2$. $\square$

By (iii), the quantity $\frac{t^*}{2} \| z^* \|^2 + \sigma^*$ bounds the
decrease obtainable within a distance of about $t^* \| z^* \|$ from
$\bar x$;
this is the meaning of the stopping test of `BundleSolver` and, with
$\| z^* \|$ in place of its square, of `SubGradSolver`. With several
components, $z^*$ and $\sigma^*$ are the sums of those of each one. With
the box and the linear rows of $X$, the dual MP also has their
multipliers, and (ii) holds for $f$ plus the indicator function of $X$,
with a term of the support function of $X$ in $\sigma^*$ (cf. the dual
form in `MasterProblemBlock.h`). Note that when the components are
`LagBFunction`s, $z^*$ is the violation of the relaxed constraints at
the convexified primal solution, and therefore the threshold `dblNZEps`
on its norm is best chosen as the tolerance of feasibility of the
original problem.

## 19.3 `BundleSolver` {#sec-19-3}

[`BundleSolver`](https://smspp.gitlab.io/smspp-project/d7/dd4/class_s_m_spp__di__unipi__it_1_1_bundle_solver.html)
implements the generalized bundle method of [Frangioni
2002](Z-bibliography.md#ref-frangioni2002) on a `MasterProblemBlock`, in the
incremental version with upper models of [van Ackooij and Frangioni
2018](Z-bibliography.md#ref-vanackooij2018) and with the easy components of
[Frangioni and Gorgone 2014](Z-bibliography.md#ref-frangioni2014). Its
parameter `intMPStbl` chooses the stabilization (0 to 4, i.e., `kProximal`,
`kLevel`, `kDoublyStabilized`, `kNone` and `kTrustRegion`, the value 3 with
the primal MP only), `intMPPrimal` the form of the MP, and the file
`strMPBSolverCfg` the `BlockSolverConfig` that gives the MP its `:Solver`;
with easy components (`intDoEasy`) the MP is the dual one. We describe the
method by [Algorithm 19.1](19-nondifferentiable.md#alg-19-1), from which we
omit the vertical linearizations, the start from a point out of $X$ and the
management of the level.

<span id="alg-19-1"></span>**Algorithm 19.1** (generalized proximal
bundle method).

```text
 1  procedure BUNDLE(x̄, t)
 2     evaluate every hard component at x̄; B ← the linearizations found
 3     loop
 4        solve the MP around x̄ with parameter t:
             d*, the predicted decrease v* < 0, z* and σ*
 5        if (t*/2)‖z*‖² + σ* ≤ ε, or σ* ≤ ε and ‖z*‖ ≤ ε_z then
 6           return x̄                                ▷ optimal
 7        if σ* < − ε and σ* ≤ − m₃ (t/2)‖z*‖² then
 8           increase t; continue                     ▷ noise reduction
 9        x ← x̄ + d*
10        repeat                                      ▷ incremental evaluation
11           evaluate one more hard component at x, adding its
                linearizations to B
12        until an upper bound on f(x) is ≤ f(x̄) + (1 − m₂) v*  (SS)
                or a lower bound on f(x) is ≥ the target of m₁   (NS)
13        if SS then x̄ ← x and possibly increase t
14        else possibly decrease t
15        remove from B the linearizations whose multiplier has been 0
             in the last intBPar1 MPs
```

[Algorithm 19.1](19-nondifferentiable.md#alg-19-1) is implemented by
`BundleSolver::compute()`, with the trial point and the inner loop in
`continuous_trial_point()` and the step in `continuous_step()`. At line 5,
$\varepsilon = \min \{$ `dblAbsAcc` $,$ `dblRelAcc` $\cdot \max \{ |f(\bar x)|
, 1 \} \}$, $t^*$ is `dbltStar`, and the norm and the threshold
$\varepsilon_z$ come from `intWZNorm` and `dblNZEps`. A nonpositive `dbltStar`
leaves only the second test. A lower bound comes from $z^*$ being "0" in this
sense, from the model over the box of the variables when this is bounded, from
an empty level set, or from a valid bound of the `Block`. At line 12, the
bounds on $f(x)$ come from the components already evaluated and, for the
others, from their models and (with bit 4 of `intTrgtMng`) from their
Lipschitz constants; a decision may thus be taken before all the components
have been evaluated. In this loop `dblMinNrEvls` sets how many components are
evaluated in any case, and with `dblIncrCost` the others are evaluated anyway
when this is cheap with respect to a MP. For the NS, the target of $m_1$
(`dblm1`) is $f(\bar x) + (1 + m_1) v^*$ when $m_1 \le 0$, as in the shipped
configurations (e.g., $m_1 = -0.30$), and a formula that also involves the
optimal value of the MP when $m_1 > 0$. Note that both a serious step (SS) and
a null step (NS) may be possible when the target of $m_1$ is below that of
$m_2$ (e.g., when $|m_1| < m_2$ with $m_1 \le 0$), and then the SS is
preferred. Line 7 is a noise reduction, meant for oracles that do not deliver
the accuracy they promise, which shows in a "too negative" aggregate error.

*Management of t.* The rules for $t$ come in three layers
(`inttSPar1`, `dbltSPar3`): a long-term strategy, chosen by bits 2 and 3
of `inttSPar1` (the default 12 selects the "balancing" one, which keeps
$\frac{t}{2} \| z^* \|^2$ and $\sigma^*$ of comparable size); the
counters `intMnSSC` and `intMnNSC` of consecutive SS and NS before $t$
may grow or shrink, by factors in given ranges and always within
[`dbltMinor`, `dbltMaior`]; and heuristics that pick the new value out
of the data of the current iteration. In the doubly stabilized mode
each SS sets $t \leftarrow t (1 + \mu)$, with $\mu$ the multiplier of
the level constraint. In the level stabilization the level is
$f(\bar x) - \Delta$, with $\Delta = (1 - m_l)( f(\bar x) - LB )$ when
a lower bound $LB$ is known ($m_l$ = `dblLStabM`) and $\Delta$
exogenous otherwise, an empty level set making the level a valid lower
bound.

*Management of the bundle.* At most `intBPar2` linearizations are kept
per component, between `intBPar4` and `intBPar3` new ones are fetched
from it at each evaluation, and those unused in the last `intBPar1` MPs
are removed. With `dblCmpAggr` in $(0, 1]$, the hard components are
summed into about $1 /$ `dblCmpAggr` groups, each a `C05SumFunction`
([§13.5](13-function-family.md#sec-13-5)), which gives a smaller MP and
a coarser model (i.e., typically cheaper iterations, but more of
them).

<span id="tab-19-1"></span>**Table 19.1.** Main parameters of
`BundleSolver`, with their default values.

| Parameter | Default | Meaning |
|---|---|---|
| `intMPStbl` | 0 | stabilization: 0 proximal, 1 level, 2 doubly stabilized, 3 none, 4 trust region |
| `intMPPrimal` | 0 | 1 for the primal MP, 0 for the dual one |
| `strMPBSolverCfg` | empty | `BlockSolverConfig` of the `:Solver` of the MP |
| `intDoEasy` | 1 | bit-wise: easy components, and which of their duals are kept |
| `dblRelAcc`, `dblAbsAcc` | $10^{-6}$, $+\infty$ | accuracy $\varepsilon$ of the stopping test |
| `dbltStar` | 100 | $t^*$ of the stopping test |
| `dblNZEps`, `intWZNorm` | 0, 2 | when $z^*$ is "0": threshold, norm and scaling |
| `dblm1`, `dblm2`, `dblm3` | 0.01, 0.99, 0.99 | NS, SS and noise reduction tests |
| `dbltInit`, `dbltMinor`, `dbltMaior` | 1, $10^{-6}$, $10^{6}$ | initial, minimum and maximum $t$ |
| `inttSPar1` | 12 | bit-wise choice of the $t$-strategies |
| `intMnSSC`, `intMnNSC` | 0, 3 | consecutive SS / NS before $t$ changes |
| `intBPar1`, `intBPar2` | 10, 100 | age of removal, linearizations per component |
| `intBPar3`, `intBPar4` | 1, 1 | maximum / minimum new linearizations per evaluation |
| `dblMinNrEvls` | 0 | components evaluated before the SS/NS tests |
| `dblCmpAggr` | 0 | aggregation of the hard components |
| `intIntVars` | 0 | 1 keeps the integer `Variable`s integer |

As for convergence, we only state it in the simplest case, i.e., with exact
oracles and the proximal stabilization, in which the SS test of line 12
reads $f(x) \le f(\bar x) + m v^*$ with $m = 1 - m_2 \in (0, 1)$.

<span id="thm-19-1"></span>**Theorem 19.1**
([Frangioni 2002](Z-bibliography.md#ref-frangioni2002)). *Let $f$ in (19.1) be
convex and finite on $X$, let the oracles be exact, and let $t$ stay in
$[t_{min}, t_{max}]$ with $t_{min} > 0$ and never increase during a sequence
of consecutive null steps. Then the values $f(\bar x)$ at the stability
centers converge to $\inf_X f$, and, if this is finite and $\varepsilon > 0$,
the stopping test of [Algorithm 19.1](19-nondifferentiable.md#alg-19-1) is
satisfied after finitely many iterations.*

More general stabilizing terms, the trust region among them, are covered by
the same paper, while the incremental method with inexact oracles is that of
[van Ackooij and Frangioni 2018](Z-bibliography.md#ref-vanackooij2018); the
doubly stabilized and the level methods have separate analyses ([de Oliveira
and Solodov 2016](Z-bibliography.md#ref-deoliveira2016), [Lemaréchal et al.
1995](Z-bibliography.md#ref-lemarechal1995)). It is worth noting that the
parameters of `BundleSolver` allow choices outside these hypotheses (e.g.,
`dbltSPar3` lets the heuristics increase $t$ at a null step), for which the
theorem gives no guarantee.

*Integer variables.* With `intIntVars` 1 the integer `ColVariable`s
are kept integer, and what is minimized is
$F(x) = f_0(x) + \sum_k f_k(x)$ over $X = [L, U]$ with $x_j \in
\mathbb{Z}$ for $j \in J$. Then the MP must be primal, in the iterate
form (`intMPV2Form` 1), and either proximal or level (a mixed-integer
quadratic program) or with the trust region or no stabilization (a
mixed-integer linear one). Its `:Solver` must keep the integrality,
with a tolerance of 0 (or as small as it allows) and a relative gap
tighter than `dblRelAcc`. We are then in the stabilized cutting-plane
method of [van Ackooij et al. 2016](Z-bibliography.md#ref-vanackooij2016),
with the stabilization relaxed by degrees as in
[Baena et al. 2020](Z-bibliography.md#ref-baena2020)
([`BundleSolver.h:2959-3050`](https://gitlab.com/smspp/bundlesolver/-/blob/develop/include/BundleSolver.h#L2959-3050)).

<span id="alg-19-2"></span>**Algorithm 19.2** (stabilized
cutting-plane method with integer variables).

```text
 1  procedure INTEGER-BUNDLE(x̄, t)
 2     x̄ ← a point of X, integer in J, with F(x̄) < ∞; stab ← true
 3     loop
 4        if stab then
 5           xᵖ ← argmin { F̌(x) + ‖x − x̄‖²/(2t) : x ∈ X, x_J ∈ ℤ^J }
 6           v* ← F̌(xᵖ) − F(x̄)
 7           if v* < − ε then x ← xᵖ
 8           else if t < t_max then
 9              t ← min{ t · m_incr , t_max }; continue
10           else stab ← false
11        if not stab then
12           xᶜ ← argmin { F̌(x) : x ∈ X, x_J ∈ ℤ^J };  ℓ ← F̌(xᶜ)
13           if F(x̄) − ℓ ≤ ε then return x̄            ▷ ε-optimal
14           x ← xᶜ
15        evaluate every hard component at x, adding the
             linearizations to the bundle
16        if F(x) < F(x̄) then x̄ ← x; stab ← true
```

In `BundleSolver`, [Algorithm 19.2](19-nondifferentiable.md#alg-19-2) is
implemented by `integer_direction()`, `integer_trial_point()` and
`integer_step()`. With the trust region, line 5 has the box $\| x - \bar x
\|_\infty \le t$ in place of the proximal term, and lines 8-9 enlarge the
region by the fraction $p$ = `dblIntRad` of the whole one (the binary
variables by a local branching constraint, a variable with no finite bound by
$t$, multiplied by `dblmxIncr` at each enlargement), so that after $\lceil 1 /
p \rceil$ enlargements it is the whole one. With only binary variables, a
region in which the center is optimal is then excluded by the reverse local
branching constraint, and a stabilized MP that these constraints make empty is
taken as a local optimum. With the proximal term, $t_{max}$ is `dbltMaior` and
$m_{incr}$ is `dblmxIncr`. At line 12, $\ell$ is the value of the MP less the
gap of its `:Solver`; if that MP is unbounded, $t$ is increased tenfold and
the stabilization is kept from then on. All the parts of [Algorithm
19.1](19-nondifferentiable.md#alg-19-1) that need the multipliers of a
continuous MP (the noise reduction, the rules for $t$, the removal of
linearizations) are skipped, and $t$ never decreases. With the level
stabilization, a level variant of the same paper is used instead, which also
solves the cutting-plane MP whenever a level set is empty.

<span id="prop-19-2"></span>**Proposition 19.2.** *Let all the
variables be integer, $X$ bounded, the oracles exact and $F$ finite on $X$,
$t_{max} < \infty$ and $\varepsilon \ge 0$, and let each MP be solved exactly.
Then [Algorithm 19.2](19-nondifferentiable.md#alg-19-2) stops after finitely
many iterations.*

*Proof.* Since the bundle is never cleaned, after a point $x$ has been
evaluated the bundle has the linearizations of all the components at
$x$, hence $\check F(x) = F(x)$; also, $\bar x$ is always the best
point evaluated so far. A point $x^p$ with $v^* < - \varepsilon$ has
not been evaluated, for otherwise $F(x^p) = \check F(x^p) < F(\bar x)$.
Neither has a point $x^c$ at which the method does not stop, for
otherwise $F(\bar x) \le F(x^c) = \check F(x^c) = \ell$ and the test of
line 13 holds. Hence each iteration that reaches line 15 evaluates a
new point, and there are at most $| X \cap \mathbb{Z}^n |$ such
iterations. Between two of them, line 9 is executed finitely many
times, since $t$ grows by the factor $m_{incr} > 1$ up to $t_{max}$ and
never decreases. Similarly, with the trust region the region grows at
most $\lceil 1 / p \rceil$ times between two moves of $\bar x$, which
are finitely many. $\square$

With continuous variables as well, the level variant may reach
`intMaxIter` with the gap still open, since the cutting-plane model need
not become exact in finitely many steps.

## 19.4 `ParallelBundleSolver` and `BundleSolverML` {#sec-19-4}

[`ParallelBundleSolver`](https://smspp.gitlab.io/smspp-project/d8/df9/class_s_m_spp__di__unipi__it_1_1_parallel_bundle_solver.html)
runs the same method, with the components evaluated concurrently on at most
`intMaxThread` threads (a parameter that `BundleSolver` ignores). Its
parameter `intParFrm` chooses whether the results are consumed in the order in
which they complete (0, the default, which is not reproducible) or in a fixed
order (1), possibly processing exactly the components that the sequential
method would (3) or all of them at each iteration (5); `dblPoolingInt` is
accepted but has no effect. The threads, with an example, are described in
[§17.4](17-parallel.md#sec-17-4); with a single component, the parallel
solver gains nothing, at a cost only slightly higher than that of the
sequential one.

[`BundleSolverML`](https://smspp.gitlab.io/smspp-project/d3/d38/class_s_m_spp__di__unipi__it_1_1_bundle_solver_m_l.html)
replaces the heuristics of the bottom layer of the management of $t$ with a
neural network, written with the C++ interface of Torch, that predicts $t$ out
of 20 features of the state of the method. The multipliers $\theta$, and hence
the direction, still come from the MP. This is the part of the method of
[Demelas et al. 2025](Z-bibliography.md#ref-demelas2025) that predicts the
step, and the class is in the library `SMS++::BundleSolverML`, built only when
Torch is found. Its defaults let the network drive $t$ (`inttSPar1` 3,
`intMnSSC` and `intMnNSC` 0, `dblmnIncr` and `dblmnDecr` about 1). Among the
parameters it adds, `intMLModel` gives the core of the network (0, the
default, a feed-forward one, then RNN, GRU and LSTM). With `intMLFeatures` 1,
the default, the features are dimensionless and the output is the factor by
which $t$ changes, so that one network may serve instances of somewhat
different scale. `intMLLoss` chooses between a loss based on the objective (0)
and one based on the outcome of each step (1), `intMLTrainOnline` 1 trains the
network at the end of each `compute()`, and `dblMLTPenalty` (since SMS++
0.9.0) weighs a term $\lambda t^2 / 2$ that keeps the loss 0 from pushing $t$
up without end. One network can be shared among several solvers, one per
instance, to train it over many instances. Torch is given $\max \{ 1 ,$
`intMaxThread` $\}$ threads, and hence the default run is reproducible; with
more threads, the order of the sums of the network, and with it the
trajectory, may change from a run to the next. The default run does not depend
on the initial weights: with `intMLFeatures` 1 the last layer starts null, and
$t$ is left as it is until the network is trained. Otherwise, `intMLSeed` $\ge
0$ seeds Torch and makes them reproducible, while the default $-1$ leaves its
generator untouched.

## 19.5 `BDMinQuadSolver` {#sec-19-5}

`BDMinQuadSolver` is a `:CDASolver` for the primal MP of `BundleSolver`, i.e.,
a `:Solver` of a `MasterProblemBlock`, that solves it with the active-set
method for the disaggregate bundle master of [Frangioni
1996](Z-bibliography.md#ref-frangioni1996). It reads from the
`MasterProblemBlock` the rows of each hard component (also the vertical ones),
the center, $t$, the box, the lower bounds of the components and the global
one, and the level, and it writes back the solution and the multipliers of the
rows of the cuts and of the level. Its state is kept from one `compute()` to
the next, and it only receives the rows that have been added, removed or
changed, which lets it restart from the previous base, as
[§19.2](19-nondifferentiable.md#sec-19-2) requires of the `:Solver` of the MP.
It handles the proximal and the doubly stabilized MP with a linear 0-th
component. It throws `std::invalid_argument` on the dual MP, easy components,
linear constraints, a hosted `Block`, integer coordinates, a quadratic 0-th
component, the scaling of the components, a concave component, a hard
component that is not a `PolyhedralFunctionBlock`, a center out of the box,
and every stabilization other than the proximal and the doubly stabilized one
(i.e., the level alone, the trust region and none). Its tolerances are
`dblRelAcc`, on the relative duality gap, and `dblFAccSol`, on the feasibility
of the multipliers (since SMS++ 0.9.0), a value out of $(0, \infty)$ giving
the built-in $10^{-6}$. Also since SMS++ 0.9.0, `intBDMQQMode` chooses how the
entries of the matrix $Q$ of the method are computed (eagerly, a whole row
when an item enters, or lazily, the default), and `intBDMQPricing` whether the
reduced costs are computed by $Q$, by $z$ or, by default, by a choice made at
each pass. Finally, `strBDMQCosts` names a cost table of its kernels written
by `bdmq_calibrate` (empty for the built-in one). We remark that the module is
private: its submodule is not checked out by default (`update = none`), and
`BUILD_BDMinQuadSolver` stays off until it is.

## 19.6 `SubGradSolver` {#sec-19-6}

[`SubGradSolver`](https://smspp.gitlab.io/smspp-project/d8/da1/class_s_m_spp__di__unipi__it_1_1_sub_grad_solver.html)
solves (19.1), with $X = \Lambda$ a box (we write $\lambda$ for $x$, as is
customary for Lagrangian multipliers), by the unified subgradient method of
[Frangioni et al. 2017](Z-bibliography.md#ref-frangioni2017), whose iterate is

$$
\breve\lambda_{i+1} = \bar\lambda_i - \nu_i d_i , \qquad
\lambda_{i+1} = \mathrm{P}_\Lambda( \breve\lambda_{i+1} ) , \qquad
d_i = \alpha_i g_i + (1 - \alpha_i) d_{i-1}
\tag{19.3}
$$

([`SubGradSolver.h:106-118`](https://gitlab.com/smspp/subgradsolver/-/blob/develop/include/SubGradSolver.h#L106-118)).
Here $\mathrm{P}_\Lambda$ is the projection on $\Lambda$, $\bar\lambda_i$ the
stability center, $g_i$ the sum of one linearization of each component at
$\lambda_i$ (the gradient for the 0-th one), $\nu_i$ the *stepsize* and
$\alpha_i \in (0, 1]$ the *deflection coefficient*. Both are given by objects
of the abstract classes `Stepsize` and `Deflection`, each with a factory in
which the concrete rules register themselves, and they are selected by name
with `strStepsize` and `strDeflection`. Since the parameters of the rules
belong to the `SubGradSolver`, they are set in its `ComputeConfig`, even
before the rule is built. [Table 19.2](19-nondifferentiable.md#tab-19-2) lists
the rules registered by the module.

<span id="tab-19-2"></span>**Table 19.2.** Rules of `SubGradSolver`, by
the name under which they are registered.

| Name | Factory | Rule | Source |
|---|---|---|---|
| `Polyak` | `Stepsize` | constant $\beta$, level at the best lower bound | [Polyak 1969](Z-bibliography.md#ref-polyak1969) |
| `ColorTV` | `Stepsize` | $\beta$ changed by green, yellow and red iterations (default) | [Barahona and Anbil 2000](Z-bibliography.md#ref-barahona2000) |
| `FumeroTV` | `Stepsize` | level moving from the lower bound to the best value | [Fumero 2001](Z-bibliography.md#ref-fumero2001) |
| `Volume` | `Deflection` | the modified Volume algorithm (default) | [Bahiense et al. 2002](Z-bibliography.md#ref-bahiense2002) |
| `PrimalDual` | both | simple or weighted averages | [Nesterov 2009](Z-bibliography.md#ref-nesterov2009) |

`Polyak` and `FumeroTV` need a finite lower bound on $f$, given by the `Block`
or by the components, and throw without one, while `ColorTV` does not. We
remark that the first three rules of the table are *target value* ones, i.e.,
$\nu_i = \beta_i ( f(\bar\lambda_i) - f^{lev}_i ) / \| d_i \|^2$ with $\beta_i
\in (0, 2)$, and they differ in the choice of $\beta_i$ and of the level
$f^{lev}_i$. `PrimalDual` chooses the stepsize and the deflection together,
and it plays both roles when it is named in `strStepsize`. With
`strDeflection` empty or `none` there is no deflection, i.e., $\alpha_i = 1$,
and each step is serious; otherwise the `Deflection` decides whether
$\bar\lambda$ moves (`do_SS()`). We can now state the method.

<span id="alg-19-3"></span>**Algorithm 19.3** (deflected projected
subgradient method).

```text
 1  procedure SUBGRADIENT(λ̄)
 2     λ ← λ̄;  d ← 0
 3     loop
 4        evaluate every component at λ;  g ← the sum of their subgradients
 5        if the Deflection declares a SS (always without one) then λ̄ ← λ
 6        α ← the deflection coefficient (1 without a Deflection)
 7        d ← α g + (1 − α) d, the vectors projected as intProjection says
 8        if t* ‖d‖ + ε_d ≤ ε · max{ 1 , |f_rec| } then return
 9        ν ← the stepsize
10        λ ← P_Λ( λ̄ − ν d )
```

[Algorithm 19.3](19-nondifferentiable.md#alg-19-3) is implemented by
`SubGradSolver::compute()`. At line 7, `intProjection` selects which of $g_i$,
$d_{i-1}$ and $d_i$ are projected on the tangent cone of $\Lambda$ at
$\bar\lambda$, and `intScheme` whether the *safe rule* is used and whether the
scheme is *stepsize-restricted* (lines 6-7 before line 9, the stepsize being
bounded by the deflection coefficient) or *deflection-restricted* (the
stepsize first, bounding the coefficient). At line 8, $\varepsilon_d$ is the
linearization error of $d_i$ at $\bar\lambda_i$, $\| d_i \|$ the norm of its
projection on the tangent cone, $f_{rec}$ the best value found, $t^*$ is
`dbltStar` and $\varepsilon$ comes from `dblRelAcc` and `dblAbsAcc`; with
`intKeepHat` 1, the default, and out of the incremental variant,
$\varepsilon_d$ is the smaller of that error and of the one at the point
$\hat\lambda_{i+1} = \alpha_{i+1} \lambda_i + (1 - \alpha_{i+1})
\hat\lambda_i$. As for `BundleSolver`, the test means that no step of length
$t^*$ decreases $f$ by more than the accuracy, but it gives no lower bound. In
the *incremental* variant (`dblIncrFactor` $> 0$), each iteration is followed
by $\lceil$ `dblIncrFactor` $\cdot n \rceil$ inner iterations along the
subgradient of one component, in an order shuffled with the seed `intSeed` and
with the stepsize scaled by the Lipschitz constants (`dblLipFactor`), where
$n$ is the number of components ($|K|$, plus 1 when there is a 0-th one).
This variant cannot be deflected. When the stepsize is at most
$10^{-8}\,t^*$ for more than 100 consecutive iterations ($100\,n$ in the
incremental variant), `compute()` returns `kLowPrecision`.

As an upper bound one has the best value found, while a lower bound is
only available from the `Block` (`get_valid_lower_bound()`) or from the
direction, which is a convex combination of linearizations.

<span id="lem-19-1"></span>**Lemma 19.1.** *Let $d$ be a convex
combination of linearizations of $f$, i.e., $f(\lambda) \ge
L(\lambda) = f(\bar\lambda) - \varepsilon_d + d (\lambda -
\bar\lambda)$ for all $\lambda$. Then (i) $\min \{ L(\lambda) :
\lambda \in \Lambda \} \le \inf_\Lambda f$; (ii) if the projection of
$-d$ on the tangent cone of $\Lambda$ at $\bar\lambda \in \Lambda$ is
0, this minimum is $f(\bar\lambda) - \varepsilon_d$.*

*Proof.* (i) Since $L \le f$ everywhere, the minimum of $L$ over
$\Lambda$ is at most that of $f$. (ii) If the projection of $-d$ on
the tangent cone $T$ is 0, then $-d$ is in the polar cone of $T$, i.e.,
in the normal cone of $\Lambda$ at $\bar\lambda$, and therefore $-d
(\lambda - \bar\lambda) \le 0$ for all $\lambda \in \Lambda$; hence
$L(\lambda) \ge L(\bar\lambda) = f(\bar\lambda) - \varepsilon_d$ on
$\Lambda$. $\square$

On a box the minimum in (i) is computed coordinate by coordinate, and it
is finite when the bounds are; with unbounded coordinates it is finite
only by chance, and the lower bound of a subgradient method therefore
usually stays $-\infty$. Case (ii) is the one used with `dblNZEps`,
below which the projected direction is declared "0"; the default 0
requires an exact 0, while a larger value may give an invalid bound.

*Primal recovery.* For each component with a global pool,
`SubGradSolver` keeps there the latest linearization and a convex
combination of the past ones, which `get_dual_solution()` makes the
important linearization of the component. With `intPrimalRec` 0 the
combination is that of the direction, whose weights are the deflection
coefficients (as in the Volume and Primal-Dual methods); with 1, and
always without a `Deflection`, it is the ergodic average, whose weights
are the stepsizes. For `LagBFunction` components this is a convex
combination of the solutions of the Lagrangian subproblems, by which
`LagrangianDualSolver` recovers its primal solution when it uses this
inner `:Solver`.

<span id="thm-19-2"></span>**Theorem 19.2.** *Let $f$ be convex with a
nonempty set of minima over the closed convex set $\Lambda$, of value
$f^*$. (i) ([Polyak 1969](Z-bibliography.md#ref-polyak1969)) Without
deflection, with $f^{lev}_i = f^*$ and $\beta_i \in [\beta_{min},
\beta_{max}] \subset (0, 2)$, $f(\lambda_i) \to f^*$. (ii)
([Larsson et al. 1999](Z-bibliography.md#ref-larsson1999)) If $f$ is
the Lagrangian function of a convex program over a compact set, and the
method runs without deflection with stepsizes $\nu_i > 0$, $\sum_i \nu_i
= \infty$ and $\sum_i \nu_i^2 < \infty$, the averages of the solutions
of the Lagrangian subproblems weighted by the stepsizes have
all their accumulation points in the set of the optimal solutions of
the convex program.*

Of course, the target value rules do not know $f^*$, and their
stepsizes need not satisfy the conditions of (ii); the convergence of
the deflected and of the incremental methods, under suitable choices of
the stepsize and of the deflection, is studied in
[Frangioni et al. 2017](Z-bibliography.md#ref-frangioni2017). For a
`LagBFunction` over a set $X$, the convex program of (ii) is the
convexified relaxation of
[Theorem 14.1](14-lag-benders-bfunction.md#thm-14-1).

## 19.7 `FrankWolfeSolver` {#sec-19-7}

[`FrankWolfeSolver`](https://smspp.gitlab.io/smspp-project/d0/dfd/class_s_m_spp__di__unipi__it_1_1_frank_wolfe_solver.html)
attaches to a "father" `Block` with no `Variable` or `Constraint`
and an `FRealObjective` with a `C05Function` $f$ inside, of
which only the diagonal linearization (i.e., the gradient) is used. Its
sub-`Block`s hold all the `Variable`s and `Constraint`s,
each with an `FRealObjective` whose `Function` is a `LinearFunction`, a
`DQuadFunction` or a `QuadFunction`. We write $X_j$ for the feasible
region of sub-`Block` $j$ as its `:Solver` sees it (integer, if that
`:Solver` keeps the integrality) and $h_j$ for the part of its
objective that is kept in its oracle, so that the problem is

$$
\min \Big\{\, F(x) = f(x) + \sum_j h_j(x_j) \;:\; x \in X =
\prod_j \mathrm{conv}(X_j) \,\Big\}
\tag{19.4}
$$

where $h_j(y) = a\, c_j y + b\, q_j(y)$, $c_j$ and $q_j$ being the
linear and the quadratic part of the objective of the sub-`Block`, and
$(a, b)$ is set by `intLMOObj` to $(0, 0)$ (`LMOLinear`, the default),
$(0, 1)$ (`LMOQuad`) or $(1, 1)$ (`LMOFull`). For each sub-`Block`, the
`:Solver` registered at position `intLMOSlvr` is its *Linear
Minimization Oracle* (LMO): the gradient of $f$ is written in the
`Objective` of the sub-`Block`, and the `:Solver` minimizes it, plus
$h_j$, over $X_j$.

<span id="alg-19-4"></span>**Algorithm 19.4** (Frank-Wolfe method over
a product of convex hulls).

```text
 1  procedure FRANK-WOLFE(x)
 2     w ← the vertices the LMOs give at ∇f(x);  x ← w      ▷ eInitLMO
 3     for k ← 0, 1, 2, … do
 4        g ← ∇f(x)
 5        for each sub-Block j do                            ▷ in parallel
 6           write g_j in the Objective of sub-Block j
 7           w_j ← argmin { g_j y + h_j(y) : y ∈ X_j }      ▷ its LMO
 8        U ← g x + ∑_j h_j(x_j) − g w − ∑_j h_j(w_j)       ▷ the gap
 9        if U ≤ ε · max{ 1 , |F(x)| } then
10           restore the Objectives of the sub-Blocks; return x
11        γ ← the step of the line search in [0, 1]
12        x ← x + γ (w − x)
```

[Algorithm 19.4](19-nondifferentiable.md#alg-19-4) is implemented by
`FrankWolfeSolver::compute()`. At line 5 the LMOs run on at most
`intMaxThread` threads, each `:Solver` with the identity that
`FrankWolfeSolver` lends it ([§17.1](17-parallel.md#sec-17-1)); the gradient
is divided by the weight of the `Objective` of the sub-`Block`, which its
`:Solver` does not see. Under (P2) below, the terms $h_j(x_j)$ of line 8 are
replaced by the convex combination of the costs of the vertices that make up
$x$. At line 9, $\varepsilon$ is `dblRelAcc`, and the gap is also compared
with `dblAbsAcc` if this is finite. At line 11, `intLineSearch` chooses the
exact line search when the objective is quadratic along the step (`LSAuto`,
the default, or `LSExact`). The alternatives are the open-loop
rule $\gamma = 2 /
(k + 2)$ (`LSAgnostic`, and `LSAuto` otherwise) and the fixed step
`dblFWStep` (`LSFixed`). `intAlgorithm` selects the vanilla method of
[Algorithm 19.4](19-nondifferentiable.md#alg-19-4) (0), the away-step one
(1, [Guélat and
Marcotte 1986](Z-bibliography.md#ref-guelat1986)) or the blended pairwise
conditional gradient (2, [Tsuji et al.
2022](Z-bibliography.md#ref-tsuji2022)), the latter two keeping an active set
of at most `intMaxAtoms` vertices. In the simplest case, the gap $U$ bounds
the distance from the optimum.

<span id="lem-19-2"></span>**Lemma 19.2.** *Let $f$ be convex and
differentiable on $X$ and each $h_j$ affine. Then, for every $x \in X$, $F(x)
- \inf_X F \le U$, with $U$ as at line 8 of [Algorithm
19.4](19-nondifferentiable.md#alg-19-4).*

*Proof.* Let $M(y) = \nabla f(x) y + \sum_j h_j(y_j)$, which is affine;
its minimum over $\prod_j X_j$ is attained at $w$, and it is also its
infimum over $X$, since an affine function has the same infimum over a
set and over its convex hull. For every $y \in X$, convexity gives
$F(y) \ge f(x) + \nabla f(x)(y - x) + \sum_j h_j(y_j) = F(x) + M(y) -
M(x) \ge F(x) - ( M(x) - M(w) ) = F(x) - U$. $\square$

<span id="thm-19-3"></span>**Theorem 19.3**
([Frank and Wolfe 1956](Z-bibliography.md#ref-frank1956),
[Jaggi 2013](Z-bibliography.md#ref-jaggi2013)). *If $F$ is convex with
an $L$-Lipschitz gradient on $X$, and $D$ is the diameter of $X$, the
iterates of the vanilla method with each $h_j$ affine, and with the
open-loop step or with the exact line search, satisfy
$F(x^k) - \min_X F \le 2 L D^2 / (k + 2)$.*

*Two problems.* When some $h_j$ is not affine, the parameter
`intCvxComb` chooses between (19.4), called (P1) (`eObjAtX`), and the
problem (P2) (`eObjCvxComb`, the default), in which the cost of a
sub-`Block` is the convex combination of the costs of the points given
by the LMO, with the weights by which the iterate is built out of them:

$$
\inf \Big\{\, f\Big( \sum_k \lambda_k v^k \Big) + \sum_j \sum_k
\lambda_k h_j(v^k_j) \;:\; v^k \in \prod_j X_j ,\; \lambda \ge 0 ,\;
\sum_k \lambda_k = 1 \,\Big\}
\tag{19.5}
$$

over all the finite convex combinations. We stress that in the two
modes the oracle is the same, while the value, the gap and the line
search change, and so does the trajectory. To compare the two values, let
$\check h_j(y) = \inf \{ \sum_i \mu_i h_j(y^i) : y^i \in X_j ,\; \mu
\ge 0 ,\; \sum_i \mu_i = 1 ,\; \sum_i \mu_i y^i = y \}$ for $y \in
\mathrm{conv}(X_j)$, i.e., the convex envelope of $h_j$ restricted to
$X_j$.

<span id="prop-19-3"></span>**Proposition 19.3.** *(i) The value of
(19.5) is $\inf \{ f(x) + \sum_j \check h_j(x_j) : x \in X \}$. (ii) If
each $h_j$ is convex on $\mathrm{conv}(X_j)$, then $\check h_j \ge h_j$
there, and the value of (P2) is at least that of (P1). (iii) If each
$h_j$ is affine, the two values coincide; the converse does not hold,
since the values also coincide, for any convex $h_j$, when each $X_j$
is convex.*

*Proof.* (i) Given $\lambda$ and the $v^k$ of (19.5), let $x =
\sum_k \lambda_k v^k$; for each $j$ the pairs $(\lambda_k, v^k_j)$ form
a convex combination of points of $X_j$ whose sum is $x_j$, hence
$\sum_k \lambda_k h_j(v^k_j) \ge \check h_j(x_j)$, and the objective of
(19.5) is at least $f(x) + \sum_j \check h_j(x_j)$. Conversely, take
$x \in X$ and, for each $j$, a convex combination $(\mu^j_i, y^j_i)$ of
points of $X_j$ with sum $x_j$. Then the product weights $\lambda =
\prod_j \mu^j_{i_j}$ on the points $(y^1_{i_1}, y^2_{i_2}, \ldots) \in
\prod_j X_j$ form a finite convex combination whose marginal on each
$j$ is $\mu^j$; hence its sum is $x$, and its cost for sub-`Block` $j$
is $\sum_i \mu^j_i h_j(y^j_i)$. Taking the infimum over the
combinations gives the converse inequality. (ii) By the convexity of
$h_j$, $h_j(\sum_i \mu_i y^i) \le \sum_i \mu_i h_j(y^i)$ for each
combination in the definition of $\check h_j$; thus $h_j \le \check
h_j$ on $\mathrm{conv}(X_j)$, and the objective of (P1) is at most that
of (i) at each $x \in X$. (iii) If $h_j$ is affine, each combination
in the definition of $\check h_j(y)$ costs $h_j(y)$, hence $\check h_j
= h_j$. If $X_j$ is convex, the trivial combination $\mu = 1$, $y^1 =
y$ gives $\check h_j(y) \le h_j(y)$, and with (ii) $\check h_j = h_j$.
$\square$

Hence (P2) is the stronger of the two relaxations, i.e., the bound of
the Dantzig-Wolfe reformulation of the sub-`Block`s with their true
costs, whose columns are generated by the LMO. At present the exact
line search of (P1) is available only when the $h_j$ are linear, and
otherwise (P1) falls back to the open-loop rule
([`FrankWolfeSolver.h`](https://gitlab.com/smspp/frankwolfesolver/-/blob/develop/include/FrankWolfeSolver.h),
`intCvxComb`).

*Directions.* With `intFWDirection` 0 (the default) the LMO is given the
gradient, as above. The other values give it the aggregated subgradient of a
small stabilized master problem (for the value 1, the master of [Proposition
19.1](19-nondifferentiable.md#prop-19-1) with $t$ = `dblFWt`) built on
gradients that the method already has, and hence at no cost of evaluation:
those of the current and of the previous iterate (1), a bundle of them in a
`MasterProblemBlock` (2), the current one and the aggregate of the older ones
(3), or a primal-dual variant whose master minimizes a bound on the gap (4).
Such a direction is accepted only if the decrease that the gradient promises
towards its vertex is at least `dblFWAccept` times its gap, and otherwise
`intFWOnReject` rules what happens. The direction 2 needs `strFWMPBCfg`, the
`BlockSolverConfig` of the `:Solver` of its `MasterProblemBlock` (`compute()`
throws otherwise), and keeps `intFWBundleSize` pieces (10 by default).
Finally, `intInitPoint` chooses where the iterates start from: the vertex
given by the LMOs at the current point (`eInitLMO`, the default, line 2 of
[Algorithm 19.4](19-nondifferentiable.md#alg-19-4)) or the current values of
the `Variable`s (`eInitBlock`, with the vanilla method only). Also,
`intFWBestLB` 1 uses in the stopping test (and in that of `dblFWAccept`) the
best bound found so far in the `compute()` in place of that of the iteration.

*`IntegralityBarrierSolver`.* The class
[`IntegralityBarrierSolver`](https://smspp.gitlab.io/smspp-project/db/d9c/class_s_m_spp__di__unipi__it_1_1_integrality_barrier_solver.html)
is a plain `:Solver` that looks for a feasible solution of a mixed-integer
linear program held in a `Block` (e.g., an `AbstractBlock` read from an MPS
file). It minimizes over the polyhedron of the continuous relaxation an
`IntegralityBarrierFunction`, i.e., a `C05Function` that sums, over the
integer variables, a smooth penalty of period 1, smallest (about
$e^{-1/\varepsilon}$) at the integer values and largest at the half-integers,
divided by a product of powers of the normalized slacks of the rows; its
global minima are the integer points. Since the function is not convex, the
method is a heuristic: a `FrankWolfeSolver` (configured by the file
`strFWCfg`) runs on an abstract copy of the `Block`, with the `:Solver` of the
linear relaxation (registered by `strLMOBSCfg`) as its oracle, and stops as
soon as rounding gives a feasible point. Otherwise, the exponents of the
slacks, which start at 0, are changed by the rule of `dblYLevel`, the starting
point is moved towards the last iterate by `dblX0Step`, and the run is
repeated, at most `intMaxIter` times. With `intInitSlvr` $\ge 0$ the
`Variable`s are first set to the solution of the `:Solver` registered to the
`Block` at that position (e.g., a barrier method without crossover, which
gives an interior point).

## 19.8 Configurations shipped with the tests {#sec-19-8}

Each solver is selected and configured by files, as in
[Chapter 11](11-configuration.md#ch-11), and the suites of
[`tests`](https://gitlab.com/smspp/tests/-/tree/develop) have at least
one for each of them; we quote the lines that matter.

*`BundleSolver` and `BundleSolverML`.* In the suite of `MMCFBlock` the
`ComputeConfig` of the `LagrangianDualSolver`
([`MMCFBlock/LDCfg.txt`](https://gitlab.com/smspp/tests/-/blob/develop/MMCFBlock/LDCfg.txt))
carries the parameters of the decomposition and of the inner
`BundleSolver` together, the MP being solved by the `GRBMILPSolver` of
`MPBCfg.txt`:

```text
dbltStar          1e+3
dblm1            -0.30
str_LDSlv_ISName BundleSolver         # the inner Solver used by
                                      # LagrangianDualSolver
strMPBSolverCfg  MPBCfg.txt
```

The differential `ComputeConfig`
[`MMCFBlock/LDCfg-ML-K.txt`](https://gitlab.com/smspp/tests/-/blob/develop/MMCFBlock/LDCfg-ML-K.txt),
an override of the same file, names `BundleSolverML` instead, with the
parameters of $t$ of [§19.4](19-nondifferentiable.md#sec-19-4) and
`intMLSeed` 0, while [`MMCFBlock/BSPar-ML.txt`](https://gitlab.com/smspp/tests/-/blob/develop/MMCFBlock/BSPar-ML.txt)
also trains the network online. [Recipe R4](R4-cfl-lagrangian.md#rec-R4)
develops a complete example with `BundleSolver` on CFL.

*`BDMinQuadSolver`.* In the core suite, the `BlockSolverConfig`
[`SMS++/PolyhedralFunctionBlock/MPBCfg-BDMQ.txt`](https://gitlab.com/smspp/tests/-/blob/develop/SMS++/PolyhedralFunctionBlock/MPBCfg-BDMQ.txt)
registers it with its default parameters, and the list `MPBSolvers.txt`
of the suite `SMS++/MasterProblemBlock` has it beside a `GRBMILPSolver`
and a `DAQPMILPSolver`, which solve the same master problems.

*`SubGradSolver`.* The suites of `MultiKnapsackAssignBlock` and of
`GeneralizedAssignmentBlock` (since SMS++ 0.9.0) relax the constraints
that link the `BinaryKnapsackBlock` sub-`Block`s with a
`LagrangianDualSolver` whose inner `:Solver` is a `SubGradSolver` with
the rules of the Volume algorithm
([`MultiKnapsackAssignBlock/LDCfg-SG.txt`](https://gitlab.com/smspp/tests/-/blob/develop/MultiKnapsackAssignBlock/LDCfg-SG.txt)):

```text
intProjection          1 # the subgradient is projected on the tangent cone
                         # of the box of the multipliers
str_LDSlv_ISName   SubGradSolver  # the inner Solver of LagrangianDualSolver
strStepsize        ColorTV        # the stepsize rule
strDeflection      Volume         # the deflection rule
```

The file of `GeneralizedAssignmentBlock` only differs in
`int_LDSlv_NNMult 0`, since its relaxed constraints are equalities.

*`FrankWolfeSolver` and `IntegralityBarrierSolver`.* The suites of
`BinaryKnapsackBlock`, `MCFBlock` and `UCBlock` build a father `AbstractBlock`
over $K$ copies of a leaf `Block`, with a random father `Objective` (by
default a separable quadratic one), and solve it both with a
`FrankWolfeSolver` and with a `:MILPSolver`. Their variants are differential
overrides of the fragment `FWCfg.txt` (the away-step method with at most 50
vertices, `LMOFull` and (P2)); e.g.,
[`BinaryKnapsackBlock/FatherBSCfg-fw.txt`](https://gitlab.com/smspp/tests/-/blob/develop/BinaryKnapsackBlock/FatherBSCfg-fw.txt)
is the vanilla method:

```text
FrankWolfeSolver      # the decomposition, vanilla Frank-Wolfe
1  # number of ComputeConfig in this BlockSolverConfig
*FWCfg.txt +
1  # f_diff == 1 ==> only the entries below are changed
2  # number of integer parameters
intAlgorithm    0 # vanilla
intMaxAtoms     0 # no active set to bound
```

The suite of `SATBlock` runs `IntegralityBarrierSolver` on the MILP
formulations of its instances
([`SATBlock/IBCfg.txt`](https://gitlab.com/smspp/tests/-/blob/develop/SATBlock/IBCfg.txt)),
with 10 runs of at most 100 iterations of the vanilla method with the
fixed step 0.75
([`SATBlock/FWIBCfg.txt`](https://gitlab.com/smspp/tests/-/blob/develop/SATBlock/FWIBCfg.txt)).

## 19.9 Notes and references {#sec-19-9}

The generalized bundle method, with the duality between the primal and the
dual MP that underlies [Proposition 19.1](19-nondifferentiable.md#prop-19-1),
is that of [Frangioni 2002](Z-bibliography.md#ref-frangioni2002); the survey
of [Frangioni 2020](Z-bibliography.md#ref-frangioni2020) treats the proximal,
trust region and level variants and the untrusted models in a single
framework. The incremental method with upper models, the easy components, the
doubly stabilized and the level methods are those of [van Ackooij and
Frangioni 2018](Z-bibliography.md#ref-vanackooij2018), [Frangioni and Gorgone
2014](Z-bibliography.md#ref-frangioni2014), [de Oliveira and Solodov
2016](Z-bibliography.md#ref-deoliveira2016) and [Lemaréchal et al.
1995](Z-bibliography.md#ref-lemarechal1995), respectively. The integer variant
follows [van Ackooij et al. 2016](Z-bibliography.md#ref-vanackooij2016) (cf.
its Theorem 1 for the argument of [Proposition
19.2](19-nondifferentiable.md#prop-19-2)) and [Baena et al.
2020](Z-bibliography.md#ref-baena2020). The unified view of the subgradient
methods, with the schemes, the safe rule and a computational comparison of the
rules on Lagrangian bounds, is that of [Frangioni et al.
2017](Z-bibliography.md#ref-frangioni2017), and the relation between the
Volume algorithm and the bundle methods is discussed by [Bahiense et al.
2002](Z-bibliography.md#ref-bahiense2002). `FrankWolfeSolver` follows, as
closely as possible, the package of [Besançon et al.
2022](Z-bibliography.md#ref-besancon2022), and (P2) is the bound of the
reformulation of [Dantzig and Wolfe 1960](Z-bibliography.md#ref-dantzig1960)
applied to the sub-`Block`s.

