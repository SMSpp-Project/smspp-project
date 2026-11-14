# 13. The Function family {#ch-13}

[Source: `SMS++/include/Function.h`, `C05Function.h`,
`C15Function.h`, `LinearFunction.h`, `DQuadFunction.h`,
`QuadFunction.h`, `PolyhedralFunction.h`, `PolyhedralFunctionBlock.h`,
`C05SumFunction.h`]

`Function`s are the objects that supply the *value* inside an
`FRowConstraint` (its left-hand side) and inside an `FRealObjective`
(its value); they also stand on their own as the mathematical content
of the two hybrids of `Block` and `Function` of
[Chapter 14](14-lag-benders-bfunction.md#ch-14). This chapter
describes the hierarchy and the idea that gives it its algorithmic
power, i.e., the *linearization pools* of `C05Function`, together with
the `Modification`s that let a `:Solver` reuse them.

![Part of the `Function` hierarchy: from the base `Function` through `C05Function` (first-order information) and `C15Function` (second-order information) to some of the leaf classes; `QuadFunction` (derived from `DQuadFunction`) and `C05SumFunction` (derived from `C05Function`) are not drawn.](../figures/Function-doxy.svg)

## 13.1 The base `Function` {#sec-13-1}

A
[`Function`](https://smspp.gitlab.io/smspp-project/d9/dd8/class_s_m_spp__di__unipi__it_1_1_function.html)
is a real-valued function of a set of *active* `Variable`s. Like
`Constraint` and `Objective`, it derives from both
[`ThinVarDepInterface`](https://smspp.gitlab.io/smspp-project/da/df9/class_s_m_spp__di__unipi__it_1_1_thin_var_dep_interface.html)
(it depends on a set of `Variable`s) and `ThinComputeInterface` (it must
be `compute()`-d, possibly at significant cost). Its value type is
`FunctionValue` (a `double`). The base class exposes only *values*:
`compute()` evaluates the function at the current values of its active
`Variable`s, and `get_value()` reads the result, while first- and
second-order information, and any specific algebraic form, are left to
the derived classes.

The base `Function` also *advertises* some mathematical properties that
a `:Solver` can rely on to choose its algorithm, through a family of
predicates (`Function.h:577-634`): `is_convex()` and `is_concave()`
(both `false` by default, and both `false` for a function that is
neither), `is_linear()`, defined as `is_convex() && is_concave()`, and
`is_lower_semicontinuous()`, `is_upper_semicontinuous()` and
`is_continuous()` (true when both of the former hold). A bundle method,
for example, checks `is_convex()` (or `is_concave()`) before treating
the linearizations as subgradients (or supergradients), and a `:Solver`
that requires a continuous objective checks `is_continuous()`. The base
class returns the most conservative answers, and a concrete `:Function`
overrides them to declare the structure it has (e.g.,
`PolyhedralFunction` answers `is_convex()` or `is_concave()` according
to the sense given to its constructor).

Two further facts about the base class are worth keeping in mind. First,
a `Function` reports its `Modification`s to *one* `Observer`
([Chapter 8](08-modification-janus.md#ch-8)), typically the
`FRowConstraint`, `FRealObjective` or `Block` that contains it. A
`Function` can also be evaluated *approximately*: a caller may accept a
lower and an upper estimate of the value (`get_lower_estimate()` and
`get_upper_estimate()`), provided they are accurate enough, which is
what makes inexact bundle and Benders methods possible.

Second, a `Function` does *not* register itself with its active
`Variable`s. A free-floating `Function` has no way of knowing whether
it is a part of some `Constraint` or `Objective`, and therefore the
responsibility of registering with the `Variable`s falls on the
`ThinVarDepInterface` that *uses* the `Function` (the
`FRowConstraint`, the `FRealObjective`, and so on). When a `Variable`
is added to or removed from the `Function`, the `Function` issues a
`FunctionModVars`, which the containing object observes and reacts to
by registering itself with that `Variable`, or unregistering from it
(`Function.h:91-120`). The practical consequence is that `Function`s
are handed to a carrier (`set_function()`,
[§5.3](05-variable-constraint-objective.md#sec-5-3)) that takes care of
this; one does not normally hold a bare `Function` and expect it to
maintain its own dependencies.

## 13.2 `C05Function`: first-order information and linearization pools {#sec-13-2}

[`C05Function`](https://smspp.gitlab.io/smspp-project/dd/d96/class_s_m_spp__di__unipi__it_1_1_c05_function.html)
extends `Function` with *first-order* information, not necessarily
continuous, in the form of *linearizations*. Assuming that the active
`Variable`s are single reals (`ColVariable`s), a linearization is an
affine function $L(x) = g x + \alpha$, defined by a real $n$-vector $g$
and a scalar $\alpha$; geometrically it is an object in the graph space
$\mathbb{R}^{n+1}$ that, for a convex function, supports the graph from
below (a subgradient inequality), and for a concave one from above.

This matters in SMS++ because, for many structured functions, computing
the value already produces a linearization as a by-product. The
canonical case is the Lagrangian function of a structured problem
(`C05Function.h:97-135`). Let $U$ be a set, $c$ a vector, $A$ a matrix
and $b$ a vector of compatible sizes, and consider

$$
(P) \quad \max \{\, c u : A u = b ,\; u \in U \,\} , \qquad
f(x) = \max \{\, c u + x (b - A u) : u \in U \,\} ,
\tag{13.1}
$$

where the maximum is assumed to be attained for every $x$ (e.g., $U$
compact and nonempty). The following is the property on which the
Lagrangian machinery of [Chapter 14](14-lag-benders-bfunction.md#ch-14)
rests.

**Lemma 13.1.** *For every $\bar u \in U$, the affine function
$L_{\bar u}(x) = c \bar u + x (b - A \bar u)$ satisfies
$L_{\bar u}(y) \le f(y)$ for all $y$. If, furthermore, $\bar u$ is
optimal in (13.1) at $\bar x$, then $L_{\bar u}(\bar x) = f(\bar x)$,
and $g = b - A \bar u$ is a subgradient of $f$ at $\bar x$, i.e.,
$f(y) \ge f(\bar x) + g (y - \bar x)$ for all $y$. In particular, $f$
is convex.*

*Proof.* For every $y$, $\bar u$ is feasible for the maximization that
defines $f(y)$, hence $f(y) \ge c \bar u + y (b - A \bar u) =
L_{\bar u}(y)$. If $\bar u$ is optimal at $\bar x$, then $f(\bar x) =
c \bar u + \bar x (b - A \bar u) = L_{\bar u}(\bar x)$, and
$L_{\bar u}(y) = L_{\bar u}(\bar x) + g (y - \bar x) = f(\bar x) +
g (y - \bar x)$, which with the first part gives the subgradient
inequality. Finally, $f$ is the pointwise maximum of the affine
functions $L_u$, $u \in U$, and therefore convex. $\square$

The same argument with $\bar u$ an $\varepsilon$-optimal solution at
$\bar x$ gives $f(y) \ge f(\bar x) - \varepsilon + g (y - \bar x)$,
i.e., an $\varepsilon$-subgradient; this is why a linearization may
"refer to several points", and one point may provide several
linearizations. `LagBFunction` exploits this mechanism
([Chapter 14](14-lag-benders-bfunction.md#ch-14)).

`C05Function` organises linearizations into two *pools*.

- The *local pool* is tied to the last point $x$ at which `compute()`
  was called, and it is entirely reset when `compute()` is called
  again. It is meant to hold the linearizations "significant at $x$"
  (for a convex function, $\varepsilon$-subgradients at $x$; for a
  concave one, $\varepsilon$-supergradients; for a smooth one, the
  gradient), and its maximum size is the parameter `intLPMaxSz`, 1 by
  default.
- The *global pool* persists across `compute()` calls and is the basis
  of *reoptimization*: it holds linearizations (or the information to
  compute them, such as $\bar u$ in Lemma 13.1) that remain valid as the
  point moves, so that a method that evaluates the function at many
  nearby points (a bundle method, a cutting-plane method) can reuse them
  rather than computing them again. Linearizations are moved from the
  local to the global pool, combined with
  `store_combination_of_linearizations()` (the convex combination of
  some of them, with given multipliers), and there is usually one
  "important" combination, the one that certifies optimality at the
  end of the solution of a problem in which $f$ is minimized.

The linearization machinery is supported by a family of
`C05FunctionMod` `Modification`s (abstract `Modification`s in the sense
of [Chapter 8](08-modification-janus.md#ch-8)) that say how a change of
the data of the function affects its value and its pools, which is the
subject of the next section.

## 13.3 Modifications, directionality, and reoptimization {#sec-13-3}

The `Function` family has a rich vocabulary for describing *how* a
function has changed, for one reason: to let a `:Solver` *reoptimize*,
i.e., reuse the work it has already done, instead of starting over.

### Directionality of a value change: `FunctionMod`

The base `FunctionMod` describes a change of the *values* of a function,
and its payload, the extended real returned by `shift()`, encodes the
predictability and the direction of the change (`Function.h:813-857`
and the comment of `shift()`). Writing $f$ and $f'$ for the function
before and after the change, the four cases are the following.

- A finite `shift()` $s$: $f'(x) = f(x) + s$ for all $x$ (the constant
  term of the function changed).
- $+\infty$ (`INFshift`): the values changed unpredictably but
  monotonically upwards, i.e., $f'(x) \ge f(x)$ for all $x$.
- $-\infty$ (`-INFshift`): symmetrically, $f'(x) \le f(x)$ for all $x$.
- NaN (`NaNshift`, to be tested with `std::isnan()`, never with `==`):
  the values changed unpredictably all over the space; the set of
  active `Variable`s, however, is unchanged, changes of it having their
  own `Modification`.

What a `:Solver` can keep in each case is the following, stated for the
minimization of $f$ over a set $X$ that does not change, with
$v = \inf_X f$ and $v' = \inf_X f'$.

**Lemma 13.2.** *Let $\ell \le v \le u$ be a lower and an upper bound
known before the change, and let $\hat x \in X$ be a point with known
value $f(\hat x)$. (i) If `shift()` is finite, $\ell + s \le v' \le
u + s$ and $f'(\hat x) = f(\hat x) + s$. (ii) If it is $+\infty$, then
$\ell \le v'$, while no upper bound survives. (iii) If it is
$-\infty$, then $v' \le u$, and $v' \le f(\hat x)$, while no lower bound
survives. (iv) If it is NaN, no bound survives.*

*Proof.* (i) Taking the infimum over $X$ of $f' = f + s$ gives
$v' = v + s$. (ii) For every $x \in X$, $f'(x) \ge f(x) \ge v \ge \ell$,
hence $v' \ge \ell$. (iii) $v' \le f'(\hat x) \le f(\hat x)$, and for
every $x \in X$, $v' \le f'(x) \le f(x)$, whence $v' \le v \le u$.
That no other bound survives in (ii), (iii) and (iv) is seen by
taking $f' = f + t$ for a constant $t$ of the appropriate sign (or of
either sign) and arbitrarily large modulus, which is compatible with the
information given. $\square$

Hence, the more a `:Solver` knows about the direction of a change, the
more of its previous work it can keep: an exact shift preserves
everything up to a translation, a monotone change preserves the bounds
on one side, and only the NaN case forces a fresh start. For a
maximization the roles of (ii) and (iii) are exchanged. The same lemma
applies to the linearizations of a convex $f$: a linearization
$L \le f$ stays below $f'$ in case (ii), and $L + s$ is a linearization
of $f'$ in case (i).

### Quasi-additivity: `FunctionModVars`

When the change is the *addition or removal of active `Variable`s*,
the relevant `Modification` is a `FunctionModVars` (`FunctionModVarsAddd`
for an addition, `FunctionModVarsRngd` and `FunctionModVarsSbst` for the
removal of a range or of a subset), which carries the affected
`Variable`s and, again through a `shift()` value, says whether the
change is *quasi-additive* (`Function.h:1034-1146`). Adding `Variable`s
$y$ to a function $f_{\text{old}}(x)$ is quasi-additive if

$$
f(x, 0) = f_{\text{old}}(x) + s \quad\text{for all } x ,
$$

with $s$ = `shift()` finite, i.e., if fixing the new `Variable`s to their
default value 0 recovers the old function up to a constant shift; the
removal of $y$ from $f_{\text{old}}(x, y)$ is quasi-additive if
$f(x) = f_{\text{old}}(x, 0) + s$ for all $x$. A NaN `shift()` says that
the change is not quasi-additive; $+\infty$ and $-\infty$ say that it is
not, but that it is monotone, i.e., $f(x, 0) \ge f_{\text{old}}(x)$ (or
$\le$) for an addition, and $f(x) \ge f_{\text{old}}(x, 0)$ (or $\le$)
for a removal. The definition is what makes the old values reusable.

**Lemma 13.3.** *If the addition of $y$ is quasi-additive with shift
$s$, every value $f_{\text{old}}(\bar x)$ computed before the change
gives the value $f(\bar x, 0) = f_{\text{old}}(\bar x) + s$ after it,
and every bound $\ell \le \inf_X f_{\text{old}}$ gives
$\ell + s \le \inf \{ f(x, y) : x \in X, y = 0 \}$. If the removal of
$y$ is quasi-additive with shift $s$, every value
$f_{\text{old}}(\bar x, \bar y)$ computed at a point with $\bar y = 0$
gives $f(\bar x) = f_{\text{old}}(\bar x, 0) + s$, while the values
computed at points with $\bar y \ne 0$ say nothing about $f$.*

*Proof.* The first two claims are the definition evaluated at $\bar x$
and its infimum over $X$, as in Lemma 13.2 (i). The third is the
definition evaluated at $\bar x$. For the last, the definition only
constrains $f_{\text{old}}$ on $\{ y = 0 \}$, and therefore
$f_{\text{old}}(\bar x, \bar y)$ with $\bar y \neq 0$ can be changed
arbitrarily without affecting $f$. $\square$

### Strong quasi-additivity: `C05FunctionModVars*`

For a `C05Function`, reusing *values* is not enough: one would like to
reuse *linearizations*, i.e., the contents of the global pool, as well.
A `C05FunctionModVarsAddd` (respectively, `C05FunctionModVarsRngd` and
`C05FunctionModVarsSbst` for a removal) says that the change is
*strongly* quasi-additive (`C05Function.h:2205-2240`), which means that
the linearizations in the global pool can be updated to linearizations
of the new function: for an addition, by computing the $N$ new entries
of their vector $g$ with `get_linearization_coefficients()`, for a
removal, by deleting the entries of the removed `Variable`s. This
likely means that the function is convex or concave, but it is not
required. If the change is only quasi-additive (a base
`FunctionModVarsAddd` that is *not* a `C05FunctionModVarsAddd`), a
`:Solver` has to consider every linearization computed at a point with
$y \neq 0$ as invalid, even in its $g_x$ part, and the `C05Function`
then has to issue also a `C05FunctionMod` of type `GlobalPoolRemoved`
with an empty `which()`, saying explicitly that the whole global pool is
invalid (before the `FunctionModVarsAddd`, as working on an empty pool
is cheaper). Note that a `C05FunctionModVarsAddd` derives from
`FunctionModVarsAddd`, and therefore it has to be tested for first.

The Lagrangian function (13.1) is the case that motivates the
distinction. If a block $A' u = b'$ of constraints is added to $(P)$ and
relaxed with multipliers $y$, the function becomes
$f(x, y) = \max \{ c u + x (b - A u) + y (b' - A' u) : u \in U \}$.

**Lemma 13.4.** *In the setting above, the addition of $y$ is
quasi-additive with $s = 0$ and strongly quasi-additive: for every
$\bar u \in U$ stored in the global pool, the extended linearization
$c \bar u + x (b - A \bar u) + y (b' - A' \bar u)$ is below
$f(x, y)$ everywhere, and it is a subgradient inequality at every point
$(\bar x, \bar y)$ at which $\bar u$ is optimal, whatever the point at
which $\bar u$ was computed. Conversely, removing the multipliers $y$ of
a block of constraints is quasi-additive with $s = 0$, and every
linearization of $f(x, y)$ with the entries of $y$ deleted is below the
new $f(x)$.*

*Proof.* At $y = 0$ the two maximizations coincide, hence
$f(x, 0) = f_{\text{old}}(x)$ for all $x$. The extended linearization is
$L_{\bar u}$ of Lemma 13.1 for the problem with the constraints
$[A ; A'] u = [b ; b']$, and Lemma 13.1 gives both claims, since its
proof only uses $\bar u \in U$ and, for the second claim, the optimality
of $\bar u$ at the point. For the removal, the new function is the old
one at $y = 0$, and a linearization of the old one, $c \bar u + x (b -
A \bar u) + y (b' - A' \bar u) \le f(x, y)$ for all $(x, y)$, gives at
$y = 0$ the linearization with the entries of $y$ deleted, below
$f(x, 0) = f(x)$. $\square$

We remark that the linearizations so obtained are, in general,
$\varepsilon$-subgradients at the points of interest with an $\varepsilon$
that changes with the function (Lemma 13.1 and the remark after it),
which is why the information stored in the global pool (here, $\bar u$)
matters more than the vector $g$ itself. Without convexity the
situation is different: for the quasi-additive removal of $y$ from
$f_{\text{old}}(x, y) = x e^y$, i.e., $f(x) = x$, the old gradient
$[e^{\bar y}, \bar x e^{\bar y}]$ reduced to its first entry is not a
linearization of $f$ unless $\bar y = 0$, and it is unclear whether
anything can be saved.

This graduated vocabulary (exact shift, monotone direction, or
unpredictable; quasi-additive or strongly quasi-additive) is the
machinery of reoptimization in SMS++, and
[Chapter 14](14-lag-benders-bfunction.md#ch-14) shows it at work:
`LagBFunction` maps the `Modification`s coming from its inner `Block`
into these types, so that a `BundleSolver` driving the Lagrangian dual
can reoptimize across changes of the inner problem.

## 13.4 `C15Function`: second-order information {#sec-13-4}

[`C15Function`](https://smspp.gitlab.io/smspp-project/db/dbd/class_s_m_spp__di__unipi__it_1_1_c15_function.html)
extends `C05Function` with *second-order* information, i.e., (partial)
Hessians, for the cases where a method can exploit curvature, and it
has its own `C15FunctionMod`. It is used far less often than
`C05Function` in the current catalogue, since the structure-exploiting
methods that dominate the applications of SMS++ (bundle, Benders, SDDP)
are first-order; however, the quadratic leaf classes below derive from
it.

## 13.5 The "easy" leaf Functions {#sec-13-5}

For functions that have a simple closed algebraic form, evaluating them
and producing their linearizations costs essentially nothing, and SMS++
provides concrete leaf classes.

- [`LinearFunction`](https://smspp.gitlab.io/smspp-project/d9/d23/class_s_m_spp__di__unipi__it_1_1_linear_function.html)
  (a `C15Function`) is a linear form $\sum_i a_i x_i$ in `ColVariable`s
  plus a constant. It is the `Function` inside an `FRowConstraint` that
  encodes a row of a linear program, or inside an `FRealObjective` that
  encodes a linear objective, and it is what `MCFBlock`,
  `BinaryKnapsackBlock` and the natural formulation of CFL use for their
  constraints and objectives
  ([Chapter 5](05-variable-constraint-objective.md#ch-5),
  [Chapter 7](07-physical-abstract.md#ch-7)).
- [`DQuadFunction`](https://smspp.gitlab.io/smspp-project/d3/dc2/class_s_m_spp__di__unipi__it_1_1_d_quad_function.html)
  (a `C15Function`) is a *separable* quadratic function, a sum of
  terms $q_i x_i^2 + a_i x_i$ plus a constant, and
  [`QuadFunction`](https://smspp.gitlab.io/smspp-project/d3/d99/class_s_m_spp__di__unipi__it_1_1_quad_function.html),
  derived from it, a general (non-diagonal) quadratic function.
- [`PolyhedralFunction`](https://smspp.gitlab.io/smspp-project/de/da6/class_s_m_spp__di__unipi__it_1_1_polyhedral_function.html)
  is the pointwise maximum (convex case) or minimum (concave case) of a
  finite set of affine pieces, possibly with a bound and with
  "vertical" pieces that define its domain; it is a `C05Function`
  whose linearizations are its defining pieces, which makes it the
  natural recipient of cuts generated elsewhere. The companion
  [`PolyhedralFunctionBlock`](https://smspp.gitlab.io/smspp-project/df/dcc/class_s_m_spp__di__unipi__it_1_1_polyhedral_function_block.html)
  wraps one as a `Block`, which can present it either as an
  `FRealObjective` with the `PolyhedralFunction` inside or as its
  epigraphic linear program. `PolyhedralFunction` is, for instance,
  where the Benders cuts produced inside an `SDDPBlock` are accumulated.
- [`C05SumFunction`](https://smspp.gitlab.io/smspp-project/d9/d47/class_s_m_spp__di__unipi__it_1_1_c05_sum_function.html)
  is the sum $f(x) = \sum_{h \in G} f_h(x)$ of a given group $G$ of
  `C05Function`s, its members, which it neither owns nor changes. Its
  active `Variable`s are the union of theirs, its value, global bounds
  and Lipschitz constant are the sums of theirs, and a linearization of
  the group is the sum of linearizations stored under the same name in
  the global pools of the members (a member at its global bound taking
  part with the horizontal linearization at that bound). It is, by
  default, the `Observer` of its members, so that what happens to them
  is seen as happening to the sum; the members can be evaluated in
  parallel (`set_members_at_once()`), possibly on the threads of
  whoever drives the group (`set_member_runner()`). `BundleSolver` uses
  it to put several components of its function into one.

That the sum of linearizations is a linearization of the sum is the
following elementary fact, stated for the convex case (the concave one
is symmetric).

**Lemma 13.5.** *If $L_h(x) = g_h x + \alpha_h \le f_h(x)$ for all $x$
and all $h \in G$, then $L(x) = \sum_h L_h(x) \le f(x)$ for all $x$; if,
furthermore, each $L_h$ is an $\varepsilon_h$-subgradient inequality of
$f_h$ at the same point $\bar x$, then $L$ is an $\varepsilon$-subgradient
inequality of $f$ at $\bar x$ with $\varepsilon = \sum_h \varepsilon_h$.*

*Proof.* Summing the inequalities $L_h(x) \le f_h(x)$ over $h$ gives the
first claim. For the second, summing $f_h(\bar x) - \varepsilon_h \le
L_h(\bar x)$ gives $f(\bar x) - \varepsilon \le L(\bar x)$, and $L$ is
affine with $L \le f$, which is the definition of an
$\varepsilon$-subgradient inequality. $\square$

The lemma is the reason why a diagonal linearization of the group has to
be the sum of linearizations of the members computed at the *same*
point: summed at different points, they would still be below $f$, but
the sum of the $\varepsilon_h$ would not be measured at any one point.

## 13.6 Where the family points next {#sec-13-6}

The two most consequential members of the family are not leaf functions
at all, but the hybrids that are at the same time a `Block` and a
`C05Function`,
[`LagBFunction`](https://smspp.gitlab.io/smspp-project/d2/df7/class_s_m_spp__di__unipi__it_1_1_lag_b_function.html)
and
[`BendersBFunction`](https://smspp.gitlab.io/smspp-project/dd/d28/class_s_m_spp__di__unipi__it_1_1_benders_b_function.html).
Each wraps an inner `Block` and turns it into a `C05Function` whose
evaluation is the solution of that inner `Block`, and whose
linearizations are produced from the primal or dual solutions of the
inner `Block`: the mechanism of Lemma 13.1, made into a reusable
component. They are the subject of
[Chapter 14](14-lag-benders-bfunction.md#ch-14), and the engines of
[Recipe R4](R4-cfl-lagrangian.md#rec-R4) (Lagrangian) and
[Recipe R5](R5-cfl-benders.md#rec-R5) (Benders).

## 13.7 Idioms {#sec-13-7}

*A `Function` in a carrier.* Since a `Function` does not register with
its `Variable`s, the supported way to use one is to give it to an
`FRowConstraint`, an `FRealObjective`, or one of the hybrids of `Block`
and `Function`, which takes ownership and maintains the dependencies; a
bare `Function` held by user code is an advanced use that requires
managing the registration by hand.

*A `C05Function` when first-order structure exists.* If the value of a
function comes with a linearization "for free" (a Lagrangian dual, a
value function, a polyhedral epigraph), modelling it as a `C05Function`
lets every bundle, cutting-plane or Benders `:Solver` exploit the
linearization pools, while modelling it as a plain `Function` throws
that information away.

*The pools for reoptimization.* When a function is evaluated at a
sequence of nearby points, the global pool is what makes the sequence
cheap: a `:Solver` adds to it and draws from it rather than computing
linearizations again, which is the mechanism on which `BundleSolver`
relies; the `Modification`s of
[§13.3](13-function-family.md#sec-13-3) say which of the stored
linearizations survive a change (Lemmas 13.2 to 13.4).
