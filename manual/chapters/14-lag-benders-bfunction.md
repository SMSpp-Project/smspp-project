# 14. LagBFunction and BendersBFunction {#ch-14}

[Source: `SMS++/include/LagBFunction.h`, `BendersBFunction.h`,
`LagrangianDualSolver/include/LagrangianDualSolver.h`;
`CapacitatedFacilityLocationBlock/include/CapacitatedFacilityLocationBlock.h`]

Two members of the `Function` family are special enough to deserve
their own chapter. Each is *simultaneously* a `Block` and a
`C05Function`: it wraps a single inner `Block` and presents that
inner `Block`, evaluated, as a first-order function of some outer
`Variable`s. They are the components that turn the abstract idea
"the value of a function is the optimum of a sub-problem" ([§13.2](13-function-family.md#sec-13-2))
into reusable building blocks, and they are the engines behind
Lagrangian decomposition ([Recipe R4](R4-cfl-lagrangian.md#rec-R4)) and Benders decomposition
([Recipe R5](R5-cfl-benders.md#rec-R5)).

## 14.1 Two hybrids, one pattern {#sec-14-1}

[`LagBFunction`](https://smspp.gitlab.io/smspp-project/d2/df7/class_s_m_spp__di__unipi__it_1_1_lag_b_function.html)
and
[`BendersBFunction`](https://smspp.gitlab.io/smspp-project/dd/d28/class_s_m_spp__di__unipi__it_1_1_benders_b_function.html)
both derive from *both* `Block` and `C05Function`. As a `Block`,
each holds exactly one sub-`Block`, the "base" `Block` $(B)$ that
represents the sub-problem. As a `C05Function`, each is a
first-order function of a set of *active* `Variable`s — call them
the *outer* `Variable`s — that are **not** `Variable`s of the
hybrid `Block` itself (they belong to whatever outer `Block`
defines them).

The pattern both follow is the same:

- to **evaluate** the function at a given value of the outer
  `Variable`s, the hybrid sets up $(B)$ accordingly and calls
  `compute()` on a generic `:Solver` attached to $(B)$ — "almost
  just calling the inner `Solver`'s `compute()`";
- the **linearization** of the function at that point is read off
  the inner `Block`'s primal and/or dual solution, exactly as in
  the Lagrangian example of [§13.2](13-function-family.md#sec-13-2);
- the hybrid maintains **pools of primal / dual `Solution`s** of
  $(B)$, one per linearization, so that the surrounding
  cutting-plane / bundle / Benders method can reoptimize;
- any **change in $(B)$** is translated into the corresponding
  change of function value and pool entries, and surfaced as a
  `C05FunctionMod` ([Chapter 8](08-modification-janus.md#ch-8), [Chapter 13](13-function-family.md#ch-13)) to whoever is solving
  the outer problem.

Because the inner `Block` may be "almost any" `Block`, and is
solved by "almost any" `:Solver`, these two classes give a generic
way to build the two most important decomposition functions over
*arbitrary* structured sub-problems.

## 14.2 `LagBFunction`: the Lagrangian function of a `Block` {#sec-14-2}

`LagBFunction` represents the Lagrangian function of its inner
`Block` $(B)$ with respect to a chosen set of *linear* terms.
Concretely, given an inner problem

$$(B) \quad \max \lbrace f(x) \mid x \in X \rbrace$$

and a vector of pairs $\langle y_i, g_i(x)\rangle$ — each $g_i$ a
linear form in the same `Variable`s $x$ as $(B)$, each $y_i$ a
Lagrangian multiplier — `LagBFunction` is the function

$$\varphi(y) = \max \lbrace f(x) + \sum_i y_i g_i(x) \mid x \in X \rbrace$$

The $g_i$ are meant to be (a part of) the *complicating
constraints* of some original problem $(O)$, relaxed in a
Lagrangian fashion; the multipliers $y_i$ are the outer
`Variable`s of the `LagBFunction` — `ColVariable`s that the
`LagBFunction` does *not* own (they are "conceptually fixed" while
$(B)$ is solved). $\varphi$ is convex in $y$ (concave if $(B)$ is
a maximisation, as written), being a pointwise optimum of affine
functions of $y$; at a point $y$, an (approximately) optimal inner
solution $x(y)$ gives the (sub/super)gradient $g(x(y)) = [g_i(
x(y))]_i$ — the linearization. Hence `LagBFunction` is, in
general, a non-smooth `C05Function`
(`LagBFunction.h:77-147`).

`LagBFunction` automates the whole apparatus: turning $(B)$ into
$\varphi$, evaluating it by calling a `:Solver` on $(B)$,
maintaining the local and global linearization pools, and keeping
$\varphi$ in sync as $(B)$ changes. It is not supposed to have any
`Variable` or `Constraint` of its own beyond those of $(B)$.

### Reoptimization: mapping inner `Modification`s to `FunctionMod*`

`LagBFunction` is where the reoptimization vocabulary of [§13.3](13-function-family.md#sec-13-3)
earns its keep. When the inner `Block` $(B)$ changes — a cost in
its objective, a bound in one of its constraints, a `Variable`
added or removed — a `Modification` is issued by $(B)$, and
`LagBFunction` *translates* it into the corresponding
`FunctionMod*` for $\varphi$: an exact-shift `FunctionMod` when the
change shifts $\varphi$'s value by a known constant, a monotone
(`+`/`−`∞) one when only the direction is known, a
quasi-additive or strongly quasi-additive `FunctionModVars` when
the change adds or removes inner `Variable`s. Because the
translation is faithful, a `BundleSolver` driving the Lagrangian
dual can react to a change in the *inner* problem by reoptimizing
— keeping the parts of its bundle (the global pool) that remain
valid — rather than rebuilding the dual from scratch.
`LagBFunction` uses this mechanism extensively, and it is one of
the concrete pay-offs of SMS++'s focus on reoptimization: a change
deep inside a decomposed model propagates outward as a precisely
typed `FunctionMod`, and the outer `:Solver` does the least work
the change allows.

### The `LagrangianDualSolver`

A user rarely constructs a `LagBFunction` by hand. The usual route
is the
[`LagrangianDualSolver`](https://smspp.gitlab.io/smspp-project/d4/dc3/class_s_m_spp__di__unipi__it_1_1_lagrangian_dual_solver.html),
a `CDASolver` that solves the Lagrangian dual of a "generic"
`Block` $(B)$ with block-diagonal structure: no `Variable`s and no
`Objective` of its own, at least one sub-`Block`, and all of its
`Constraint`s being the *linear linking constraints* between the
sub-`Block`s. Given such a $(B)$, `LagrangianDualSolver`
*stealthily* builds a hidden `LagrangianDualBlock`: for each
sub-`Block` $(B_i)$ of $(B)$ it constructs a `LagBFunction`
wrapping $(B_i)$, dualising the linking constraints, and attaches
the user-chosen inner `:Solver` to drive the dual (a
`BundleSolver`, typically). The original $(B)$ still "believes" the
$(B_i)$ are its own sub-`Block`s; the framework keeps both views
consistent through the `Modification` mechanism
(`LagrangianDualSolver.h:55-130`). The output is a *dual* bound
and a *convexified* primal solution — the convex combination of
inner solutions corresponding to the optimal multipliers, which is
"better" than any single inner solution in the precise sense of
Lagrangian duality.

**The running example.** CFL in the Knapsack Formulation
([Chapter 12](12-sub-block.md#ch-12)) is exactly a `Block` of this shape: a master carrying
only the customer-satisfaction linking constraints, over
`f_n_facilities` `BinaryKnapsackBlock` sub-`Block`s. Attaching a
`LagrangianDualSolver` to it dualises the linking constraints,
leaving one Lagrangian knapsack per facility — each a
`LagBFunction` over a `BinaryKnapsackBlock`, solved by a
`DPBinaryKnapsackSolver`. [Recipe R4](R4-cfl-lagrangian.md#rec-R4) develops this end to end, and
shows how the derived `PrimalProximalHeur` turns the convexified
primal solution into a feasible one.

## 14.3 `BendersBFunction`: the value function of a `Block` {#sec-14-3}

`BendersBFunction` represents the *value function* of its inner
`Block` $(B)$ as a function of outer `Variable`s that enter the
right- and/or left-hand sides of $(B)$'s constraints. Concretely,
$(B)$ has the form

$$(B) \quad \min \lbrace c(y) \mid w \le E(y) \le z, \ y \in Y \rbrace$$

and one is interested in how its optimum varies as $w$ and $z$ are
shifted by an affine function of the outer `Variable`s $x$. The
shift is expressed by a matrix $A$ and a vector $b$: the mapping
$M(x) = A x + b$ gives, component by component, the value that the
left- or right-hand side of a designated `RowConstraint` of $(B)$
must take. The `BendersBFunction` is then

$$\varphi(x) = \min \lbrace c(y) \mid (g - Fx) \le E(y) \le (h - Fx), \ y \in Y \rbrace$$

i.e. the value of $(B)$ once its constraint sides have been set to
$M(x)$. The outer `Variable`s $x$ are the active `Variable`s of
the `BendersBFunction`; each corresponds to one column of $A$
(`BendersBFunction.h:64-141`). Evaluating $\varphi(x)$ is one
solve of $(B)$; the dual solution of $(B)$ gives the
linearization (a *Benders optimality cut*), and when $(B)$ is
infeasible at a given $x$ the Farkas ray gives a *vertical*
linearization (a *Benders feasibility cut*) — both handled by the
`C05Function` linearization machinery.

Like `LagBFunction`, `BendersBFunction` has no `Variable` or
`Constraint` of its own beyond those of $(B)$, evaluates via a
generic `:Solver` on $(B)$, and maintains the linearization pools.

It also maps changes in its inner `Block` $(B)$ into the
`FunctionMod*` types of [§13.3](13-function-family.md#sec-13-3), with the same reoptimization
intent described for `LagBFunction` above. **At version 0.6.0,
however, the `BendersBFunction` translation is more limited than
`LagBFunction`'s**: fewer kinds of inner change are mapped to the
fine-grained, directionally-typed `FunctionMod`s that would let an
outer `:Solver` reoptimize maximally, and more of them fall back
to a coarser "value changed" signal. This is an implementation
gap, not a design one, and it is expected to be narrowed in future
revisions; the underlying mechanism is the same.

**The running example.** CFL in the Benders Formulation
([§3.5](03-mental-model.md#sec-3-5)) is built on exactly this: the master carries the design
`Variable`s $y$ and a single epigraphic `Variable` $v$; the
transportation sub-problem $\varphi(y)$ is a hidden `MCFBlock`
(the very flow relaxation of [Chapter 10](10-r3block.md#ch-10)) wrapped in a
`BendersBFunction`. As the master MILP visits design points $\hat
y$, a user-cut / lazy callback evaluates the `BendersBFunction` —
one MCF solve — and emits either an optimality cut (from the dual)
or a feasibility cut (from the Farkas ray, in the feasibility-cuts
sub-variant). [Recipe R5](R5-cfl-benders.md#rec-R5) develops this end to end.

`BendersBFunction` is also the component that produces the cuts in
an `SDDPBlock`: there, the Benders cuts generated at each stage are
accumulated into a `PolyhedralFunction` ([§13.4](13-function-family.md#sec-13-4)) representing the
recourse-cost approximation of the following stage.

**Status — planned.** A `BendersDecompositionSolver` that would
automate the construction of an enclosing Benders master for any
suitable `Block` — in the way `LagrangianDualSolver` automates the
Lagrangian master — is documented in the project's plans but is
not released at version 0.6.0. The CFL/BenForm recipe of R5 works
today because the cuts are emitted on demand by the master MILP's
user-cut callback driving `generate_dynamic_constraints()`, which
does not depend on that future `:Solver`.

## 14.4 Why "almost any `Block`, almost any `Solver`" {#sec-14-4}

The power of these two hybrids is that the inner `Block` $(B)$ is
not constrained to be of any particular type, and the `:Solver`
that evaluates it is not constrained to be of any particular type
either. A `LagBFunction` can wrap a `BinaryKnapsackBlock` solved
by dynamic programming, a `UCBlock` solved by a `:MILPSolver`, or
a `Block` that is itself decomposed and solved by a nested
`LagrangianDualSolver`. A `BendersBFunction` can wrap an
`MCFBlock` solved by a fast network simplex, or a far more
complex sub-problem solved by whatever fits. This is what lets
SMS++ express *multi-level, heterogeneous, nested* decompositions
— Benders outside, SDDP in the middle, Lagrange inside, dynamic
programming at the leaves — by composing these components, each
indifferent to what the others are.

## 14.5 Idioms {#sec-14-5}

**Do not build a `LagBFunction` by hand if a
`LagrangianDualSolver` will do.** For a `Block` with the
block-diagonal "linking constraints over independent sub-`Block`s"
shape, attach a `LagrangianDualSolver` and let it construct the
`LagBFunction`s and the hidden `LagrangianDualBlock`. Constructing
`LagBFunction`s manually is for cases that fall outside that
shape.

**Configure the inner `Solver`, not just the outer one.** Both
hybrids evaluate their inner `Block` with a `:Solver` that must be
attached and configured. For a `BendersBFunction` whose inner
`Block` is "hidden" (as in CFL/BenForm), the inner `:Solver` is
configured through the `f_extra_Configuration` of the outer
`Block` ([§11.6](11-configuration.md#sec-11-6)); forgetting it is the most common setup mistake.

**Expect a bound and a convexified / fractional solution, not the
integer optimum.** A `LagBFunction`-based dual gives a bound and a
convexified primal solution; a `BendersBFunction`-based master
gives a bound and the master's solution. Turning either into a
feasible integer solution is the job of a subsequent heuristic
(`PrimalProximalHeur`, rounding, branching), not of the hybrid
itself.
