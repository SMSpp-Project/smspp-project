# 13. The Function family {#ch-13}

[Source: `SMS++/include/Function.h`, `C05Function.h`,
`C15Function.h`, `DQuadFunction.h`, `QuadFunction.h`,
`PolyhedralFunction.h`, `PolyhedralFunctionBlock.h`]

`Function`s are the objects that supply the *value* inside an
`FRowConstraint` (its left-hand side) and inside an
`FRealObjective` (its value); they also stand on their own as the
mathematical content of the two `Block`-and-`Function` hybrids of
[Chapter 14](14-lag-benders-bfunction.md#ch-14). This chapter describes the hierarchy and the one idea
that gives it its algorithmic power: the *linearization pools* of
`C05Function`.

![The `Function` hierarchy: from the base `Function` through `C05Function` (first-order information) and `C15Function` (second-order information) down to the leaf implementations.](../figures/Function-doxy.svg)

## 13.1 The base `Function` {#sec-13-1}

A
[`Function`](https://smspp.gitlab.io/smspp-project/d9/dd8/class_s_m_spp__di__unipi__it_1_1_function.html)
is a real-valued function of a set of *active* `Variable`s. Like
`Constraint` and `Objective`, it derives from both
[`ThinVarDepInterface`](https://smspp.gitlab.io/smspp-project/da/df9/class_s_m_spp__di__unipi__it_1_1_thin_var_dep_interface.html)
(it depends on a set of `Variable`s) and `ThinComputeInterface`
(it must be `compute()`-d, possibly at significant cost). Its value
type is `FunctionValue` (a `double`). The base class exposes only
*values*: `compute()` evaluates the function at the current values
of its active `Variable`s, and accessor methods read the result.
First- and second-order information, and any specific algebraic
form, are left to derived classes.

Crucially, the base `Function` also *advertises* a number of
mathematical properties that a `:Solver` can rely on to choose its
algorithm. These are exposed through a family of `is_*()`
predicates (`Function.h:567-615`):

- `is_convex()` and `is_concave()` — whether the function is
  convex / concave (both default to `false`; a function that is
  neither simply returns `false` to both);
- `is_linear()` — defined as `is_convex() && is_concave()`;
- `is_lower_semicontinuous()`, `is_upper_semicontinuous()`, and
  `is_continuous()` (true when both of the former hold).

A bundle method, for example, will check `is_convex()` (or
`is_concave()`) before treating the linearizations as
sub/supergradients; a `:Solver` that requires a smooth objective
will check `is_continuous()`. The base class returns the most
conservative answers; a concrete `:Function` overrides them to
declare the structure it actually has.

Two further facts about the base class are worth keeping in mind.

First, a `Function` reports its `Modification`s to *one*
`Observer` ([Chapter 8](08-modification-janus.md#ch-8)) — typically the `FRowConstraint`,
`FRealObjective`, or `Block` that contains it. A `Function` can
also be evaluated *approximately*: a caller may accept lower
and/or upper estimates of the value, provided they are accurate
enough, which is what makes inexact bundle and Benders methods
possible.

Second — a subtle but important rule — a `Function` does **not**
register itself with its active `Variable`s. A free-floating
`Function` has no way of knowing whether it is a part of some
`Constraint` / `Objective` or not, so the responsibility of
registering with the `Variable`s falls on the
`ThinVarDepInterface` that *uses* the `Function` (the
`FRowConstraint`, `FRealObjective`, ...). When a `Variable` is
added to or removed from the `Function`, the `Function` issues a
`FunctionModVars` `Modification`, which the containing object
observes and reacts to by registering / unregistering itself with
that `Variable` (`Function.h:90-119`). The practical consequence
for a user is that `Function`s are always handed to a carrier
(`set_function(...)`, [§5.3](05-variable-constraint-objective.md#sec-5-3)) that takes care of this; one does not
normally hold a bare `Function` and expect it to maintain its own
`Variable` dependencies.

## 13.2 `C05Function`: first-order information and linearization pools {#sec-13-2}

[`C05Function`](https://smspp.gitlab.io/smspp-project/d3/de3/_c05_function_8h.html)
extends `Function` with *first-order* information: not necessarily
continuous, in the form of **linearizations**. Assuming the active
`Variable`s are single reals (`ColVariable`s), a linearization is
an affine function $L(x) = gx + \alpha$ defined by a real
$n$-vector $g$ and a scalar $\alpha$; geometrically it is an object
in the graph space $\mathbb{R}^{n+1}$ that, for a convex function,
*supports the graph from below* (a subgradient inequality), and
for a concave one from above.

The reason this matters so much in SMS++ is that, for many
structured functions, computing the value already produces the
linearization as a by-product. The canonical case is a Lagrangian
function $f(x) = \max \lbrace cu + x(b - Au) \mid u \in U \rbrace$: any
optimal $u^*$ of the inner problem yields both $f(x) = cu^* +
x(b - Au^*)$ and the linearization $(g, \alpha) = (b - Au^*,
cu^*)$, which supports the graph of $f$ everywhere
(`C05Function.h:96-133`). This is exactly the mechanism that
`LagBFunction` exploits ([Chapter 14](14-lag-benders-bfunction.md#ch-14)).

`C05Function` organises linearizations into two **pools**:

- the **local pool** is tied to the last point $x$ at which
  `compute()` was called, and is automatically cleared when
  `compute()` is next called at a different point. It is meant to
  hold the linearizations "significant at $x$" — for a convex
  function, the $\varepsilon$-subgradients at $x$; for a concave
  one, the $\varepsilon$-supergradients; for a smooth one, the
  gradient. One evaluation may produce several linearizations
  (multiple $\varepsilon$-optimal inner solutions), and one
  linearization may be valid at many points — both situations the
  pool accommodates.
- the **global pool** persists across `compute()` calls and is the
  basis of *reoptimization*: it accumulates linearizations that
  remain valid as the point moves, so that a method which
  re-solves the function at many nearby points (a bundle method, a
  cutting-plane method) can reuse previously computed
  linearizations rather than recomputing them. The global pool
  supports operations such as taking a convex combination of its
  entries, and marking an "important linearization" (the one
  active at an optimum).

The linearization machinery is supported by a family of
`C05FunctionMod` `Modification`s (the abstract-stream
`Modification`s of [Chapter 8](08-modification-janus.md#ch-8)) that signal, in fine-grained ways,
how a change in the function's data affects its value and its
pools — the subject of the next section.

## 13.3 Modifications, directionality, and reoptimization {#sec-13-3}

The `Function` family carries an unusually rich vocabulary for
describing *how* a function has changed, and it does so for a
single, deliberate reason: to let a `:Solver` **reoptimize** —
reuse the work it has already done — instead of starting over.
This emphasis on reoptimization is one of the more distinctive
aspects of SMS++; the `Function` `Modification`s are where it is
most visible, and the design goes well beyond what one usually
finds in comparable settings.

### Directionality of a value change: `FunctionMod`

The base
`FunctionMod` describes a change to the *value* of a function, and
its single payload — the extended real returned by `shift()` —
encodes not just the magnitude but the *predictability and
direction* of the change (`Function.h:798-841`). Four cases:

- a **finite, non-NaN** `shift()`: the value changed by *exactly*
  that amount at *every* point — the most informative case, "the
  constant term of the function changed". A `:Solver` can update
  every cached value and every linearization by the same additive
  shift, with no recomputation.
- `+`∞ (`INFshift`): the value changed "unpredictably but
  monotonically **upwards**" — every value is now $\ge$ what it
  was. The exact new values are unknown, but the *direction* is,
  which is enough for a `:Solver` maintaining bounds to keep a
  valid lower or upper bound.
- `−`∞ (`−INFshift`): symmetrically, "unpredictably but
  monotonically **downwards**".
- **NaN** (`NaNshift`): the value changed unpredictably, in no
  particular direction — the "nuclear `Modification` for a
  `Function`": everything previously known about its value is
  unreliable (the set of active `Variable`s, however, is
  unchanged; changes to that have their own `Modification`).

The point of distinguishing "up", "down", "none/exact" and "any"
is precisely reoptimization: the more a `:Solver` knows about the
*direction* of a change, the more of its previous work it can
keep. A monotone-up change preserves lower bounds; an exact shift
preserves everything up to a translation; only the NaN case forces
a fresh start.

### Quasi-additivity: `FunctionModVars`

When the change is the *addition or removal of active
`Variable`s*, the relevant `Modification` is a `FunctionModVars`,
which carries the affected `Variable`s and, again through a
`shift()` value, declares whether the change is **quasi-additive**
(`Function.h:1008-1059`). Adding `Variable`s $y$ to a function
$f_{\text{old}}(x)$ is quasi-additive if

$$f(x, 0) = f_{\text{old}}(x) + \text{shift()} \quad\text{for all } x,$$

i.e. if fixing the new `Variable`s to their default value (0)
recovers the old function up to a constant shift; removal is
defined symmetrically. Quasi-additivity is what lets a `:Solver`
reuse previously computed *values* across a change in the variable
set: the old values are still meaningful, shifted by a known
constant, at the points where the added variables are zero. (A
NaN `shift()` signals that the change is *not* quasi-additive and
the old values cannot be trusted.)

### Strong quasi-additivity: `C05FunctionModVars`

For a `C05Function`, reusing *values* is not enough; one would
like to reuse *linearizations* — the contents of the global pool —
as well. This requires a stronger property. The
`C05FunctionModVarsAddd` `Modification` signals that a
variable-set change is **strongly quasi-additive** (which in
practice usually means the function is convex or concave;
`C05Function.h:2259-2285`). When it is, the linearizations already
in the global pool remain valid (suitably interpreted) after the
change, so a bundle / cutting-plane `:Solver` can keep its whole
model. When a change is only ordinarily quasi-additive — a base
`FunctionModVarsAddd` is issued, *not* the `C05` one — a `:Solver`
must treat the first-order information computed at points where
the added variables were non-zero as invalid, even though the
*values* may still be reusable. The framework therefore lets a
`C05Function` declare exactly how much of the previous first-order
work survives a change, down to this fine distinction.

This graduated vocabulary — exact shift vs monotone direction vs
unpredictable; quasi-additive vs strongly quasi-additive — is the
machinery that makes SMS++'s reoptimization possible in practice,
and [Chapter 14](14-lag-benders-bfunction.md#ch-14) shows it doing real work: `LagBFunction` maps the
`Modification`s coming from its inner `Block` into precisely these
`FunctionMod*` types, so that a `BundleSolver` driving the
Lagrangian dual can reoptimize across changes to the inner
problem.

## 13.4 `C15Function`: second-order information {#sec-13-4}

[`C15Function`](https://smspp.gitlab.io/smspp-project/db/dbd/class_s_m_spp__di__unipi__it_1_1_c15_function.html)
extends `C05Function` with *second-order* information — (partial)
Hessians — for the cases where a method can exploit curvature. It
is used far less often than `C05Function` in the current
catalogue, because the structure-exploiting methods that dominate
SMS++ applications (bundle, Benders, SDDP) are first-order, but it
is there for the second-order methods that need it.

## 13.5 The "easy" leaf Functions {#sec-13-5}

For functions that have a simple closed algebraic form, evaluating
them and producing their (trivial) linearizations costs essentially
nothing, and SMS++ provides concrete leaf classes with no overhead:

- a linear `Function` — a plain linear form $\sum_i a_i x_i$ in
  `ColVariable`s — is the recipient inside an `FRowConstraint`
  that encodes a "row of a linear program" or inside an
  `FRealObjective` that encodes a linear objective; this is what
  `MCFBlock`, `BinaryKnapsackBlock` and the natural formulation of
  CFL use for their constraints and objectives (Chapters [5](05-variable-constraint-objective.md#ch-5), [7](07-physical-abstract.md#ch-7)).
- [`DQuadFunction`](https://smspp.gitlab.io/smspp-project/d3/dc2/class_s_m_spp__di__unipi__it_1_1_d_quad_function.html)
  is a *separable* quadratic form (a sum of per-variable quadratic
  terms);
  [`QuadFunction`](https://smspp.gitlab.io/smspp-project/d3/d99/class_s_m_spp__di__unipi__it_1_1_quad_function.html)
  is a general quadratic form.
- [`PolyhedralFunction`](https://smspp.gitlab.io/smspp-project/de/da6/class_s_m_spp__di__unipi__it_1_1_polyhedral_function.html)
  is a function defined as the pointwise maximum (or minimum) of a
  finite set of affine pieces — i.e., a function that *is* its own
  set of linearizations. It is a `C05Function` whose linearizations
  are exactly its defining pieces, which makes it the natural
  recipient of cuts generated externally; the companion
  [`PolyhedralFunctionBlock`](https://smspp.gitlab.io/smspp-project/df/dcc/class_s_m_spp__di__unipi__it_1_1_polyhedral_function_block.html)
  wraps one as a `Block`. `PolyhedralFunction` is, for instance,
  where the Benders cuts produced inside an `SDDPBlock` are
  accumulated ([Chapter 14](14-lag-benders-bfunction.md#ch-14) mentions this in passing).

## 13.6 Where the family points next {#sec-13-6}

The two most consequential members of the family are not "leaf"
functions at all but the hybrids that are *simultaneously* a
`Block` and a `C05Function`:
[`LagBFunction`](https://smspp.gitlab.io/smspp-project/d2/df7/class_s_m_spp__di__unipi__it_1_1_lag_b_function.html)
and
[`BendersBFunction`](https://smspp.gitlab.io/smspp-project/dd/d28/class_s_m_spp__di__unipi__it_1_1_benders_b_function.html).
Each wraps an inner `Block` and turns it into a `C05Function`
whose evaluation is the solution of that inner `Block`, and whose
linearizations are produced from the inner `Block`'s primal / dual
solutions — exactly the Lagrangian-function mechanism sketched in
[§13.2](13-function-family.md#sec-13-2), made into a reusable component. They are the subject of
[Chapter 14](14-lag-benders-bfunction.md#ch-14), and the engines behind [Recipes R4](R4-cfl-lagrangian.md#rec-R4) (Lagrangian) and R5
(Benders).

## 13.7 Idioms {#sec-13-7}

**Hand a `Function` to a carrier; do not free-float it.** Because
a `Function` does not register with its `Variable`s, the supported
way to use one is to give it to an `FRowConstraint`, an
`FRealObjective`, or one of the `Block`+`Function` hybrids, which
takes ownership and maintains the `Variable` dependencies. A bare
`Function` held by user code is an advanced use that requires
managing the registration by hand.

**Reach for `C05Function` when first-order structure exists.** If
a function's value comes with a linearization "for free" (a
Lagrangian dual, a value function, a polyhedral epigraph),
modelling it as a `C05Function` lets every bundle / cutting-plane
/ Benders `:Solver` exploit the linearization pools. Modelling it
as a plain `Function` throws that information away.

**Let the pools do the reoptimization.** When a function is
re-evaluated at a sequence of nearby points, the global pool is
what makes the sequence cheap: a `:Solver` should add to and draw
from it rather than recomputing linearizations from scratch. This
is the mechanism that `BundleSolver` relies on.
