# 14. LagBFunction and BendersBFunction {#ch-14}

[Source: [`SMS++/include/LagBFunction.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/LagBFunction.h), [`BendersBFunction.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/BendersBFunction.h),
[`LagrangianDualSolver/include/LagrangianDualSolver.h`](https://gitlab.com/smspp/lagrangiandualsolver/-/blob/develop/include/LagrangianDualSolver.h),
[`BendersDecompositionSolver/include/BendersDecompositionSolver.h`](https://gitlab.com/smspp/bendersdecompositionsolver/-/blob/develop/include/BendersDecompositionSolver.h);
[`CapacitatedFacilityLocationBlock/include/CapacitatedFacilityLocationBlock.h`](https://gitlab.com/smspp/capacitatedfacilitylocationblock/-/blob/develop/include/CapacitatedFacilityLocationBlock.h);
the tests `test_bds.cpp` of
[`tests/CapacitatedFacilityLocationBlock`](https://gitlab.com/smspp/tests/-/tree/develop/CapacitatedFacilityLocationBlock)
(`CFL_BDS_test`) and of
[`tests/TwoStageStochasticBlock`](https://gitlab.com/smspp/tests/-/tree/develop/TwoStageStochasticBlock)]

We are concerned in this chapter with the two members of the `Function`
family that are at the same time a `Block` and a `C05Function`,
`LagBFunction` and `BendersBFunction`, and with the two `Solver`s that build
decompositions out of them, `LagrangianDualSolver` and
`BendersDecompositionSolver`. Each of the two functions wraps a single inner
`Block`, and presents it, evaluated, as a first-order function of some outer
`Variable`s. Hence, they turn the idea "the value of a function is the
optimum of a subproblem" ([§13.2](13-function-family.md#sec-13-2)) into
reusable components, and they are the engines of Lagrangian decomposition
([Recipe R4](R4-cfl-lagrangian.md#rec-R4)) and of Benders decomposition
([Recipe R5](R5-cfl-benders.md#rec-R5)).

We first describe the pattern that the two functions share
([§14.1](14-lag-benders-bfunction.md#sec-14-1)). Then we discuss the
Lagrangian function of a `Block`, with its pools, the translation of the
changes of the inner `Block`, the `LagrangianDualSolver` and the meaning of
the bound it computes ([§14.2](14-lag-benders-bfunction.md#sec-14-2)), and
the value function of a `Block`, with its optimality and feasibility cuts
and its phase one ([§14.3](14-lag-benders-bfunction.md#sec-14-3)). After a
remark on the generality of the two constructions
([§14.4](14-lag-benders-bfunction.md#sec-14-4)) and the idioms
([§14.5](14-lag-benders-bfunction.md#sec-14-5)), we describe the
`BendersDecompositionSolver`, which builds and solves the Benders
decomposition of a `Block` ([§14.6](14-lag-benders-bfunction.md#sec-14-6));
the notes and references close the chapter
([§14.7](14-lag-benders-bfunction.md#sec-14-7)).

## 14.1 Two hybrids, one pattern {#sec-14-1}

[`LagBFunction`](https://smspp.gitlab.io/smspp-project/d2/df7/class_s_m_spp__di__unipi__it_1_1_lag_b_function.html)
and
[`BendersBFunction`](https://smspp.gitlab.io/smspp-project/dd/d28/class_s_m_spp__di__unipi__it_1_1_benders_b_function.html)
both derive from `Block` and from `C05Function`. As a `Block`, each holds one
sub-`Block`, the "base" `Block` $(B)$ that represents the subproblem. As a
`C05Function`, each is a first-order function of a set of active `Variable`s,
which we call the *outer* `Variable`s, that are *not* `Variable`s of the
hybrid `Block` (they belong to whatever outer `Block` defines them). The
pattern that both follow has four parts:

- to *evaluate* the function at given values of the outer `Variable`s, the
  hybrid sets up $(B)$ accordingly and calls `compute()` of a generic
  `:Solver` registered to $(B)$ (in a `LagBFunction`, the first one, unless
  the parameter `intInnrSlvr` names another, or is $-1$, in which case
  there is none and `compute()` throws);
- the *linearization* of the function at that point is read off the primal or
  the dual solution of $(B)$, as in [Lemma
  13.1](13-function-family.md#lem-13-1);
- the hybrid keeps *pools of primal or dual `Solution`s* of $(B)$, one per
  linearization, so that the surrounding cutting-plane, bundle ([Chapter
  19](19-nondifferentiable.md#ch-19)) or Benders method can reoptimize;
- any *change of $(B)$* is translated into the corresponding change of the
  value and of the pools, and issued as a `C05FunctionMod` ([Chapter
  8](08-modification-janus.md#ch-8), [Chapter
  13](13-function-family.md#ch-13)) to whoever solves the outer problem.

Both hybrids also apply the weight of the `Objective` of their inner `Block`
([§12.2](12-sub-block.md#sec-12-2)), which the `:Solver` of $(B)$ does not
see. The value, the linearizations and the constant term of a
`BendersBFunction` are those of its inner `:Solver` multiplied by the weight
of $(B)$ and by that of the `BendersBFunction`. A `LagBFunction` does the same
with the value and the constants of its linearizations, and it divides the
cutoffs and the absolute accuracy it gives to the inner `:Solver` by the
weight of $(B)$. It writes the Lagrangian cost of each `Variable` in the
`Objective` of the `Block` of $(B)$ that defines it, divided by the weight of
that `Objective`, and hence the Lagrangian term enters with no weight. This is
the default `intPushCostToOwner` 1, while with 0 all of it goes into the
`Objective` of $(B)$, and those of the sub-`Block`s, which are then left
untouched, may be of any kind. A change of the weights inside $(B)$ reaches
the hybrid as a `BlockWeightMod`, which a `LagBFunction` turns into new
Lagrangian costs and an `AlphaChanged` `LagBFunctionMod`.

Since the inner `Block` may be "almost any" `Block`, solved by "almost
any" `:Solver`, these two classes give a generic way to build the two
most important decomposition functions over arbitrary structured
subproblems.

## 14.2 `LagBFunction`: the Lagrangian function of a `Block` {#sec-14-2}

`LagBFunction` represents the Lagrangian function of its inner `Block` $(B)$
with respect to a chosen set of terms
([`LagBFunction.h:76-155`](https://gitlab.com/smspp/smspp/-/blob/develop/include/LagBFunction.h#L76-155)).
Given the inner problem

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

The $g_i$ are meant to be (a part of) the complicating constraints of some
original problem $(O)$, relaxed in a Lagrangian fashion. The multipliers $y_i$
are the outer `Variable`s, i.e., `ColVariable`s that the `LagBFunction` does
not own (they are "conceptually fixed" while $(B)$ is solved), and the sign
constraints that the relaxed inequalities impose on them are a concern of the
outer `Block`. The pairs can be added and removed (`add_dual_pairs()`,
`remove_variable[s]()`), which is how dynamically generated constraints are
relaxed. With respect to
[§13.2](13-function-family.md#sec-13-2), the multipliers $y$ play the role of
the argument $x$ there, and the inner variables $x$ that of $u$; the
properties of (14.1) are therefore those of [Lemma
13.1](13-function-family.md#lem-13-1).

<span id="lem-14-1"></span>**Lemma 14.1.** *Assume that the maximum in (14.1)
is attained for every $y$. Then $\varphi$ is convex, and if $\bar x$ is
$\varepsilon$-optimal in (14.1) at $\bar y$, then $h = [ g_i(\bar x) ]_{i \in
I}$ satisfies $\varphi(y) \ge \varphi(\bar y) - \varepsilon + h (y - \bar y)$
for all $y$, i.e., it is an $\varepsilon$-subgradient of $\varphi$ at $\bar
y$. If $(B)$ is a minimization, $\varphi$ is concave and $h$ is an
$\varepsilon$-supergradient.*

*Proof.* [Lemma 13.1](13-function-family.md#lem-13-1) and the remark after it,
with $c(x)$ in place of $c u$ and $[ g_i(x) ]_{i \in I}$ in place of $b - A
u$: its proof uses them only as the values at $u$ of two functions, and
neither their linearity nor any property of $U$. The minimization case follows
by applying this to $-c$ and $-g$. $\square$

Hence `LagBFunction` is, in general, a nonsmooth `C05Function`: the
gradient exists at $y$ only if the optimal $x(y)$ gives a unique $h$,
which may well not happen; however, one can still use each $h$ as an
$\varepsilon$-subgradient. `LagBFunction` automates the apparatus, i.e.,
turning $(B)$ into $\varphi$, evaluating it by calling a `:Solver`
registered to $(B)$, maintaining the local and global pools, and keeping
$\varphi$ in sync as $(B)$ changes. It is not supposed to have any
`Variable` or `Constraint` beyond those of $(B)$.

### The global pool

An entry of the global pool is a solution $x^*$ of $(B)$, or a convex
combination $\sum_k \theta_k x^k$ of solutions (a *convexified* entry),
together with the constant of its linearization, which is $c(x^*)$, or $\sum_k
\theta_k c(x^k)$ for a convexified entry, plus the Lagrangian term. The
parameter `intPoolExtMem` chooses how this constant is kept
([`LagBFunction.h:467-485`](https://gitlab.com/smspp/smspp/-/blob/develop/include/LagBFunction.h#L467-485)).
With 0, the default, the constant is stored, and a change of the linear costs
of $(B)$ updates it in place by the scalar product of the change with the
stored point. With 1, only the difference between the constant and the value
of $c$ at the stored point is kept, and the constant is computed again from
that point when it is needed, which is correct only for an affine $c$. The two
differ when $c$ is not affine, since then $\sum_k \theta_k c(x^k)$ and
$c(\sum_k \theta_k x^k)$ differ in general.

A change of the quadratic coefficients of the `DQuadFunction` in the
`Objective` of $(B)$ is a case in point, and the parameter `intPoolQuadMem`
(since SMS++ 0.9.0) decides what happens to the convexified entries. With 0,
the default, they are deleted. With 1, each entry keeps, for each `Variable`
$x_j$ of the `DQuadFunction`, the value $s_j = \sum_k \theta_k (x^k_j)^2$,
combined in the same way as the solutions when a convexified entry is formed,
and the constant is updated exactly, by the following elementary fact.

<span id="lem-14-2"></span>**Lemma 14.2.** *Let $c(x) = \sum_j ( q_j x_j^2 +
a_j x_j ) + c_0$, and let an entry of the global pool combine the solutions
$x^k$ with weights $\theta_k \ge 0$, $\sum_k \theta_k = 1$, with constant
$\alpha = \sum_k \theta_k c(x^k)$. If $q$ changes into $q + \Delta q$, the
constant of the same combination of the same solutions becomes $\alpha +
\sum_j \Delta q_j s_j$, with $s_j = \sum_k \theta_k (x^k_j)^2$, while
$s_j = (\sum_k \theta_k x^k_j)^2$ holds only if all the $x^k_j$ with
$\theta_k > 0$ coincide.*

*Proof.* The new constant is $\sum_k \theta_k ( c(x^k) + \sum_j \Delta q_j
(x^k_j)^2 ) = \alpha + \sum_j \Delta q_j \sum_k \theta_k (x^k_j)^2$. The last
claim is the case of equality of the strict convexity of the square.
$\square$

Hence, the combined point alone does not allow the update, and this is why the
convexified entries are deleted unless $s_j$ is kept. Even with 1, an entry
that has no $s_j$ (e.g., one restored from a `LagBFunctionState`, which does
not save them) is deleted, and so is a vertical entry when a changed $q_j$
multiplies a nonzero component of its direction. Note that the `Modification`s
that `DQuadFunction` issues when `Variable`s are removed from it,
`DQuadFunctionModVarsRngd` and `DQuadFunctionModVarsSbst` (since SMS++ 0.9.0),
carry the linear and the quadratic coefficients of the removed `Variable`s,
and `LagBFunction` uses them to update the constants of the global pool in the
same way.

### Reoptimization: mapping inner `Modification`s to `FunctionMod*`

`LagBFunction` is where the vocabulary of
[§13.3](13-function-family.md#sec-13-3) is put to work. When $(B)$ changes (a
cost of its objective, a side of one of its constraints, a `Variable` added or
removed), it issues a `Modification`, which `LagBFunction` translates into a
`C05FunctionMod` for $\varphi$. The `shift()` of this is finite only when the
`Objective` of $(B)$ moves by a constant, i.e., when its constant term
changes: then $\varphi$ moves by a constant, and the constants of the global
pool are shifted with it. A change of the constant term of one of the $g_i$
changes the $i$-th entry of all the linearizations by the same known amount,
and it is issued as a `C05FunctionModLinRngd`, whose `shift()` is NaN since
the change of the value depends on $y_i$. Any other change of $(B)$ is issued
with `shift()` NaN, whence a `:Solver` of the outer problem keeps no bound
([Lemma 13.2](13-function-family.md#lem-13-2) (iv)). This holds also for a
change that can only grow $X$, say, a `Variable` that is no longer fixed
(since SMS++ 0.9.0): such a change cannot take away the feasibility of the
solutions in the global pool, which is therefore kept whole, but it changes
the value of $\varphi$ by an amount that is not known. A change that may
shrink $X$, instead, may make some of the stored solutions infeasible: each of
them is checked, those that are no longer feasible are deleted, and the
`C05FunctionMod` lists them.

The addition and removal of Lagrangian pairs, instead, change the outer
`Variable`s, and they are issued as `C05FunctionModVarsAddd`,
`C05FunctionModVarsRngd` and `C05FunctionModVarsSbst` with `shift()` 0. They
are strongly quasi-additive by [Lemma 13.4](13-function-family.md#lem-13-4),
and therefore all the global pool survives them. Thanks to this translation, a
`BundleSolver` driving the Lagrangian dual ([Chapter
19](19-nondifferentiable.md#ch-19)) can react to a change of the inner problem
by keeping the parts of its bundle that remain valid ([Lemma
13.2](13-function-family.md#lem-13-2)), instead of rebuilding the dual from
scratch.

Two general facts support the translation. First, the hybrid only reads
the kinds of `Modification` of $(B)$ that can affect it (its
`concerned_by()`), and a `Modification` it does not recognize is judged
by what that `Modification` declares to change (`Modification::changes()`,
[Chapter 8](08-modification-janus.md#ch-8)), instead of being taken as a
change of everything. Second, the primal `Solution`s stored in the global
pool are adapted to a change of $(B)$ (`Solution::adapt()`, which answers
whether a `Solution` is unchanged, adapted or invalid); for instance, the
global pool of a `LagBFunction` survives the removal of an inner `Variable`.

### The `LagrangianDualSolver`

A user rarely constructs a `LagBFunction` by hand. The usual route is the
[`LagrangianDualSolver`](https://smspp.gitlab.io/smspp-project/d4/dc3/class_s_m_spp__di__unipi__it_1_1_lagrangian_dual_solver.html),
a `CDASolver` that solves the Lagrangian dual of a "generic" `Block` $(B)$
with a precise structure
([`LagrangianDualSolver.h:63-98`](https://gitlab.com/smspp/lagrangiandualsolver/-/blob/develop/include/LagrangianDualSolver.h#L63-98)).
It has no `Variable`, at least one sub-`Block`, no `Objective` or one with no
`Variable` (its constant term being part of the dual value), and no dependence
on `Variable`s outside of $(B)$. If there is more than one sub-`Block`, its
`Constraint`s are all and only the linking constraints between the
sub-`Block`s; in any case, all of them are `FRowConstraint`s with a
`LinearFunction` (with no constant term) inside, while `OneVarConstraint`s are
not supported. A relaxed row has one multiplier, and therefore a ranged row
($-\infty < l < u < +\infty$) is refused with `std::invalid_argument`, by
`set_Block()` or, for a dynamic row added later, by `compute()`, and so is a
row with $l > u$; a ranged row has to be written as two one-sided ones. For
the same reason the type of a relaxed row (equality, $\le$, $\ge$ or free) is
fixed when its multiplier is created, and a change of its sides that changes
the type makes each following `compute()` throw `std::logic_error`, until the
row gets its type back
([`LagrangianDualSolver.h:338-363`](https://gitlab.com/smspp/lagrangiandualsolver/-/blob/develop/include/LagrangianDualSolver.h#L338-363)).
With `intRecursive` (0 by default) the decomposition does not stop at the
children of $(B)$: a child that has the same shape as $(B)$ is decomposed in
turn, and its linking rows are relaxed too. Finally, no sub-`Block` of $(B)$
may make any assumption on the type of $(B)$, or refer to its data, and this
allows the following arrangement.

Given such a $(B)$, the `LagrangianDualSolver` builds a hidden
`LagrangianDualBlock` $(LB)$ with one sub-`Block` $(LB_i)$ per sub-`Block`
$(B_i)$ of $(B)$. Each $(LB_i)$ is empty, save for an `FRealObjective`
containing a `LagBFunction` whose inner `Block` is $(B_i)$, with the linking
constraints dualized, and the inner `:Solver` chosen by the user (typically a
`BundleSolver`) is registered to $(LB)$. While the `LagrangianDualSolver`
needs the dual whole, i.e., within `compute()`, `get_var_solution()` and
`get_dual_solution()`, the father of $(B_i)$ is its `LagBFunction` (although
$(B)$ still "believes" that $(B_i)$ is its sub-`Block`), and $(B)$ is made the
father of the `LagBFunction`; hence, each `Modification` of $(B_i)$ reaches
both $(LB)$, translated, and $(B)$
([`LagrangianDualSolver.h:100-131`](https://gitlab.com/smspp/lagrangiandualsolver/-/blob/develop/include/LagrangianDualSolver.h#L100-131)).
The rest of the time $(B_i)$ is given back to $(B)$, which is then whole for
any other `Solver` attached to it, and a `Modification` that $(B_i)$ issues
meanwhile reaches the `LagrangianDualSolver` through $(B)$; the
`LagrangianDualSolver` keeps it aside, and hands it to the `LagBFunction` when
it takes $(B_i)$ again
([`LagrangianDualSolver.h:1394-1407`](https://gitlab.com/smspp/lagrangiandualsolver/-/blob/develop/include/LagrangianDualSolver.h#L1394-1407)).
All this is undone when the `LagrangianDualSolver` is unregistered. If
something requires the original father-son relation to be kept, the parameter
`int_LDSlv_iBCopy` makes the `LagBFunction` wrap a copy of $(B_i)$ instead,
kept in sync by an `UpdateSolver` ([§10.6](10-r3block.md#sec-10-6)), which
requires `get_R3_Block( nullptr )` and `map_back_solution()` to work for each
$(B_i)$.

The output is a dual bound and a *convexified* primal solution, i.e.,
the convex combination of inner solutions given by the multipliers of
the important linearization ([§13.2](13-function-family.md#sec-13-2)).
Its meaning is the classical result on Lagrangian duality
([Geoffrion 1974](Z-bibliography.md#ref-geoffrion1974)), which we recall in
the form in which it applies here, i.e., for a set $X$ whose convex hull is
a polytope. This covers a finite $X$, and also the set of the mixed-integer
points of a bounded polyhedron, such as the knapsacks with binary and
continuous variables of the splittable CFL: the integer variables take
finitely many values, for each of which the continuous ones range over a
polytope, and the convex hull of a finite union of polytopes is a
polytope.

<span id="thm-14-1"></span>**Theorem 14.1.** *Let $(O)$ be $\max \{ c x : G
x = d ,\; x \in X \}$ with $X \subseteq \mathbb{R}^n$ nonempty and such that
$\mathrm{conv}(X)$ is a polytope, $c$ and $G$ linear, and let $\varphi(y) =
\max \{ c x + y (d - G x) : x \in X \}$. Then $\min_y \varphi(y) = \max \{ c
x : G x = d ,\; x \in \mathrm{conv}(X) \}$ whenever the latter is feasible.
Hence the Lagrangian dual bound is the value of the convexified relaxation,
which is never weaker than $v(O)$, and, if $X$ is the set of the
mixed-integer points of a bounded polyhedron $P$, at least as strong as the
value of the continuous relaxation over $P$.*

*Proof.* Let $x^1, \ldots, x^p$ be the vertices of $\mathrm{conv}(X)$; each
of them belongs to $X$, since an extreme point of the convex hull of a set
belongs to the set. For each $y$, the linear function $c x + y (d - G x)$
attains its maximum over the polytope $\mathrm{conv}(X)$ at a vertex, and
$\{ x^1, \ldots, x^p \} \subseteq X \subseteq \mathrm{conv}(X)$; hence,
$\varphi(y) = \max_k \{ c x^k + y (d - G x^k) \}$. Thus, $\min_y
\varphi(y)$ is the linear program $\min \{ \eta : \eta \ge c x^k + y (d - G
x^k),\; k = 1, \ldots, p \}$ in the variables $(\eta, y)$. Its dual has a
variable $\lambda_k \ge 0$ per constraint, and reads $\max \{ \sum_k
\lambda_k c x^k : \sum_k \lambda_k = 1 ,\; \sum_k \lambda_k (d - G x^k) = 0
,\; \lambda \ge 0 \}$, i.e., setting $x = \sum_k \lambda_k x^k$ and since
$\mathrm{conv}( \{ x^1, \ldots, x^p \} ) = \mathrm{conv}(X)$, $\max \{ c x :
G x = d ,\; x \in \mathrm{conv}(X) \}$. If the latter is feasible, both
linear programs are feasible (the first one always is, for $\eta$ large), and
linear programming duality gives the equality. The other claims follow from
$\{ x : G x = d \} \cap X \subseteq \{ x : G x = d \} \cap \mathrm{conv}(X)$,
and from $\mathrm{conv}(X) \subseteq P$ when $X \subseteq P$, since $P$ is
convex. $\square$

We remark that for rows $G x \le d$, relaxed with multipliers $y \ge 0$ (and
hence $\varphi$ minimized over $y \ge 0$), the proof applies verbatim, the
dual constraints $\sum_k \lambda_k (d - G x^k) = 0$ becoming $\sum_k
\lambda_k (d - G x^k) \ge 0$, i.e., $G x \le d$ in the convexified
relaxation; a minimization is the maximization of the opposite objective.

In words, the convexified solution is optimal for the convexified
relaxation, and this is the precise sense in which it is "better" than
any single inner solution, none of which needs to satisfy $G x = d$. It
is not, in general, feasible for $(O)$, and the gap between
$\min_y \varphi$ and $v(O)$ is the price of the convexification.

*The running example.* CFL in the knapsack formulation ([Chapter
12](12-sub-block.md#ch-12)) is a `Block` of this shape: a master carrying only
the linking constraints $\sum_i X_{ij} = 1$, over one `BinaryKnapsackBlock`
sub-`Block` per facility. Attaching a `LagrangianDualSolver` to it dualizes
the linking constraints, leaving one Lagrangian knapsack per facility, each a
`LagBFunction` over a `BinaryKnapsackBlock` solved by a
`CoreDPBinaryKnapsackSolver`. By [Theorem
14.1](14-lag-benders-bfunction.md#thm-14-1) its bound is that of the
formulation with each knapsack replaced by its convex hull, which is at least
as strong as the strong continuous relaxation of CFL. [Recipe
R4](R4-cfl-lagrangian.md#rec-R4) develops this end to end, and shows how
`PrimalProximalHeur` turns the convexified primal solution into a feasible
one.

## 14.3 `BendersBFunction`: the value function of a `Block` {#sec-14-3}

`BendersBFunction` represents the *value function* of its inner `Block` $(B)$
as a function of outer `Variable`s that enter the sides of the constraints of
$(B)$
([`BendersBFunction.h:65-143`](https://gitlab.com/smspp/smspp/-/blob/develop/include/BendersBFunction.h#L65-143)).
The inner `Block` has the form

$$
(B) \quad \min \{\, c(y) : l \le E(y) \le u ,\; y \in Y \,\} ,
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

Note that the roles of $x$ and $y$ are exchanged with respect to
[§14.2](14-lag-benders-bfunction.md#sec-14-2): here, as in
[§13.2](13-function-family.md#sec-13-2), the $x$ are the outer `Variable`s,
and the $y$ those of the inner `Block`. The dependence on $x$ is given by a
matrix $A$, a vector $b$ and a vector of pairs $(C_i, S_i)$, each made of a
`RowConstraint` $C_i$ of $(B)$ and a side $S_i$ (`eLHS`, `eRHS` or `eBoth`):
the mapping $M(x) = A x + b$ gives, in its $i$-th component, the value that
the side $S_i$ of $C_i$ takes, which is how $g - F x$ and $h - F x$ are
written. The outer `Variable`s $x$ are the active `Variable`s of the
`BendersBFunction`, the $j$-th corresponding to the $j$-th column of $A$; no
pair may appear twice, and a constraint with `eBoth` may not appear again.
Evaluating $\varphi(x)$ is one solve of $(B)$ with its sides set to $M(x)$.
The dual solution of $(B)$ gives the linearization (a Benders *optimality
cut*), and when $(B)$ is infeasible at $x$ an unbounded dual direction (a
Farkas certificate) gives a *vertical* linearization (a Benders *feasibility
cut*). Both are valid by linear programming duality, which we state for the
linear case, in which $c(y) = c y$, $E(y) = E y$ and $Y$ is folded into the
rows of $E$ with sides that do not depend on $x$.

<span id="lem-14-3"></span>**Lemma 14.3.** *Let $\varphi(x) = \min \{ c y :
l(x) \le E y \le u(x) \}$, with $l(x)$ and $u(x)$ affine in $x$ (some
components possibly infinite and constant). (i) For every $(\lambda, \mu)
\ge 0$ with $(\lambda - \mu) E = c$ and $\lambda_k = 0$ ($\mu_k = 0$)
wherever $l_k = -\infty$ ($u_k = +\infty$), the affine function $L(x) =
\lambda l(x) - \mu u(x)$ satisfies $L(x) \le \varphi(x)$ for all $x$, with
equality at $\bar x$ if $(\lambda, \mu)$ is dual optimal there and
$\varphi(\bar x)$ is finite. Hence $\varphi$ is convex on its domain. (ii)
For every $(\lambda, \mu) \ge 0$ with $(\lambda - \mu) E = 0$ and the same
zeros, every $x$ at which $(B)$ is feasible satisfies $\lambda l(x) - \mu
u(x) \le 0$. If $\lambda l(\bar x) - \mu u(\bar x) > 0$, the inequality is
violated at $\bar x$ and cuts it off.*

*Proof.* (i) If $y$ is feasible at $x$, then $\lambda l(x) \le \lambda E y$
and $\mu u(x) \ge \mu E y$ (the terms with infinite sides having zero
multiplier), hence $L(x) \le (\lambda - \mu) E y = c y$. Taking the infimum
over the feasible $y$ gives $L(x) \le \varphi(x)$, which is trivially true
where $(B)$ is infeasible ($\varphi = +\infty$). At $\bar x$, if $(\lambda,
\mu)$ is dual optimal, strong duality of linear programming gives $L(\bar x) =
\varphi(\bar x)$. Convexity follows as in [Lemma
14.1](14-lag-benders-bfunction.md#lem-14-1), $\varphi$ being the supremum of
the affine $L$ over the dual feasible set, by strong duality wherever it is
finite. (ii) The same chain gives $\lambda l(x) - \mu u(x) \le (\lambda - \mu)
E y = 0$ for every feasible $y$. $\square$

Since $l$ and $u$ are affine in $x$ through $M(x) = A x + b$, the
linearization of (i) is $g x + \alpha$ with $g$ and $\alpha$ obtained
from $(\lambda, \mu)$, $A$ and $b$; the
`BendersBFunction` stores them in its pools. For a nonlinear convex $(B)$ the
same holds with Lagrangian duality in place of linear duality, under a
constraint qualification.

### The phase one and the global lower bound

A feasibility cut needs an unbounded dual direction, which the `:Solver` of
$(B)$ may not give; e.g., a `:Solver` of convex QPs usually does not. For this
case, `set_phase_one()` (since SMS++ 0.9.0) gives the `BendersBFunction` a
second `BendersBFunction`, its *phase one*, with the same active `Variable`s
in the same order
([`BendersBFunction.h:1964-1997`](https://gitlab.com/smspp/smspp/-/blob/develop/include/BendersBFunction.h#L1964-1997)).
Its inner `Block` is a copy of $(B)$ (cf. `AbstractBlock::mirror()`,
[§10.4](10-r3block.md#sec-10-4)) in which the `Constraint`s that the mapping
writes into are relaxed by nonnegative slacks, and whose `Objective` minimizes
the weighted sum of the slacks. When $(B)$ is infeasible at $x$, its `:Solver`
gives no unbounded dual direction, and no `Constraint` has its left-hand side
above its right-hand side, the phase one is computed. Its optimal dual
solution, written in the `Constraint`s of $(B)$ of which those of the copy are
the copies (0 in the others), is a dual direction of $(B)$, since the
`Objective` of the copy is zero on the `Variable`s of $(B)$, and the vertical
linearization is that of the phase one, scaled by the ratio of the weights of
the two inner `Block`s. This is valid by the following statement, in which we
write $\psi$ for the value function of the phase one.

<span id="lem-14-4"></span>**Lemma 14.4.** *Let $\psi(x)$ be the least
weighted violation, with nonnegative weights, of the `Constraint`s of a copy
of $(B)$ from which some `Constraint`s may have been left out, and let $L(x) =
g x + \alpha$ satisfy $L \le \psi$ everywhere. Then $L(x) \le 0$ at every $x$
at which $(B)$ is feasible; hence, if $L(\bar x) > 0$, the cut $L(x) \le 0$
cuts off $\bar x$ and no feasible point.*

*Proof.* If $(B)$ is feasible at $x$, a feasible point of $(B)$ with all the
slacks at zero is feasible for the copy, also when some `Constraint`s have
been left out, and it has value 0; since $\psi \ge 0$, $\psi(x) = 0$, and
$L(x) \le \psi(x) = 0$. $\square$

By [Lemma 14.3](14-lag-benders-bfunction.md#lem-14-3) (i), applied to the
phase one (which is feasible for every $x$), its linearization at the current
point satisfies the hypothesis of [Lemma
14.4](14-lag-benders-bfunction.md#lem-14-4). Also, when the `:Solver` of $(B)$
returns `kUnbounded` (as a `:Solver` of convex QPs may do for an inner `Block`
that is infeasible by less than its tolerance) and the linearization of the
phase one is positive at the current point, `compute()` returns `kInfeasible`,
the value is $+\infty$, and the linearization is the vertical one of the phase
one. The phase one is used only for an inner `Block` that minimizes, and it is
not owned by the `BendersBFunction`. Furthermore, `set_global_lower_bound()`
(since SMS++ 0.9.0) gives a value below which the `BendersBFunction` is known
not to go (e.g., 0 when the `Objective` of $(B)$ is a sum of squares), which
is not checked and is returned by `get_global_lower_bound()`; a bundle method
uses it to keep its cutting-plane master bounded.

### Reoptimization

Like `LagBFunction`, `BendersBFunction` has no `Variable` or `Constraint`
beyond those of $(B)$, evaluates by a generic `:Solver` registered to $(B)$,
and maintains the linearization pools. It also maps the changes of $(B)$ into
the `FunctionMod*` types of [§13.3](13-function-family.md#sec-13-3). The dual
solutions in its global pool are what the translation has to preserve, and the
cases are distinguished accordingly (`BendersBFunction::add_Modification()`).
A change that only moves the sides of some `Constraint` of $(B)$ (including
relaxing or enforcing one) keeps all of them dual feasible, since dual
feasibility in [Lemma 14.3](14-lag-benders-bfunction.md#lem-14-3) (i) does not
involve the sides, and therefore only the constants of the linearizations are
computed again. The value moves with the feasible region when this moves one
way only, by the monotonicity of (14.2) in its feasible set: if a change turns
the feasible region $Y(x)$ of $(B)$ into $Y'(x) \supseteq Y(x)$ for every $x$,
and leaves the `Objective` unchanged, then the new function satisfies
$\varphi' \le \varphi$ everywhere, since each $y$ feasible before is still
feasible, and symmetrically $Y'(x) \subseteq Y(x)$ gives $\varphi' \ge
\varphi$. Accordingly, `sides_changed()` issues the `C05FunctionMod` with
`shift()` $-\infty$ or $+\infty$ (exchanged if $(B)$ maximizes) when the
`Modification` declares that the region may only grow or only shrink, and NaN
otherwise
([`BendersBFunction.cpp:1896-1926`](https://gitlab.com/smspp/smspp/-/blob/develop/src/BendersBFunction.cpp#L1896-1926)).

Removing `Variable`s from $(B)$ also keeps the pool (a removed column only
drops one of the equations of dual feasibility), the value changing
unpredictably. Adding a `Constraint` keeps the pool (its multiplier being 0),
and removing one keeps the entries in which its multiplier is zero. Any other
change of the model of $(B)$ may take away the feasibility of the dual
solutions, and it is reported as the "nuclear" `FunctionMod`; the mapping is
therefore coarser than that of `LagBFunction` for changes of the
coefficients and of the `Objective` of $(B)$.

*The running example.* CFL in the Benders formulation
([§3.5](03-mental-model.md#sec-3-5)) is built on this. The master carries
the design `Variable`s $Y$ and a single epigraphic `Variable` $v$, and the
transportation subproblem is a hidden `MCFBlock` (the flow relaxation of
[§10.5](10-r3block.md#sec-10-5), with the facility arcs at cost 0) wrapped
in a `BendersBFunction`. At each design point $\hat Y$ at which the master
`:MILPSolver` asks the CFL `Block` for cuts (by its separation loop or by
its lazy-constraint callback), the `BendersBFunction` is evaluated (one MCF
solve), and it gives either an optimality cut, from the dual, or a
feasibility cut, from the Farkas ray in the feasibility-cuts variant. [Recipe R5](R5-cfl-benders.md#rec-R5)
develops this end to end.

`BendersBFunction` is also the component that produces the cuts in an
`SDDPBlock`, where the Benders cuts generated at each stage are
accumulated into a `PolyhedralFunction`
([§13.5](13-function-family.md#sec-13-5)) representing the
approximation of the recourse cost of the following stage. The enclosing
Benders master of a generic `Block` is built by the
`BendersDecompositionSolver` of [§14.6](14-lag-benders-bfunction.md#sec-14-6),
in the way in which `LagrangianDualSolver` builds the Lagrangian one.

## 14.4 Why "almost any `Block`, almost any `Solver`" {#sec-14-4}

The inner `Block` $(B)$ of the two hybrids is not constrained to be of any
particular type, and neither is the `:Solver` that evaluates it. A
`LagBFunction` can wrap a `BinaryKnapsackBlock` solved by dynamic programming,
a `UCBlock` solved by a `:MILPSolver`, or a `Block` that is in turn decomposed
and solved by a nested `LagrangianDualSolver`; a `BendersBFunction` can wrap
an `MCFBlock` solved by a network simplex, or a far more complex subproblem
solved by whatever fits. Hence, SMS++ can express multi-level, heterogeneous,
nested decompositions (Benders outside, SDDP in the middle, Lagrange inside,
dynamic programming at the leaves) by composing these components, each
indifferent to what the others are. We remark that the guarantees of Lemmas
[14.1](14-lag-benders-bfunction.md#lem-14-1) and
[14.3](14-lag-benders-bfunction.md#lem-14-3) hold only as far as the inner
`:Solver` returns optimal (or $\varepsilon$-optimal) primal and dual
solutions: an inner `:Solver` that only returns feasible ones gives valid
bounds in the direction of [Lemma 14.1](14-lag-benders-bfunction.md#lem-14-1)
but not, in general, valid Benders cuts.

## 14.5 Idioms {#sec-14-5}

*A `LagrangianDualSolver` instead of a hand-built `LagBFunction`.* For
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

*A bound and a convexified or fractional solution, not the integer optimum.* A
dual built on `LagBFunction`s gives a bound and a convexified primal solution
([Theorem 14.1](14-lag-benders-bfunction.md#thm-14-1)), and a master built on
`BendersBFunction`s gives a bound and the solution of the master. Turning
either into a feasible integer solution is the job of a subsequent heuristic
(`PrimalProximalHeur`, rounding, branching), not of the hybrid.

*A phase one for a `:Solver` that gives no rays.* When the inner `:Solver` of
a `BendersBFunction` gives no unbounded dual direction, as a `:Solver` of
convex QPs usually does not, a phase one
([§14.3](14-lag-benders-bfunction.md#sec-14-3)) keeps the feasibility cuts
available. A subproblem that is always feasible (e.g., because the rows that
$x$ can make unsatisfiable carry a slack with a large cost) needs none.

## 14.6 `BendersDecompositionSolver` {#sec-14-6}

The `BendersDecompositionSolver` is a `CDASolver` that, attached to a suitably
structured `Block` $(B)$, builds and solves its Benders decomposition
([`BendersDecompositionSolver.h:119-310`](https://gitlab.com/smspp/bendersdecompositionsolver/-/blob/develop/include/BendersDecompositionSolver.h#L119-310)).
The `Block` is assumed to encode

$$
(O) \quad \min \Big\{\, d(x) + \sum_{k \in K} c^k(y^k) :
x \in X ,\; (x, y^k) \in Z^k ,\; k \in K \,\Big\}
\tag{14.3}
$$

(or the corresponding maximization), where $x$ are the *complicating*
`Variable`s, $d$ and $X$ are the `Objective` and the `Constraint`s of the
*master*, and each subproblem $k \in K$ has its `Variable`s $y^k$, its
`Objective` $c^k$ and its `Constraint`s, coupled to the master only through
rows of the form $l^k \le F^k x + E^k(y^k) \le u^k$, i.e., affine in $x$. A
subproblem in which no $x$ appears is allowed, its value function being then
a constant. Projecting out the $y^k$ gives

$$
(O') \quad \min \Big\{\, d(x) + \sum_{k \in K} v^k(x) : x \in X \,\Big\} ,
\qquad
v^k(x) = \min \{\, c^k(y^k) : l^k - F^k x \le E^k(y^k) \le u^k - F^k x \,\} ,
\tag{14.4}
$$

and each $v^k$ is the function (14.2) of a `BendersBFunction`, whose active
`Variable`s are the $x$. The cuts are therefore not generated by the
`BendersDecompositionSolver`, but by the `BendersBFunction`s through the
standard interface of `C05Function`; the `BendersDecompositionSolver` is the
layer that automates the decomposition.

*The reformulation.* Which sub-`Block`s are in the master and which
`Variable`s are complicating cannot be read off a `Block`, since the same
`Block` admits many Benders reformulations. The vector-of-int parameters
`vintMasterBlock` and `vintMasterVars` give the positions of the sub-`Block`s
that belong to the master (the subproblems being the others) and the positions
of the complicating `Variable`s among those of the master. Both are empty by
default, which means that the root is the master, that each sub-`Block` of it
is a subproblem, and that each `ColVariable` of the root is complicating.
These defaults are used by the test `test_bds.cpp` of
`TwoStageStochasticBlock` and by the option `-k` of `smspp_tssb_solver`, both
on the form that a `TwoStageStochasticBlock` gives through
`get_Benders_form()`, while the test of `AbstractBlock` sets them
(`BSPar-BDS-Keep.txt`). The `:Solver` then finds, in each subproblem, the rows
that are linear in $x$, takes the term $F^k x$ out of them (recording $F^k$),
and builds one `BendersBFunction` per subproblem. The master is assembled
differently in the two regimes, since they add different things to $(B)$. In
the MILP regime the master is a `Block` of the `:Solver`, holding the epigraph
`Variable`s and the cuts, in which $(B)$ is grafted as the only sub-`Block`,
and any `Block` can be the root. In the convex regime the value functions are
carried by sub-`Block`s of the `Block` that holds the $x$, i.e., $(B)$ if it
is an `AbstractBlock`, and otherwise the copy of its abstract representation
([§10.4](10-r3block.md#sec-10-4)). In both regimes the subproblems stay in
$(B)$, inside their `BendersBFunction`s, and they are declared excluded to the
`:Solver` of the master (`Solver::set_excluded_blocks()`).

*The convex regime* (`int_BDSlv_Regime` 0, the default). The `Objective` of
the master is $d(x) + \sum_k v^k(x)$, the $v^k$ entering as `C05Function`s,
and the master is handed to a `CDASolver` of the bundle family
([Chapter 19](19-nondifferentiable.md#ch-19)), which runs the (stabilized)
cutting-plane method by asking the `BendersBFunction`s for linearizations
([van Ackooij et al. 2016](Z-bibliography.md#ref-vanackooij2016)). With
continuous $x$ this solves $(O')$; with integer $x$, it solves either its
continuous relaxation or, with a `BundleSolver` that keeps them integer, the
stabilized decomposition over the integer $x$.

*The MILP regime* (`int_BDSlv_Regime` 1). The master is a MILP with one
epigraph `Variable` $\eta^k$ per subproblem (`int_BDSlv_CutType` 0, the
multi-cut default) or a single one for their sum (`int_BDSlv_CutType` 1),
solved by a `:MILPSolver`, and the cuts are added to it as dynamic
`Constraint`s by the following loop
([Baena et al. 2020](Z-bibliography.md#ref-baena2020)). On continuous $x$,
with a master that is an LP or a convex QP, it is the cutting-plane method
([Kelley 1960](Z-bibliography.md#ref-kelley1960)). In it, $f(x) = d(x) +
\sum_k v^k(x)$ is the objective of $(O')$, and the box $\| x - c \|_\infty
\le \delta$ around a center $c$ is the *trust region*, used only if
`dblTRRadius` is positive.

<span id="alg-14-1"></span>**Algorithm 14.1** (the cut loop of the MILP
regime, with the trust region).

```text
 1  procedure BENDERS-MILP(x⁰)
 2     c ← x⁰;  δ ← dblTRRadius;  box ← (δ > 0)
 3     if box then evaluate each v^k at c, add its cut;  f_c ← f(c)
 4     repeat
 5        solve the master, with ‖x − c‖_∞ ≤ δ if box
 6        (x̂, η̂) ← its solution;  v ← its value
 7        evaluate each v^k at x̂;  add its feasibility cut if v^k(x̂) = +∞,
          and, unless (x̂, η̂) is that of the previous round, its optimality
          cut if this is violated at (x̂, η̂) by more than
          dblCutTol · max{1, |value of the cut at x̂|}
 8        serious ← box and f(x̂) < +∞
                  and (f_c = +∞ or f(x̂) ≤ f_c − 0.1 (f_c − v))
 9        if serious then
10           if the box is binding at x̂ then δ ← 2 δ
11           c ← x̂;  f_c ← f(x̂)
12        if no cut has been added then
13           if not box then return x̂
14           if the box is not binding at x̂ or not serious then box ← false
15     until int_BDSlv_MaxRounds rounds, or dblMaxTime, have been used
```

This is `solve_MILP_master()` in
[`BendersDecompositionSolver.cpp`](https://gitlab.com/smspp/bendersdecompositionsolver/-/blob/develop/src/BendersDecompositionSolver.cpp).
At line 7, an infeasible subproblem gives a feasibility cut, as discussed
below. The value $v$ of a master with the box is not a lower bound, and the
lower bound is $-\infty$ until the box is taken away (line 14, or when the box
leaves out all the master). Without the box, $v$ is the lower bound. At the
end, the upper bound is the value of the master plus how much the value
functions exceed their epigraph `Variable`s at its solution, which is
$f(\hat x)$ when the master is solved exactly; a loop stopped by a budget
reports none (the log prints the best $f(\hat x)$ over the rounds in which
every subproblem has a value). A master that returns the solution of the
previous round gets no optimality cut at line 7, and, if every subproblem is
feasible there, that round is the last one. On
continuous $x$ the box keeps the cuts where the solution is while few cuts
bound the $x$ (e.g., unbounded $x$ in the first rounds), and with integer $x$
it is a heuristic phase, after which the loop is the ordinary one. A
subproblem solved by a `:Solver` that is not exact (e.g., a Lagrangian dual)
gives a value above its cut, and then the loop stops with the master value as
lower bound and the values at the incumbent as upper one, returning
`kLowPrecision` if they are farther apart than `dblRelAcc` allows. A
subproblem stopped by its own limits of time or of iterations (since SMS++
0.9.0) gives no cut in that round, which is then `kLowPrecision`, and the loop
goes on with the cuts of the others. The parameter `dblMaxTime` bounds the
whole `compute()`: the master and each subproblem are given what is left of
it, or their own limit if shorter, the loop returns `kStopTime` when it is
over, and the recovery `:Solver` described below is also given what is left.
The subproblems of a round are evaluated, and their `:Solver`s attached, in
`intMaxThread` threads (1 by default, i.e., sequentially).

*The feasibility cuts.* An infeasible subproblem proves that $\hat x$ is not
feasible for $(O)$ either, and `int_BDSlv_FeasCut` chooses how it is cut away.
With 0 the subproblems are declared never infeasible, which is the case
whenever the rows that the master can make unsatisfiable carry a slack with a
large enough cost, and which requires nothing of the `:Solver` of the
subproblems. With 1, the default, the cut is the Farkas certificate of [Lemma
14.3](14-lag-benders-bfunction.md#lem-14-3) (ii), which the `:Solver` of the
subproblem has to produce. With 2, it is a combinatorial (no-good) cut, which
only forbids the current assignment of the complicating `Variable`s, and it
needs all of them to be binary ([Codato and Fischetti
2006](Z-bibliography.md#ref-codato2006)); it requires nothing of the
`:Solver`, but it is much weaker, and it is rejected in the convex regime,
where it is not a linearization. With 3, it is the cut of the phase one of a
replica of the subproblem, as in [Lemma
14.4](14-lag-benders-bfunction.md#lem-14-4), made by `AbstractBlock::mirror()`
and solved by the `BlockSolverConfig` in `strPhaseOneBSC` (since SMS++ 0.9.0;
that of the subproblems if empty); the Farkas certificate is asked for only if
this cut does not cut off $\hat x$. The multipliers of the phase one are
bounded by the costs of its slacks, which are therefore the normalization that
selects the cut ([Fischetti et al.
2010](Z-bibliography.md#ref-fischetti2010)), and `int_BDSlv_PhaseOneWeights`
chooses them: 1 for each slack (0, the default), the inverse of the 2-norm of
the coefficients of its row (1), which makes the cut independent of the
scaling of the rows, or a single slack for all the rows, weighted by the row
sums of the mapping (2). Also, `int_BDSlv_CutNorm` scales a feasibility cut by
the 1-norm (1), the 2-norm (2) or the largest (3) of its coefficients, or not
at all (0, the default).

We remark that the presolve of the `:Solver` of the subproblems has to be
switched off if Farkas cuts are wanted, since a presolve that detects the
infeasibility on the reduced problem returns no certificate for the
original one; the `BlockSolverConfig` `BDSSCfg.txt` of the CFL tests does
so. A missing certificate is reported by an exception, instead of letting
the loop converge to a wrong optimum. The optimality cuts, instead, come
from any optimal dual solution, basic or not.

*Stronger cuts.* A subproblem is often dual degenerate, and then each of its
optimal dual solutions gives a valid cut, some of which dominate the others on
the feasible set of the master. A cut that no other dominates is
*Pareto-optimal* ([Magnanti and Wong
1981](Z-bibliography.md#ref-magnanti1981)), and `int_BDSlv_Pareto` 1 adds one
per round in the one-step form of [Papadakos
2008](Z-bibliography.md#ref-papadakos2008), i.e., by evaluating the value
function at a *core point* $x^\circ$. The core point starts where the $x$ are
when the loop starts, and after each round that adds cuts it moves towards the
incumbent, $x^\circ \leftarrow x^\circ + \beta (\hat x - x^\circ)$, with
$\beta$ = `dbl_BDSlv_CoreMove` (0.5 by default). The two questions that the
classical decomposition poses to a subproblem (is it feasible, and what is its
value function) can also be answered together, by one cut-generating problem
whose cut is a feasibility cut or an optimality cut according to the solution
([Fischetti et al. 2010](Z-bibliography.md#ref-fischetti2010)). With
`int_BDSlv_Unified` 1 this problem is the phase one with the epigraph row
$c^k(y^k) \le \eta^k$ added, whose slack costs `dbl_BDSlv_EpiWeight` (1 by
default). Since the multipliers are bounded by the costs, the ratio of this
weight to those of the coupling rows decides how strong the cut is, and on a
model whose `Objective` and coupling rows are measured in different units it
has to be set accordingly. With `int_BDSlv_Unified` 2, the *deepest* cut, the
normalization bounds the coefficients of the cut instead of the multipliers,
and the cut is the one whose $\ell_1$ distance from $(\hat x, \hat \eta)$ is
largest, with no weight to tune. Both require the MILP regime and the
multi-cut aggregation. With either of them, `int_BDSlv_Pareto` 2 (which is
rejected otherwise) separates the unified cut once per round at the incumbent
moved towards the core point by the step `dbl_BDSlv_ParetoMu` (0.1 by
default), which gives a nondominated cut with one problem per subproblem
([Sherali and Lunday 2013](Z-bibliography.md#ref-sherali2013)).

*Giving the `Block` back.* The reformulation is a change of $(B)$, since the
$x$ are taken out of the `Constraint`s of the subproblems and, in the MILP
regime, $(B)$ is grafted into the master; while it lasts, another `Solver`
attached to $(B)$ sees a different problem. With `int_BDSlv_Restore` 1, the
reformulation is made at the beginning of each `compute()` and undone at its
end, the solution being written in the `Variable`s of $(B)$ before. This is
needed when the `BendersDecompositionSolver` is registered beside other
`Solver`s, as in a cross-check, and it costs the reformulation at each
`compute()` and the cuts, which are not kept. With 0, the default, the
reformulation stays until the `:Solver` is unregistered.

<span id="tab-14-1"></span>**Table 14.1.** The parameters of
`BendersDecompositionSolver` and their defaults.

| Parameter | Default | Meaning |
|---|---|---|
| `int_BDSlv_Regime` | 0 | 0 convex master, 1 MILP master |
| `int_BDSlv_CutType` | 0 | 0 multi-cut, 1 single-cut (MILP regime) |
| `int_BDSlv_FeasCut` | 1 | 0 always feasible, 1 Farkas, 2 combinatorial, 3 phase one |
| `int_BDSlv_MaxRounds` | $+\infty$ | rounds of the cut loop of the MILP regime |
| `int_BDSlv_Pareto` | 0 | 0 none, 1 core point, 2 perturbed incumbent |
| `int_BDSlv_CutNorm` | 0 | scaling of the feasibility cuts |
| `int_BDSlv_PhaseOneWeights` | 0 | 0 unit, 1 row norms, 2 row sums, one slack |
| `int_BDSlv_Unified` | 0 | 0 separate cuts, 1 unified, 2 deepest |
| `int_BDSlv_Restore` | 0 | 1 gives $(B)$ back after each `compute()` |
| `int_BDSlv_iBCopy` | 0 | reserved: any other value throws |
| `intMaxThread` | 1 (0 in `ThinComputeInterface`) | threads for the subproblems of a round, for the attachment of their `:Solver`s and for the recovery |
| `dblTRRadius` | 0 | initial radius of the trust region, 0 for none |
| `dblCutTol` | $10^{-9}$ | relative violation for a cut to be added |
| `dblSubLB` | $-\infty$ | global lower bound of each $v^k$ (convex regime; since SMS++ 0.9.0) |
| `dbl_BDSlv_CoreMove` | 0.5 | step of the core point towards the incumbent |
| `dbl_BDSlv_ParetoMu` | 0.1 | step of the perturbed incumbent |
| `dbl_BDSlv_EpiWeight` | 1 | cost of the slack of the epigraph row |
| `vintMasterBlock`, `vintMasterVars` | empty | the master and the complicating `Variable`s |
| `str_BDSlv_MSName` | empty | reserved: read by nothing |
| `str_Mstr_BSCfg` | empty | `BlockSolverConfig` of the master, whose first `:Solver`, a `CDASolver`, is the master `:Solver`; required (empty throws at the first `compute()`) |
| `str_Bsub_BSCfg` | empty | `BlockSolverConfig` of the subproblems |
| `strPhaseOneBSC` | empty | `BlockSolverConfig` of the replicas of the phase one |
| `strRecoveryBSC` | empty | `BlockSolverConfig` of the recovery of a solution |

The parameter `dblSubLB` is passed to each `BendersBFunction` by
`set_global_lower_bound()`, and `strRecoveryBSC`, if not empty, makes the
`:Solver` solve each subproblem once more at the end of `compute()`, at the
design of the master, with the `Solver` that the file attaches (e.g., a
`:MILPSolver`): the upper bound then becomes the value of a feasible solution,
also when the subproblems have been solved by a `:Solver` whose values are
only bounds. Two limitations are stated in the header: the coupling of each
subproblem with $x$ must be affine, and the subproblems must be convex
(typically LPs), integer subproblems not being supported. Finally, the
arrangement of [Recipe R5](R5-cfl-benders.md#rec-R5), in which the cuts are
emitted on the request of the master `:MILPSolver` (by its separation loop or
by its lazy-constraint callback) through `generate_dynamic_constraints()`,
remains available; it may be the more natural one when the cuts are to be seen
by the branch-and-cut of the master while it proceeds. The test `CFL_BDS_test`
compares the two on the same instance, in both regimes and with and without
the Pareto-optimal cuts, with the `BlockSolverConfig`s `BSPar-BDS.txt`,
`BSPar-BDS-MILP.txt` and `BSPar-BDS-Pareto.txt`. For the two runs of the MILP
regime it prints the number of rounds (`get_elapsed_iterations()`, which in
the convex regime counts the iterations of the master `:Solver` instead) and
that of the cuts added to the master (`get_num_cuts()`).

## 14.7 Notes and references {#sec-14-7}

Lagrangian relaxation for integer programming, and the equality of the
Lagrangian dual with the convexified relaxation of [Theorem
14.1](14-lag-benders-bfunction.md#thm-14-1), are due to [Geoffrion
1974](Z-bibliography.md#ref-geoffrion1974), and the use of Lagrangian methods
in integer optimization is surveyed in [Frangioni
2005](Z-bibliography.md#ref-frangioni2005). The decomposition of
[§14.3](14-lag-benders-bfunction.md#sec-14-3) and
[§14.6](14-lag-benders-bfunction.md#sec-14-6) is that of [Benders
1962](Z-bibliography.md#ref-benders1962), and the master of the MILP regime
without stabilization is the cutting-plane method of [Kelley
1960](Z-bibliography.md#ref-kelley1960). The stabilized (and possibly inexact)
decomposition with a bundle-type master of the convex regime follows [van
Ackooij et al. 2016](Z-bibliography.md#ref-vanackooij2016), and the stabilized
combinatorial master of the MILP regime follows [Baena et al.
2020](Z-bibliography.md#ref-baena2020); the bundle methods themselves are
those of [Frangioni 2020](Z-bibliography.md#ref-frangioni2020) ([Chapter
19](19-nondifferentiable.md#ch-19)). The Pareto-optimal cuts are those of
[Magnanti and Wong 1981](Z-bibliography.md#ref-magnanti1981), in the one-step
form of [Papadakos 2008](Z-bibliography.md#ref-papadakos2008), and the
nondominated cut obtained at a perturbed point is that of [Sherali and Lunday
2013](Z-bibliography.md#ref-sherali2013). The combinatorial cuts are those of
[Codato and Fischetti 2006](Z-bibliography.md#ref-codato2006), while the role
of the normalization in the selection of a feasibility cut, and the separation
of feasibility and optimality cuts by one problem, are discussed in [Fischetti
et al. 2010](Z-bibliography.md#ref-fischetti2010).
