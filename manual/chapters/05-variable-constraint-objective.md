# 5. Variable, Constraint, Objective {#ch-5}

[Source: `SMS++/include/Variable.h`, `ColVariable.h`, `Constraint.h`,
`RowConstraint.h`, `OneVarConstraint.h`, `FRowConstraint.h`,
`Objective.h`, `RealObjective.h`, `FRealObjective.h`,
`LinearObjective.h`]

## 5.1 Concept {#sec-5-1}

The three primitives that live inside a `Block` are
[`Variable`](https://smspp.gitlab.io/smspp-project/df/d67/class_s_m_spp__di__unipi__it_1_1_variable.html),
[`Constraint`](https://smspp.gitlab.io/smspp-project/d2/d03/class_s_m_spp__di__unipi__it_1_1_constraint.html),
and
[`Objective`](https://smspp.gitlab.io/smspp-project/d3/d23/class_s_m_spp__di__unipi__it_1_1_objective.html).
A `Block` contains, conceptually, any number of `Variable`s and
`Constraint`s organised in *groups*, and at most one `Objective`.
Together they constitute the *abstract representation* of the
mathematical model carried by the `Block` ([Chapter 7](07-physical-abstract.md#ch-7)); when a
`Block` is solved by a general-purpose `:Solver`, this is what the
`:Solver` reads.

All three classes share two architectural traits:

- each instance **belongs to one `Block`** (its `f_Block` field
  points to the owning `Block`);
- the *name* of an instance is its **memory address**, with the
  consequence — discussed for `Block` in [§4.2](04-block.md#sec-4-2) — that an instance
  cannot be moved once constructed. Dynamic groups therefore live
  in `std::list`; static groups live in `std::vector` or
  `boost::multi_array` whose storage is allocated once and never
  reallocated.

`Constraint` and `Objective` further share two interfaces inherited
from common base classes:

- they derive from
  [`ThinVarDepInterface`](https://smspp.gitlab.io/smspp-project/da/df9/class_s_m_spp__di__unipi__it_1_1_thin_var_dep_interface.html):
  each `Constraint` and each `Objective` depends on a specific set
  of *active* `Variable`s, queryable through that interface;
- they derive from
  [`ThinComputeInterface`](https://smspp.gitlab.io/smspp-project/d9/d1b/class_s_m_spp__di__unipi__it_1_1_thin_compute_interface.html):
  a `Constraint`'s satisfaction status and an `Objective`'s value
  must be `compute()`-d before being read, and computation is
  expected to be potentially costly (for instance, an `Objective`
  given by a multi-dimensional integral, or a `Constraint` given by
  the solution of a hard sub-problem).

`Variable`, by contrast, derives from neither: its "value" is
specified by derived classes, not by the base, and it has no
notion of computation (only of being fixed or not).

## 5.2 Variable {#sec-5-2}

The base class `Variable` makes very few assumptions:

- a `Variable` belongs to one `Block` (`get_Block()`);
- a `Variable` influences a set of *active stuff* (instances of
  `ThinVarDepInterface`: `Constraint`s, `Objective`s, `Function`s);
  the set is exposed through `v_iterator`s and queried through
  `is_active(...)`;
- a `Variable` can be (temporarily) *fixed* via
  `is_fixed(bool, ModParam)`; the current state is read by
  `is_fixed()` with no arguments.

No method to *read the value* of a `Variable` exists in the base
class, because that requires knowing what kind of value it is.

### `ColVariable`

The overwhelmingly common concrete `:Variable` is
[`ColVariable`](https://smspp.gitlab.io/smspp-project/dc/d32/class_s_m_spp__di__unipi__it_1_1_col_variable.html),
which holds a single real value (`VarValue = double`). The name is
a deliberate reminiscence of the column structure of a linear
programming coefficient matrix.

`ColVariable` extends `Variable` in two directions. First, it gives
the value: `set_value(VarValue)`, `get_value()`. Second, it can be
restricted to a subset of the reals by setting a *type*
(`col_var_type` enum); the 16 possible types are

```
kContinuous    any real value
kInteger       any integer value
kNonNegative   any non-negative real value
kNatural       any non-negative integer value
kNonPositive   any non-positive real value
kNegative      any non-positive integer value
kZeroReal      any real value provided it is 0
kZeroInteger   any integer value provided it is 0
kUnitary       any real value in [-1, 1]
kTernary       either -1, 0, or 1
kPosUnitary    any real value in [0, 1]
kBinary        either 0 or 1
kNegUnitary    any real value in [-1, 0]
kNegBinary     either -1 or 0
kZeroRealU     any real value in [-1, 1] provided it is 0
kZeroIntU      any integer value in [-1, 1] provided it is 0
```

Combined with the "fixed / not fixed" state, these cover most of
the bound, sign and integrality restrictions found in practical
models. Crucially, all of these are *type* restrictions stored
inside the `ColVariable` itself, *not* `Constraint`s in the
`Block`: this saves a substantial amount of memory in models where
most variables are, say, simply non-negative or binary.

## 5.3 Constraint {#sec-5-3}

The base class `Constraint`, beyond what its base classes already
provide, supports:

- *relaxation*: `relax(bool, ModParam)` temporarily disables the
  `Constraint`; `is_relaxed()` reads the current state;
- *feasibility checking*: `feasible()` returns `true` if the
  `Constraint` is satisfied at the current values of its active
  `Variable`s. This is a pure virtual method; it relies on
  `compute()` having been called first.

`Constraint` is otherwise abstract: it specifies nothing about
*what* it constrains.

### `RowConstraint`

[`RowConstraint`](https://smspp.gitlab.io/smspp-project/d1/dbf/class_s_m_spp__di__unipi__it_1_1_row_constraint.html)
specialises `Constraint` to the case "*a real-valued left-hand side
lies between a lower and an upper bound*", i.e. the canonical row
constraint of a linear (or non-linear) program:

$$
\mathrm{lhs}\_\mathrm{bound} \le \mathrm{LHS} \le \mathrm{rhs}\_\mathrm{bound}.
$$

Two `RHSValue`s store the lower and upper bounds, accessed via
`get_lhs()` / `get_rhs()` / `set_lhs(...)` / `set_rhs(...)` /
`set_both(...)`. Either bound may be infinite (sentinel `RHSINF`).
A single real dual variable is attached, accessed via `get_dual()`
and set by `set_dual(...)`. The actual real-valued left-hand side
expression is *not* defined in `RowConstraint`; that is left to
subclasses.

Two principal subclasses cover the common cases.

### `OneVarConstraint`

[`OneVarConstraint`](https://smspp.gitlab.io/smspp-project/d1/d37/class_s_m_spp__di__unipi__it_1_1_one_var_constraint.html)
is the case "*the real-valued left-hand side is the value of one
single `ColVariable`*", i.e. a bound (or pair of bounds) on a
single variable. The class ships with several pre-defined refinements:
`BoxConstraint`, `LB0Constraint`, `UB0Constraint`, `LBConstraint`,
`UBConstraint`, `NNConstraint` (non-negativity, `≥ 0`),
`NPConstraint` (non-positivity, `≤ 0`), `ZOConstraint`
(`x ∈ [0,1]`). The most common of these, `LB0Constraint`, encodes
"`0 ≤ x ≤ ub`", and is the variant used by `MCFBlock` for the arc
capacity constraints when the per-arc upper bound is not infinite
([§4.5](04-block.md#sec-4-5) and [Chapter 7](07-physical-abstract.md#ch-7)).

### `FRowConstraint`

[`FRowConstraint`](https://smspp.gitlab.io/smspp-project/db/d90/class_s_m_spp__di__unipi__it_1_1_f_row_constraint.html)
is the case "*the real-valued left-hand side is computed by a
`Function`*", and is the workhorse of essentially every "real"
linear or non-linear constraint in the framework. The `Function`
inside is set with `set_function(Function*)` and retrieved with
`get_function()`. The framework takes ownership of the `Function*`
passed to `set_function`: passing `nullptr` releases it.

The `Function` family ([Chapter 13](13-function-family.md#ch-13)) covers linear, quadratic, and
polyhedral cases, plus the two specialised "`Block` + `Function`"
hybrids `LagBFunction` and `BendersBFunction`. For a "row of a
linear program" the standard combination is `FRowConstraint`
carrying a `Function` that evaluates a linear expression in
`ColVariable`s, which is exactly what `MCFBlock` uses for its
flow-conservation constraints.

## 5.4 Objective {#sec-5-4}

The base class `Objective` adds two facts to its base classes:

- it has a *sense* (`int f_sense`), either `eMin = -1`
  (minimisation, lower values are better) or `eMax = +1`
  (maximisation). The sense is read by `get_sense()` and set by
  `set_sense(int, ModParam)`. The base class assumes the value of
  the `Objective` is totally ordered, so that minimising vs
  maximising is well defined; problems with vector-valued
  objectives would extend the class.
- a value type is *not* defined in `Objective` directly, leaving
  room for multi-objective extensions. A derived class
  `RealObjective` (and `FRealObjective` below) fixes the value
  type to be an *extended* real (`RealObjective::OFValue`, the
  same as the value type of `RealFunction`), so that `+∞` and
  `-∞` are representable to encode infeasibility and unboundedness.

The composition rule for sub-`Block`s is also fixed at the
`Objective` level: the `Objective` of a sub-`Block` is summed into
the `Objective` of its father `Block`. This is what gives the
recursive `Block` tree a single, well-defined `Objective` overall.

### `FRealObjective`

[`FRealObjective`](https://smspp.gitlab.io/smspp-project/dd/d7c/class_s_m_spp__di__unipi__it_1_1_f_real_objective.html)
is to `Objective` what `FRowConstraint` is to `Constraint`: the
value is computed by a `Function`. The interface is the same:
`set_function(Function*)` (the framework takes ownership) and
`get_function()`. `FRealObjective` is the standard choice for any
`Block` whose `Objective` is best described by a `Function`,
which in practice is "essentially every `Block` with a non-trivial
objective".

A simpler `LinearObjective` exists for the case where the objective
is a plain linear form in `ColVariable`s; it stores the
coefficients directly without the `Function` indirection.

## 5.5 Inline example: the abstract representation of `BinaryKnapsackBlock` {#sec-5-5}

A reader who reaches this point may be tempted to construct a
`Block`'s abstract representation by hand: instantiate a vector of
`ColVariable`s, build an `FRowConstraint` carrying a linear
`Function`, build an `FRealObjective` likewise, and connect
everything by hand. **That is not how the framework works.** The
abstract representation of a concrete `:Block` is built *by the
`:Block` itself*, on demand, through its
`generate_abstract_variables()`, `generate_abstract_constraints()`
and `generate_objective()` overrides. User code typically *triggers*
these calls but does not *populate* what they produce.

For `BinaryKnapsackBlock`, the trigger is as small as:

```cpp
#include "BinaryKnapsackBlock.h"

using namespace SMSpp_di_unipi_it;

int main()
{
 // physical data: 4 items, knapsack of capacity 5
 BinaryKnapsackBlock         bkb;
 const std::vector< double > W = { 2.0, 3.0, 4.0, 1.0 };  // weights
 const std::vector< double > P = { 3.0, 4.0, 5.0, 1.0 };  // profits
 const std::vector< bool >   I = { true, true, true, true };  // integral
 bkb.load( W.size() , /* capacity = */ 5.0 , W , P , I );

 // construct the abstract representation on demand
 bkb.generate_abstract_variables();
 bkb.generate_abstract_constraints();
 bkb.generate_objective();
 // from this point on, bkb carries both the physical and the
 // abstract representation; the two are kept in sync by the
 // mechanisms covered in Chapter 7 and Chapter 8.

 return 0;
}
```

What `BinaryKnapsackBlock` constructs in response — and what the
three primitives of this chapter look like in concrete — is
documented in the `:Block`'s own source comments and reflected in
its protected data members (`BinaryKnapsackBlock.h:940-942`):

- one *static* group of `Variable`s, materialised as
  `std::vector< ColVariable > v_x` of size $n$ (one per item), of
  `col_var_type` either `kBinary` (integer items) or `kPosUnitary`
  (continuous items), according to the integrality vector;
- one *static* group of `Constraint`s, consisting of a single
  `FRowConstraint f_cnst` whose `Function` is a linear expression
  $\sum_i W_i x_i$ with right-hand side equal to the capacity
  $C$ and no lower bound;
- a single `FRealObjective f_obj` carrying a linear `Function`
  with coefficients $P_i$, whose sense is `eMax` by default and
  can be flipped to `eMin` by `bkb.set_objective_sense(false)`.

The three protected fields are accessible to the rest of the
framework (and to a derived `:Block`), but they are not part of the
public interface of `Block` and should not be reached at by user
code; the framework convention is to read the *physical*
representation through the `:Block`-specific accessors
(`get_Var(i)`, `get_objective_value()`, `get_x(...)`, ...) rather
than to walk the abstract groups directly. The mechanism for
*iterating* over the abstract groups of an arbitrary `Block` is at
present mediated by `boost::any`, an arrangement scheduled for
revision in a future release; for the moment, code that needs to
look at the abstract representation of a `:Block` it does not know
should go through the `:Block`'s own getters.

The deliberately small `AbstractBlock` class ([Chapter 1](01-introduction.md#ch-1), also
discussed in [Chapter 4](04-block.md#ch-4)) is the converse case: a `Block` *without*
a problem class of its own, whose abstract representation *is*
constructed by user code from the outside. `AbstractBlock` is the
correct vehicle if the goal is to assemble a `Variable` /
`Constraint` / `Objective` arrangement by hand — typically when
loading a `.mps` or `.lp` file — and the construction patterns of
this section are the canonical way to do so. The interested reader
will find a worked example of an `AbstractBlock` built from
scratch in [Recipe R3](R3-cfl-three-ways.md#rec-R3) (where the MCF flow relaxation of CFLB is
described) and a more systematic treatment in [Appendix A](A-writing-block.md#app-A), where a
fresh `:Block` is developed from scratch.

## 5.6 API outline {#sec-5-6}

| Class | Key methods |
|---|---|
| `Variable` | `get_Block()`, `is_fixed()`, `is_fixed(bool, ModParam)`, iterators on active stuff |
| `ColVariable` | `set_value(VarValue)`, `get_value()`, `is_integer(bool, ModParam)`, `is_positive(...)`, `is_unitary(...)`, `get_type()` |
| `Constraint` | `relax(bool, ModParam)`, `is_relaxed()`, `feasible()`, `compute(bool)` |
| `RowConstraint` | `set_lhs`, `set_rhs`, `set_both`, `get_lhs`, `get_rhs`, `get_dual`, `set_dual` |
| `OneVarConstraint` | accepts a single `ColVariable`; the bounds are the only data |
| `FRowConstraint` | `set_function(Function*)`, `get_function()`; LHS is the `Function`'s value |
| `Objective` | `set_sense(int, ModParam)`, `get_sense()`, `compute(bool)` |
| `RealObjective` | adds `OFValue` and the `value()` accessor |
| `FRealObjective` | `set_function(Function*)`, `get_function()` |
| `LinearObjective` | direct linear-form storage, no `Function*` indirection |

For the exhaustive list of public methods, see the corresponding
Doxygen page of each class.

## 5.7 Idioms {#sec-5-7}

**Pointer-identity equality.** Anywhere a `Variable*` or a
`Constraint*` appears, it is compared with `==`; two pointers
referring to the same memory address are the same object. This is
not just an optimisation: it is the only sense of "equality" the
framework recognises. In particular, two `ColVariable`s with the
same value are *not* the same variable; copying a `ColVariable`
into a different vector slot makes a new, distinct `ColVariable`.

**Ownership transfer to wrappers.** `FRowConstraint::set_function`
and `FRealObjective::set_function` *take ownership* of the
`Function*` passed in. The caller must not delete it, must not
re-attach it to a second wrapper, and should not keep a
pre-existing pointer to it for later use (the wrapper may replace
or destroy the function in response to a `Modification`). Passing
`nullptr` releases the currently held `Function*` (and deletes it).

**Composition of `Objective`s in a `Block` tree.** When a
sub-`Block` carries an `Objective`, it contributes additively to
the `Objective` of its father (and recursively up). A `Block` can
have no `Objective` of its own and still expose a non-trivial
overall `Objective` made up entirely of those of its sub-`Block`s;
this is the standard pattern for "master" `Block`s that only
carry linking constraints, as in `CapacitatedFacilityLocationBlock`
in the Knapsack Formulation ([Chapter 12](12-sub-block.md#ch-12), [Recipe R4](R4-cfl-lagrangian.md#rec-R4)).
