# 14. LagBFunction and BendersBFunction {#ch-14}

[Source: [`SMS++/include/LagBFunction.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/LagBFunction.h), [`BendersBFunction.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/BendersBFunction.h),
[`LagrangianDualSolver/include/LagrangianDualSolver.h`](https://gitlab.com/smspp/lagrangiandualsolver/-/blob/develop/include/LagrangianDualSolver.h),
[`BendersDecompositionSolver/include/BendersDecompositionSolver.h`](https://gitlab.com/smspp/bendersdecompositionsolver/-/blob/develop/include/BendersDecompositionSolver.h);
[`CapacitatedFacilityLocationBlock/include/CapacitatedFacilityLocationBlock.h`](https://gitlab.com/smspp/capacitatedfacilitylocationblock/-/blob/develop/include/CapacitatedFacilityLocationBlock.h)]

Two members of the `Function` family deserve their own chapter. Each is
at the same time a `Block` and a `C05Function`: it wraps a single inner
`Block` and presents that inner `Block`, evaluated, as a first-order
function of some outer `Variable`s. They turn the idea "the value of a
function is the optimum of a sub-problem"
([§13.2](13-function-family.md#sec-13-2)) into reusable components, and
they are the engines of Lagrangian decomposition
([Recipe R4](R4-cfl-lagrangian.md#rec-R4)) and of Benders decomposition
([Recipe R5](R5-cfl-benders.md#rec-R5)).

## 14.1 Two hybrids, one pattern {#sec-14-1}

[`LagBFunction`](https://smspp.gitlab.io/smspp-project/d2/df7/class_s_m_spp__di__unipi__it_1_1_lag_b_function.html)
and
[`BendersBFunction`](https://smspp.gitlab.io/smspp-project/dd/d28/class_s_m_spp__di__unipi__it_1_1_benders_b_function.html)
both derive from `Block` and from `C05Function`. As a `Block`, each
holds one sub-`Block`, the "base" `Block` $(B)$ that represents the
sub-problem. As a `C05Function`, each is a first-order function of a set
of active `Variable`s, which we call the *outer* `Variable`s, that are
*not* `Variable`s of the hybrid `Block` itself (they belong to whatever
outer `Block` defines them). The pattern that both follow has four
parts:

- to *evaluate* the function at given values of the outer `Variable`s,
  the hybrid sets up $(B)$ accordingly and calls `compute()` of a
  generic `:Solver` registered to $(B)$;
- the *linearization* of the function at that point is read off the
  primal or the dual solution of $(B)$, as in Lemma 13.1;
- the hybrid keeps *pools of primal or dual `Solution`s* of $(B)$, one
  per linearization, so that the surrounding cutting-plane, bundle or
  Benders method can reoptimize;
- any *change of $(B)$* is translated into the corresponding change of
  the value and of the pools, and issued as a `C05FunctionMod`
  ([Chapter 8](08-modification-janus.md#ch-8),
  [Chapter 13](13-function-family.md#ch-13)) to whoever solves the
  outer problem.

Both hybrids also apply the weight of the `Objective` of their inner
`Block` ([§12.2](12-sub-block.md#sec-12-2)), which the `:Solver` of
$(B)$ does not see. The value, the linearizations and the constant term
of a `BendersBFunction` are those of its inner `:Solver` multiplied by
the weight of $(B)$ and by its own. A `LagBFunction` does the same with
the value and the constants of its linearizations, while it writes in
the `Objective` of each `Block` of $(B)$ the Lagrangian term divided by
the weight of that `Objective` (so that the Lagrangian term enters with
no weight), and it divides the cutoffs and the absolute accuracy it
gives to the inner `:Solver` by the weight of $(B)$. A change of the
weights inside $(B)$ reaches the hybrid as a `BlockWeightMod`, which a
`LagBFunction` turns into new Lagrangian costs and an `AlphaChanged`
`LagBFunctionMod`.

Since the inner `Block` may be "almost any" `Block`, solved by "almost
any" `:Solver`, these two classes give a generic way to build the two
most important decomposition functions over arbitrary structured
sub-problems.

## 14.2 `LagBFunction`: the Lagrangian function of a `Block` {#sec-14-2}

`LagBFunction` represents the Lagrangian function of its inner `Block`
$(B)$ with respect to a chosen set of terms
([`LagBFunction.h:76-155`](https://gitlab.com/smspp/smspp/-/blob/develop/include/LagBFunction.h#L76-155)). Given the inner problem

$$
(B) \quad \max \{\, c(x) : x \in X \,\}
$$

whose `Objective` is an `FRealObjective` with a `LinearFunction` or a
`DQuadFunction` inside, and a vector of pairs
$\langle y_i , g_i(x) \rangle$, $i \in I$, where each $g_i$ is a
`LinearFunction` in the same `Variable`s $x$ as $(B)$ and each $y_i$ is a
Lagrangian multiplier,
`LagBFunction` is the function

$$
\varphi(y) = \max \Big\{\, c(x) + \sum_{i \in I} y_i g_i(x) :
x \in X \,\Big\} .
\tag{14.1}
$$

The $g_i$ are meant to be (a part of) the complicating constraints of
some original problem $(O)$, relaxed in a Lagrangian fashion; the
multipliers $y_i$ are the outer `Variable`s, `ColVariable`s that the
`LagBFunction` does not own (they are "conceptually fixed" while $(B)$
is solved), and the sign constraints that the relaxed inequalities
impose on them are a concern of the outer `Block`. The pairs can be
added and removed (`add_dual_pairs()`, `remove_variable[s]()`), which is
how dynamically generated constraints are relaxed. The properties of
(14.1) are those of Lemma 13.1, whose proof uses neither the linearity
of $c$ nor that of $x \mapsto b - A x$.

**Lemma 14.1.** *Assume that the maximum in (14.1) is attained for every
$y$. Then $\varphi$ is convex, and if $\bar x$ is $\varepsilon$-optimal
in (14.1) at $\bar y$, then $h = [ g_i(\bar x) ]_{i \in I}$ satisfies
$\varphi(y) \ge \varphi(\bar y) - \varepsilon + h (y - \bar y)$ for all
$y$, i.e., it is an $\varepsilon$-subgradient of $\varphi$ at $\bar y$.
If $(B)$ is a minimization, $\varphi$ is concave and $h$ is an
$\varepsilon$-supergradient.*

*Proof.* For every $x \in X$ the function $y \mapsto c(x) + \sum_i y_i
g_i(x)$ is affine, and $\varphi$ is their pointwise maximum, hence
convex. For every $y$, $\bar x$ is feasible in (14.1), and therefore
$\varphi(y) \ge c(\bar x) + y h = c(\bar x) + \bar y h + (y - \bar y) h
\ge \varphi(\bar y) - \varepsilon + (y - \bar y) h$, the last step by
the $\varepsilon$-optimality of $\bar x$. The minimization case follows
by applying this to $-c$ and $-g$. $\square$

Hence `LagBFunction` is, in general, a nonsmooth `C05Function`: the
gradient exists at $y$ only if the optimal $x(y)$ gives a unique $h$,
which may easily not happen. `LagBFunction` automates the whole
apparatus: turning $(B)$ into $\varphi$, evaluating it by calling a
`:Solver` registered to $(B)$, maintaining the local and global pools,
and keeping $\varphi$ in sync as $(B)$ changes. It is not supposed to
have any `Variable` or `Constraint` of its own beyond those of $(B)$.

### Reoptimization: mapping inner `Modification`s to `FunctionMod*`

`LagBFunction` is where the vocabulary of
[§13.3](13-function-family.md#sec-13-3) is put to work. When $(B)$
changes (a cost of its objective, a side of one of its constraints, a
`Variable` added or removed), $(B)$ issues a `Modification`, which
`LagBFunction` translates into a `C05FunctionMod` for $\varphi$ whose
`shift()` is as precise as the change allows: finite when the change
shifts $\varphi$ by a known constant, $\pm\infty$ when only the
direction is known, NaN otherwise. The direction follows from the
monotonicity of (14.1) in its feasible set: if a change turns $X$ into
$X' \supseteq X$ and leaves the objective unchanged on $X$, then the
new function satisfies $\varphi' \ge \varphi$ everywhere, since every
$x \in X$ is still feasible, which is an upward monotone change, and
symmetrically $X' \subseteq X$ gives a downward one. The addition and
removal of Lagrangian pairs, instead, change the outer `Variable`s, and
they are issued as `C05FunctionModVarsAddd`, `C05FunctionModVarsRngd`
and `C05FunctionModVarsSbst` with `shift()` 0: they are strongly
quasi-additive by Lemma 13.4, and therefore the whole global pool
survives them. Thanks to this translation, a `BundleSolver` driving the
Lagrangian dual can react to a change of the inner problem by keeping
the parts of its bundle that remain valid (Lemma 13.2), rather than
rebuilding the dual from scratch.

Two general facts support the translation. First, the hybrid only reads
the kinds of `Modification` of $(B)$ that can affect it (its
`concerned_by()`), and a `Modification` it does not recognise is judged
by what that `Modification` says it changes (`Modification::changes()`,
[Chapter 8](08-modification-janus.md#ch-8)), rather than taken as a
change of everything. Second, the primal `Solution`s stored in the
global pool are adapted to a change of $(B)$ (`Solution::adapt()`,
which answers whether a `Solution` is unchanged, adapted or invalid),
so that, say, the global pool of a `LagBFunction` survives the removal
of an inner `Variable`.

### The `LagrangianDualSolver`

A user rarely constructs a `LagBFunction` by hand. The usual route is the
[`LagrangianDualSolver`](https://smspp.gitlab.io/smspp-project/d4/dc3/class_s_m_spp__di__unipi__it_1_1_lagrangian_dual_solver.html),
a `CDASolver` that solves the Lagrangian dual of a "generic" `Block` $(B)$
with a precise structure ([`LagrangianDualSolver.h:63-100`](https://gitlab.com/smspp/lagrangiandualsolver/-/blob/develop/include/LagrangianDualSolver.h#L63-100)). It has no
`Variable` of its own, at least one sub-`Block`, no `Objective` or one with no
`Variable` (its constant term being part of the dual value), and no dependence
on `Variable`s outside of $(B)$. If there is more than one sub-`Block`, its
`Constraint`s are all and only the linking constraints between the
sub-`Block`s, and all of them are `FRowConstraint`s with a `LinearFunction`
inside. Given such a $(B)$, the `LagrangianDualSolver` builds a hidden
`LagrangianDualBlock` $(LB)$ with one sub-`Block` $(LB_i)$ per sub-`Block`
$(B_i)$ of $(B)$; each $(LB_i)$ is empty, save for an `FRealObjective`
containing a `LagBFunction` whose inner `Block` is $(B_i)$, with the linking
constraints dualised, and the user-chosen inner `:Solver` (typically a
`BundleSolver`) is registered to $(LB)$. The father of $(B_i)$ becomes the
`LagBFunction`, while $(B)$ still "believes" that $(B_i)$ is its sub-`Block`,
and $(B)$ is made the father of the `LagBFunction`, so that every
`Modification` of $(B_i)$ reaches both $(LB)$, translated, and $(B)$
([`LagrangianDualSolver.h:100-140`](https://gitlab.com/smspp/lagrangiandualsolver/-/blob/develop/include/LagrangianDualSolver.h#L100-140)); all this is undone when the
`LagrangianDualSolver` is unregistered. If something requires the original
father-son relation to be kept, the parameter `int_LDSlv_iBCopy` makes the
`LagBFunction` wrap a copy of $(B_i)$ instead, kept in sync by an
`UpdateSolver` ([§10.6](10-r3block.md#sec-10-6)), which requires
`get_R3_Block( nullptr )` and `map_back_solution()` to work for every $(B_i)$.

The output is a dual bound and a *convexified* primal solution, i.e.,
the convex combination of inner solutions given by the multipliers of
the important linearization ([§13.2](13-function-family.md#sec-13-2)).
Its meaning is the classical result on Lagrangian duality (Geoffrion,
1974, in the Bibliography), which we recall in the form in which it
applies here.

**Theorem 14.2.** *Let $(O)$ be $\max \{ c x : G x = d ,\; x \in X \}$
with $X$ a finite set (e.g., the integer points of a bounded polyhedron,
such as the solutions of a set of knapsacks), $c$ and $G$ linear, and
let $\varphi(y) = \max \{ c x + y (d - G x) : x \in X \}$. Then
$\min_y \varphi(y) = \max \{ c x : G x = d ,\; x \in \mathrm{conv}(X)
\}$ whenever the latter is feasible, and therefore the Lagrangian dual
bound is the value of the convexified relaxation, at least as strong as
that of the continuous relaxation and never weaker than $v(O)$.*

*Proof.* Since $X = \{ x^1, \ldots, x^p \}$ is finite,
$\min_y \varphi(y)$ is the linear program
$\min \{ \eta : \eta \ge c x^k + y (d - G x^k),\; k = 1, \ldots, p \}$
in the variables $(\eta, y)$. Its dual has a variable $\lambda_k \ge 0$
per constraint, and reads $\max \{ \sum_k \lambda_k c x^k :
\sum_k \lambda_k = 1 ,\; \sum_k \lambda_k (d - G x^k) = 0 ,\;
\lambda \ge 0 \}$, i.e., setting $x = \sum_k \lambda_k x^k \in
\mathrm{conv}(X)$, $\max \{ c x : G x = d ,\; x \in \mathrm{conv}(X)
\}$. If the latter is feasible, both linear programs are feasible (the
first one always is, for $\eta$ large), and linear programming duality
gives the equality. The other claims follow from $\{ x : G x = d \} \cap
X \subseteq \{ x : G x = d \} \cap \mathrm{conv}(X)$, and from
$\mathrm{conv}(X)$ being contained in the continuous relaxation of $X$
when $X$ is the set of integer points of a polyhedron. $\square$

In words, the convexified solution is optimal for the convexified
relaxation, and this is the precise sense in which it is "better" than
any single inner solution, none of which needs to satisfy $G x = d$; it
is not, in general, feasible for $(O)$, and the gap between
$\min_y \varphi$ and $v(O)$ is the price of the convexification.

*The running example.* CFL in the Knapsack Formulation
([Chapter 12](12-sub-block.md#ch-12)) is a `Block` of this shape: a
master carrying only the linking constraints $\sum_i X_{ij} = 1$, over
`f_n_facilities` `BinaryKnapsackBlock` sub-`Block`s. Attaching a
`LagrangianDualSolver` to it dualises the linking constraints, leaving
one Lagrangian knapsack per facility, each a `LagBFunction` over a
`BinaryKnapsackBlock` solved by a `DPBinaryKnapsackSolver`; by
Theorem 14.2 its bound is that of the formulation with each knapsack
replaced by its convex hull, which is at least as strong as the strong
continuous relaxation of CFL. [Recipe R4](R4-cfl-lagrangian.md#rec-R4)
develops this end to end, and shows how `PrimalProximalHeur` turns the
convexified primal solution into a feasible one.

## 14.3 `BendersBFunction`: the value function of a `Block` {#sec-14-3}

`BendersBFunction` represents the *value function* of its inner `Block`
$(B)$ as a function of outer `Variable`s that enter the sides of the
constraints of $(B)$ ([`BendersBFunction.h:65-143`](https://gitlab.com/smspp/smspp/-/blob/develop/include/BendersBFunction.h#L65-143)). The inner `Block`
has the form

$$
(B) \quad \min \{\, c(y) : w \le E(y) \le z ,\; y \in Y \,\} ,
$$

where $Y$ is a convex set described by `RowConstraint`s and bounds,
and typically comes from an original problem
$(O)\; \min \{ d(x) + c(y) : g \le F x + E(y) \le h ,\; x \in X ,\;
y \in Y \}$, reformulated as $\min \{ d(x) + \varphi(x) : x \in X \}$
with

$$
\varphi(x) = \min \{\, c(y) : g - F x \le E(y) \le h - F x ,\;
y \in Y \,\} .
\tag{14.2}
$$

The dependence on $x$ is given by a matrix $A$, a vector $b$ and a
vector of pairs $(C_i, S_i)$, each made of a `RowConstraint` $C_i$ of
$(B)$ and a side $S_i$ (`eLHS`, `eRHS` or `eBoth`): the mapping
$M(x) = A x + b$ gives, in its $i$-th component, the value that the
side $S_i$ of $C_i$ takes, which is how $g - F x$ and $h - F x$ are
written. The outer `Variable`s $x$ are the active `Variable`s of the
`BendersBFunction`, the $j$-th corresponding to the $j$-th column of
$A$; no pair may appear twice, and a constraint with `eBoth` may not
appear again. Evaluating $\varphi(x)$ is one solve of $(B)$ with its
sides set to $M(x)$; the dual solution of $(B)$ gives the linearization
(a Benders *optimality cut*), and when $(B)$ is infeasible at $x$ a dual
unbounded ray (a Farkas certificate) gives a *vertical* linearization (a
Benders *feasibility cut*). Both are valid by linear programming
duality, which we state for the linear case, in which $c(y) = c y$,
$E(y) = E y$ and $Y$ is folded into the rows of $E$ with sides that do
not depend on $x$.

**Lemma 14.3.** *Let $\varphi(x) = \min \{ c y : w(x) \le E y \le z(x)
\}$, with $w(x)$ and $z(x)$ affine in $x$ (some components possibly
infinite and constant). (i) For every $(\lambda, \mu) \ge 0$ with
$(\lambda - \mu) E = c$ and $\lambda_k = 0$ ($\mu_k = 0$) wherever
$w_k = -\infty$ ($z_k = +\infty$), the affine function
$L(x) = \lambda w(x) - \mu z(x)$ satisfies $L(x) \le \varphi(x)$ for
all $x$, with equality at $\bar x$ if $(\lambda, \mu)$ is dual optimal
there and $\varphi(\bar x)$ is finite; hence $\varphi$ is convex on its
domain. (ii) For every $(\lambda, \mu) \ge 0$ with $(\lambda - \mu) E =
0$ and the same zeros, every $x$ at which $(B)$ is feasible satisfies
$\lambda w(x) - \mu z(x) \le 0$; if $\lambda w(\bar x) - \mu z(\bar x)
> 0$, the inequality is violated at $\bar x$ and cuts it off.*

*Proof.* (i) If $y$ is feasible at $x$, then $\lambda w(x) \le \lambda
E y$ and $\mu z(x) \ge \mu E y$ (the terms with infinite sides having
zero multiplier), hence $L(x) \le (\lambda - \mu) E y = c y$; taking the
infimum over the feasible $y$ gives $L(x) \le \varphi(x)$, which is
trivially true where $(B)$ is infeasible ($\varphi = +\infty$). At
$\bar x$, if $(\lambda, \mu)$ is dual optimal, strong duality of linear
programming gives $L(\bar x) = \varphi(\bar x)$. Convexity follows as
in Lemma 14.1, $\varphi$ being the supremum of the affine $L$ over the
dual feasible set, by strong duality wherever it is finite. (ii) The
same chain gives $\lambda w(x) - \mu z(x) \le (\lambda - \mu) E y = 0$
for every feasible $y$. $\square$

Since $w$ and $z$ are affine in $x$ through $M(x) = A x + b$, the
linearization of (i) is $g x + \alpha$ with $g$ and $\alpha$ obtained
from $(\lambda, \mu)$, $A$ and $b$, which is what the
`BendersBFunction` stores in its pools. For a nonlinear convex $(B)$ the
same holds with Lagrangian duality in place of linear duality, under a
constraint qualification.

Like `LagBFunction`, `BendersBFunction` has no `Variable` or
`Constraint` of its own beyond those of $(B)$, evaluates by a generic
`:Solver` registered to $(B)$, and maintains the linearization pools.
It also maps the changes of $(B)$ into the `FunctionMod*` types of
[§13.3](13-function-family.md#sec-13-3). The dual solutions in its
global pool are what the translation has to preserve, and the cases are
distinguished accordingly (`BendersBFunction::add_Modification()`). A
change that only moves the sides of some `Constraint` of $(B)$
(including relaxing or enforcing one) keeps all of them dual feasible,
since dual feasibility in Lemma 14.3 (i) does not involve the sides,
and therefore only the constants of the linearizations are computed
again, the value moving up or down with the feasible region when this
moves one way only. Removing `Variable`s from $(B)$ also keeps the
pool (a removed column only drops one of the equations of dual
feasibility), the value changing unpredictably; adding a `Constraint`
keeps the pool (its multiplier being 0), and removing one keeps the
entries in which its multiplier is zero. Any other change of the model
of $(B)$ may take away the feasibility of the dual solutions, and it is
reported as the "nuclear" `FunctionMod`; the mapping is therefore
coarser than that of `LagBFunction` for changes of the coefficients and
of the `Objective` of $(B)$.

*The running example.* CFL in the Benders Formulation
([§3.5](03-mental-model.md#sec-3-5)) is built on this: the master
carries the design `Variable`s $Y$ and a single epigraphic `Variable`
$v$, and the transportation sub-problem is a hidden `MCFBlock` (the flow
relaxation of [§10.5](10-r3block.md#sec-10-5), with the facility arcs at
cost 0) wrapped in a `BendersBFunction`. As the master MILP visits
design points $\hat Y$, a callback evaluates the `BendersBFunction`
(one MCF solve) and emits either an optimality cut, from the dual, or a
feasibility cut, from the Farkas ray in the feasibility-cuts
sub-variant. [Recipe R5](R5-cfl-benders.md#rec-R5) develops this end to
end.

`BendersBFunction` is also the component that produces the cuts in an
`SDDPBlock`, where the Benders cuts generated at each stage are
accumulated into a `PolyhedralFunction`
([§13.5](13-function-family.md#sec-13-5)) representing the
approximation of the recourse cost of the following stage.

*The Benders master.* Since version 0.7.0 the enclosing Benders master
is built by `BendersDecompositionSolver`, in the way
`LagrangianDualSolver` builds the Lagrangian one. The complicating
`Variable`s $x$ and the subproblems are those named by the parameters
`vintMasterVars` and `vintMasterBlock`, since the same `Block` admits,
in general, many Benders reformulations. In each subproblem the
`:Solver` finds the rows that are linear in $x$, and it wraps the
subproblem in a `BendersBFunction`; the master is then solved in one of
two regimes, selected by `int_BDSlv_Regime`. In the *convex* regime the
value functions are `C05Function` terms of the master `Objective`, and
a `:CDASolver` of the bundle family (e.g., `BundleSolver`, or
`ParallelBundleSolver` if the subproblems are to be evaluated in
parallel) drives a stabilized cutting-plane method. In the *MILP*
regime, instead, the master is a MILP with one epigraph `Variable` per
subproblem (or a single one), the cuts enter it as dynamic
`Constraint`s, and the master is solved again after each round of cuts.
A `TwoStageStochasticBlock` provides the required form through
`get_Benders_form()`, and the `-k` option of `smspp_tssb_solver` uses
it. Also, the arrangement of [Recipe R5](R5-cfl-benders.md#rec-R5), in
which the cuts are emitted by the user-cut callback of the master
`:MILPSolver` driving `generate_dynamic_constraints()`, remains
available; it may be the more natural one when the cuts are to be seen
by the branch-and-cut of the master while it proceeds.

## 14.4 Why "almost any `Block`, almost any `Solver`" {#sec-14-4}

The inner `Block` $(B)$ of the two hybrids is not constrained to be of
any particular type, and neither is the `:Solver` that evaluates it. A
`LagBFunction` can wrap a `BinaryKnapsackBlock` solved by dynamic
programming, a `UCBlock` solved by a `:MILPSolver`, or a `Block` that is
itself decomposed and solved by a nested `LagrangianDualSolver`; a
`BendersBFunction` can wrap an `MCFBlock` solved by a network simplex,
or a far more complex sub-problem solved by whatever fits. This is what
lets SMS++ express multi-level, heterogeneous, nested decompositions
(Benders outside, SDDP in the middle, Lagrange inside, dynamic
programming at the leaves) by composing these components, each
indifferent to what the others are. We remark that the guarantees of
Lemmas 14.1 and 14.3 hold only as far as the inner `:Solver` returns
optimal (or $\varepsilon$-optimal) primal and dual solutions: an inner
`:Solver` that only returns feasible ones gives valid bounds in the
direction of Lemma 14.1 but not, in general, valid Benders cuts.

## 14.5 Idioms {#sec-14-5}

*A `LagrangianDualSolver` rather than a hand-built `LagBFunction`.* For
a `Block` with the shape "linking constraints over independent
sub-`Block`s", attaching a `LagrangianDualSolver` builds the
`LagBFunction`s and the hidden `LagrangianDualBlock`; constructing
`LagBFunction`s by hand is for the cases that fall outside that shape.

*The inner `Solver` configured, not only the outer one.* Both hybrids
evaluate their inner `Block` with a `:Solver` that must be registered and
configured. For a `BendersBFunction` whose inner `Block` is hidden (as
in CFL/BenForm), the inner `:Solver` is configured through the
`f_extra_Configuration` of the outer `Block`
([§11.6](11-configuration.md#sec-11-6)), and forgetting it makes the
`Block` throw.

*A bound and a convexified or fractional solution, not the integer
optimum.* A dual built on `LagBFunction`s gives a bound and a convexified
primal solution (Theorem 14.2), and a master built on
`BendersBFunction`s gives a bound and the solution of the master;
turning either into a feasible integer solution is the job of a
subsequent heuristic (`PrimalProximalHeur`, rounding, branching), not of
the hybrid.
