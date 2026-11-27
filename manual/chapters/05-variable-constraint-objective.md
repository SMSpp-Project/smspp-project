# 5. Variable, Constraint, Objective {#ch-5}

[Source: [`SMS++/include/Variable.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Variable.h), [`ColVariable.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/ColVariable.h), [`Constraint.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Constraint.h),
[`RowConstraint.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/RowConstraint.h), [`OneVarConstraint.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/OneVarConstraint.h), [`FRowConstraint.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/FRowConstraint.h),
[`Objective.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Objective.h) (also `RealObjective`), [`FRealObjective.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/FRealObjective.h),
[`LinearConstraint.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/LinearConstraint.h), [`Group.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Group.h)]

We are concerned in this chapter with the three primitives that live
inside a `Block`, i.e., `Variable`, `Constraint` and `Objective`, which
together form its abstract representation. After the traits that the
three classes share ([§5.1](05-variable-constraint-objective.md#sec-5-1)),
we describe `Variable` and its concrete `ColVariable`
([§5.2](05-variable-constraint-objective.md#sec-5-2)), `Constraint` and
its refinements, together with the convention on the sign of the dual
variable of a `RowConstraint`
([§5.3](05-variable-constraint-objective.md#sec-5-3)), and `Objective`
with the rule by which the `Objective`s of a tree of `Block`s compose
([§5.4](05-variable-constraint-objective.md#sec-5-4)). We then show the
abstract representation that `BinaryKnapsackBlock` builds
([§5.5](05-variable-constraint-objective.md#sec-5-5)), outline the
interface of the classes
([§5.6](05-variable-constraint-objective.md#sec-5-6)) and close with
the idioms that follow from the identity rule
([§5.7](05-variable-constraint-objective.md#sec-5-7)).

## 5.1 Concept {#sec-5-1}

The three primitives that live inside a `Block` are
[`Variable`](https://smspp.gitlab.io/smspp-project/df/d67/class_s_m_spp__di__unipi__it_1_1_variable.html),
[`Constraint`](https://smspp.gitlab.io/smspp-project/d2/d03/class_s_m_spp__di__unipi__it_1_1_constraint.html)
and
[`Objective`](https://smspp.gitlab.io/smspp-project/d3/d23/class_s_m_spp__di__unipi__it_1_1_objective.html).
A `Block` contains any number of `Variable`s and `Constraint`s,
organized in groups, and at most one `Objective`; together they form
the *abstract representation* of the model of the `Block` (cf.
[Chapter 7](07-physical-abstract.md#ch-7)), i.e., the part of the
model that a general-purpose `:Solver` reads.

These three classes share two traits: each instance belongs to one
`Block` (returned by its `get_Block()`), and the *name* of an instance
is its memory address. Hence, as discussed for `Block` in
[§4.2](04-block.md#sec-4-2), an instance cannot be moved once
constructed: the dynamic groups live in `std::list`s, and the static
ones in `std::vector`s or `boost::multi_array`s that are sized once
and never reallocated.

`Constraint` and `Objective` also share two interfaces, inherited from
common base classes. The first is
[`ThinVarDepInterface`](https://smspp.gitlab.io/smspp-project/da/df9/class_s_m_spp__di__unipi__it_1_1_thin_var_dep_interface.html):
each `Constraint` and each `Objective` depends on a specific set of
*active* `Variable`s, which can be queried through that interface.
The second is
[`ThinComputeInterface`](https://smspp.gitlab.io/smspp-project/d9/d1b/class_s_m_spp__di__unipi__it_1_1_thin_compute_interface.html):
whether a `Constraint` is satisfied and what value an
`Objective` has must be `compute()`-d before being read, a computation
which may be somewhat costly (say, an `Objective` given by a
multi-dimensional integral, or a `Constraint` given by the solution of
a hard subproblem). `Variable`, instead, derives from neither: its
value is defined by the derived classes, not by the base one, and it
has no notion of computation (only that of being fixed or not).

## 5.2 Variable {#sec-5-2}

The base class `Variable` makes few assumptions. A `Variable` belongs
to one `Block` (`get_Block()`), in which it sits in a group
(`get_Group()`), and it influences a set of *active* objects (instances
of `ThinVarDepInterface`, i.e., `Constraint`s, `Objective`s and
`Function`s), which one can count with `get_num_active()`, read with
`get_active( i )` and search with `is_active( stuff )`. It can also
be (temporarily) *fixed* with `is_fixed( bool , ModParam )`, the
current state being read by `is_fixed()` with no arguments. However,
the base class has no method to read the value of a `Variable`, since
this requires knowing what kind of value it is.

### `ColVariable`

By far the most common concrete `:Variable` is
[`ColVariable`](https://smspp.gitlab.io/smspp-project/dc/d32/class_s_m_spp__di__unipi__it_1_1_col_variable.html),
which holds a single real value (`VarValue` is `double`); the name
recalls the columns of the coefficient matrix of a Linear Program.
`ColVariable` extends `Variable` in two ways. First, it gives the
value, through `set_value( VarValue )` and `get_value()`; a fixed
`ColVariable` keeps its value, i.e., `set_value()` with a value that
differs from it by more than a relative $10^{-6}$ throws
`std::domain_error`, and one unfixes the `ColVariable` first. Second, it
can be restricted to a subset of the reals by its *type* (the enum
`col_var_type`), whose 16 values are the following:

| type | allowed values | type | allowed values |
|---|---|---|---|
| `kContinuous` | any real | `kInteger` | any integer |
| `kNonNegative` | any real $\ge 0$ | `kNatural` | any integer $\ge 0$ |
| `kNonPositive` | any real $\le 0$ | `kNegative` | any integer $\le 0$ |
| `kZeroReal` | 0 (real) | `kZeroInteger` | 0 (integer) |
| `kUnitary` | any real in $[-1, 1]$ | `kTernary` | $-1$, 0 or 1 |
| `kPosUnitary` | any real in $[0, 1]$ | `kBinary` | 0 or 1 |
| `kNegUnitary` | any real in $[-1, 0]$ | `kNegBinary` | $-1$ or 0 |
| `kZeroRealU` | 0 (real, in $[-1, 1]$) | `kZeroIntU` | 0 (integer, in $[-1, 1]$) |

The type is composed of independent flags, set by
`is_integer( bool , ModParam )`, `is_positive( bool , ModParam )`,
`is_negative( bool , ModParam )` and `is_unitary( bool , ModParam )` (or all
at once by `set_type()`), and read by `get_type()`. Together with the state
"fixed or not", the types cover most of the restrictions of bound, sign and
integrality found in practical models. Since they are stored in the
`ColVariable` instead of as `Constraint`s of the `Block`, they save a
substantial amount of memory in models where most variables are, say, simply
nonnegative or binary.

## 5.3 Constraint {#sec-5-3}

The base class `Constraint`, beyond what its base classes provide,
supports *relaxation*: `relax( bool , ModParam )` temporarily disables
the `Constraint`, and `is_relaxed()` reads the current state. It also
supports *feasibility checking*, by which the pure virtual
`feasible()` returns `true` if the `Constraint` is satisfied by the
values of its active `Variable`s at the time of the last `compute()`
(which therefore has to be called first). `Constraint` is otherwise
abstract, i.e., it does not specify *what* it constrains.

### `RowConstraint`

[`RowConstraint`](https://smspp.gitlab.io/smspp-project/d1/dbf/class_s_m_spp__di__unipi__it_1_1_row_constraint.html)
specializes `Constraint` to the case of a real-valued expression
$g(x)$ of the active `Variable`s that has to lie between two bounds,
i.e., the canonical row of a linear (or nonlinear) program,

$$
l \le g(x) \le u \,,
$$

where $l$ and $u$ (of type `RHSValue`) are read by `get_lhs()` and `get_rhs()`
and set by `set_lhs( ... )`, `set_rhs( ... )` and `set_both( ... )` (the
latter sets $l = u$); either of them can be infinite, i.e., equal to
`RowConstraint::RHSINF` (or its opposite). Although a ranged row is two
inequalities, a single real dual variable is attached to the `Constraint`,
read by `get_dual()` and set by `set_dual( ... )`. Its value is the
coefficient of $g(x)$ in the Lagrangian function, i.e., for a minimization,
positive when the multiplier of the upper side is nonzero and negative when
that of the lower side is, as [§9.4](09-solution.md#sec-9-4) shows (together
with the representation of a dual direction). A `:Solver` that writes a dual
solution into a `RowConstraint` has to obey this convention. The expression
$g$, however, is not defined in `RowConstraint` but in its derived classes,
of which two cover the common cases.

### `OneVarConstraint`

[`OneVarConstraint`](https://smspp.gitlab.io/smspp-project/d1/d37/class_s_m_spp__di__unipi__it_1_1_one_var_constraint.html)
is the case where $g(x)$ is the value of a single `ColVariable`, i.e.,
a bound (or a pair of bounds) on one variable. The class has several
predefined refinements: `BoxConstraint` ($l \le x \le u$, both free),
`LB0Constraint` ($0 \le x \le u$), `UB0Constraint` ($l \le x \le 0$),
`LBConstraint` ($l \le x$), `UBConstraint` ($x \le u$), `NNConstraint`
($x \ge 0$), `NPConstraint` ($x \le 0$) and `ZOConstraint` ($0 \le x
\le 1$). `MCFBlock` uses `LB0Constraint` for the capacities of the
arcs (cf. [§7.1](07-physical-abstract.md#sec-7-1)).

### `FRowConstraint`

[`FRowConstraint`](https://smspp.gitlab.io/smspp-project/db/d90/class_s_m_spp__di__unipi__it_1_1_f_row_constraint.html)
is the case where $g(x)$ is computed by a `Function`, and it carries
essentially all the nontrivial linear or nonlinear constraints of the
framework. The `Function` is set by

```cpp
void set_function( Function * const function = nullptr ,
                   ModParam issueMod = eModBlck , bool deleteold = true );
```

and read by `get_function()`. The `Function` passed becomes property
of the `FRowConstraint`, which deletes it in its destructor; the
previous `Function`, if any, is deleted if `deleteold` is `true` (the
default), while with `deleteold == false` the caller is assumed to
hold another pointer to it and to dispose of it (under `eDryRun`,
instead, the call has no effect). Hence `set_function()` with no
argument empties the `FRowConstraint`, and deletes its `Function`
unless told otherwise. Since the `FRowConstraint` also registers with
the `Variable`s of the `Function` (and follows the changes of that set),
a `Function` cannot be shared between two `FRowConstraint`s.

In turn, the family of `Function`s (cf. [Chapter
13](13-function-family.md#ch-13)) covers the linear, quadratic and
polyhedral cases, plus the two hybrids of a `Block` and a `Function`,
`LagBFunction` and `BendersBFunction`. For a row of a Linear Program
the standard combination is an `FRowConstraint` carrying a
`LinearFunction`, as `MCFBlock` uses for its flow conservation
constraints.

## 5.4 Objective {#sec-5-4}

The base class `Objective` adds two facts to its base classes. First,
it has a *sense*, read by `get_sense()` and set by
`set_sense( int , ModParam )`, which is either `Objective::eMin`
(whose value is $-1$: minimization, lower values are better) or
`Objective::eMax` (whose value is $+1$: maximization), while
`Objective::eUndef` (0) means that the sense is not yet defined; a new
`Objective` is `eMin`. Here the values of the `Objective` are assumed
to be totally ordered, in which case minimizing and maximizing make
sense; a vector-valued objective would require extending the class.
Second, the type of the value is *not* defined in `Objective`, which
leaves room for multi-objective extensions; the derived class
`RealObjective` (and `FRealObjective` below) fixes it to a real
(`RealObjective::OFValue`, i.e., `double`, read by `value()`).

The rule by which the `Objective`s of the sub-`Block`s compose is
fixed at this level: the `Objective` of a sub-`Block` is added to that
of its father, which gives a tree of `Block`s a single, well-defined
`Objective`. This sum may be weighted, since a `Block` has a weight
of its `Objective` and one of its `Variable`s, applied by the
`:Solver`s that assemble a problem out of nested `Block`s
([§12.2](12-sub-block.md#sec-12-2)).

### `FRealObjective`

[`FRealObjective`](https://smspp.gitlab.io/smspp-project/dd/d7c/class_s_m_spp__di__unipi__it_1_1_f_real_objective.html)
is to `Objective` what `FRowConstraint` is to `Constraint`: its value
is computed by a `Function`, which is set by `set_function()` (with
the same ownership rules) and read by `get_function()`. It is the
standard choice for any `Block` whose `Objective` is described by a
`Function`, i.e., in practice, for essentially all the `Block`s with a
nontrivial objective. There is no separate class for a linear
objective: a linear form of `ColVariable`s is an `FRealObjective`
carrying a `LinearFunction`. On the side of the constraints,
`LinearConstraint` is an `FRowConstraint` whose `Function` is forced
to be a `LinearFunction`, which cannot be replaced (`set_function()`
throws `std::logic_error`) and which it exposes through
`get_linear_function()`, `get_coefficient()`, `add_variable()`,
`add_variables()`, `modify_coefficient()` and
`modify_coefficients()`.

## 5.5 Example: the abstract representation of `BinaryKnapsackBlock` {#sec-5-5}

The abstract representation of a concrete `:Block` is built *by the
`:Block`*, on demand, through its overrides of
`generate_abstract_variables()`, `generate_abstract_constraints()` and
`generate_objective()`. Code that uses a `:Block` triggers these
calls, but it does not fill in what they produce; constructing the
`ColVariable`s, the `FRowConstraint` and the `FRealObjective` by hand
is done with an `AbstractBlock` (see below). For
`BinaryKnapsackBlock`, the trigger is as small as the following.

```cpp
#include "BinaryKnapsackBlock.h"

using namespace SMSpp_di_unipi_it;

int main()
{
 // physical data: 4 items, knapsack of capacity 5
 BinaryKnapsackBlock bkb;
 const std::vector< double > W = { 2.0 , 3.0 , 4.0 , 1.0 };    // weights
 const std::vector< double > P = { 3.0 , 4.0 , 5.0 , 1.0 };    // profits
 const std::vector< bool > I = { true , true , true , true };  // integral
 bkb.load( W.size() , /* capacity = */ 5.0 , W , P , I );

 // construct the abstract representation on demand
 bkb.generate_abstract_variables();
 bkb.generate_abstract_constraints();
 bkb.generate_objective();
 // from this point on, bkb carries both the physical and the
 // abstract representation; the two are kept in sync by the
 // mechanisms covered in Chapter 7 and Chapter 8.

 return( 0 );
 }
```

The method used is

```cpp
void load( Index n , double Capacity , c_doubleVec & Weights ,
           c_doubleVec & Profits , c_boolVec & Integrality = {} ,
           const std::vector< unsigned char > fxd = {} );
```

where an empty `Integrality` makes all the items integer (a nonempty
one must have size $n$), and `fxd`, if not empty, fixes some of them,
`fxd[ i ]` being 0 if item $i$ is free, 1 if it is fixed to 0 and 2 if
it is fixed to 1. The `ColVariable`s are constructed by `load()`
itself (as for `MCFBlock`, cf. [§7.6](07-physical-abstract.md#sec-7-6)),
so that the first of the three calls returns at once. The other two
construct the rest of the objects that the comments of
`BinaryKnapsackBlock` describe, which are held in three protected fields
([`BinaryKnapsackBlock.h:1021-1023`](https://gitlab.com/smspp/binaryknapsackblock/-/blob/develop/include/BinaryKnapsackBlock.h#L1021-1023)). These are (i) one static group of
`Variable`s, the `std::vector< ColVariable > v_x` of size $n$ (one per
item), each of type `kBinary` for an integer item and `kPosUnitary`
for a continuous one; (ii) one static group of `Constraint`s, the
single `FRowConstraint f_cnst`, whose `LinearFunction` is $\sum_i W_i
x_i$, with right-hand side equal to the capacity $C$ and no lower
bound; and (iii) the `FRealObjective f_obj`, which carries the
`LinearFunction` $\sum_i P_i x_i$ and whose sense is `eMax` by default
(`bkb.set_objective_sense( false )` makes it `eMin`).

The three fields are accessible to a derived `:Block`, but they are
not part of the interface of `Block`. Code that uses a
`BinaryKnapsackBlock` reads it through the methods of the `:Block`
instead: the physical data through `get_Weight()`, `get_Profit()`,
`get_Capacity()` and their siblings, the solution through `get_x()`
and `get_objective_value()`, and the single `ColVariable`s, if needed,
through `get_Var( i )`. Code that has to look at the abstract
representation of a `:Block` it does not know iterates over its
groups, which `get_static_variable_groups()`,
`get_static_constraint_groups()` and their dynamic counterparts return
as `BaseGroup` objects ([`Group.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Group.h)), each of which gives its name,
its shape and its elements as `Variable` or `Constraint`. On such a
group, `elements_are< T >()` checks once whether the elements derive
from `T`, `for_each_as()` and `for_each_cell_as()` run loops typed on
the element, meant for the hot paths, and `for_each()` serves the
others. The order in which the elements are visited (the order of the
storage) is part of the contract.

`AbstractBlock` is the converse case: a `Block` *without* a specific
class of problems, whose abstract representation is constructed by the
code that uses it, typically when a `.mps` or `.lp` file is read. It
is the right vehicle to assemble a set of `Variable`s, `Constraint`s
and an `Objective` by hand, through `add_static_variable()`,
`add_static_constraint()` and their dynamic counterparts; the tests of
the core contain `AbstractBlock`s built in this way
([`SMS++/test/AbstractBlockGenerator.h`](https://gitlab.com/smspp/smspp/-/blob/develop/test/AbstractBlockGenerator.h), used by
[`tests_AbstractBlock.cpp`](https://gitlab.com/smspp/smspp/-/blob/develop/test/tests_AbstractBlock.cpp)), while [Appendix
A](A-writing-block.md#app-A) develops a new `:Block` from scratch.

## 5.6 Outline of the interface {#sec-5-6}

| Class | Main methods |
|---|---|
| `Variable` | `get_Block()`, `get_Group()`, `is_fixed()`, `is_fixed(bool, ModParam)`, `get_num_active()`, `get_active(Index)`, `is_active(...)` |
| `ColVariable` | `set_value(VarValue)`, `get_value()`, `is_integer(bool, ModParam)`, `is_positive(...)`, `is_negative(...)`, `is_unitary(...)`, `set_type(...)`, `get_type()` |
| `Constraint` | `relax(bool, ModParam)`, `is_relaxed()`, `feasible()`, `compute(bool)` |
| `RowConstraint` | `set_lhs`, `set_rhs`, `set_both`, `get_lhs`, `get_rhs`, `get_dual`, `set_dual` |
| `OneVarConstraint` | the single `ColVariable` and the bounds are the only data |
| `FRowConstraint` | `set_function(Function*, ModParam, bool)`, `get_function()`; $g$ is the value of the `Function` |
| `Objective` | `set_sense(int, ModParam)`, `get_sense()`, `compute(bool)` |
| `RealObjective` | `OFValue` and `value()` (in [`Objective.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Objective.h)) |
| `FRealObjective` | `set_function(Function*, ModParam, bool)`, `get_function()` |
| `LinearConstraint` | `get_linear_function()`, `get_coefficient(Index)`, `add_variable[s](...)`, `modify_coefficient[s](...)`; `set_function()` throws |

For the complete list of the public methods, one can see the Doxygen page of
each class.

## 5.7 Idioms {#sec-5-7}

Wherever a `Variable *` or a `Constraint *` appears, it is compared
with `==`: two pointers to the same address are the same object, and
this is the only sense of equality that the framework recognizes. In
particular, two `ColVariable`s with the same value are *not* the same
variable, and copying a `ColVariable` into another slot of a vector
makes a new, distinct `ColVariable`.

`set_function()` takes ownership of the `Function`
([§5.3](05-variable-constraint-objective.md#sec-5-3)), which therefore
can be given to one carrier only.

When a sub-`Block` has an `Objective`, it contributes to that of its
father (and recursively upwards), possibly weighted. A `Block` can
therefore carry no `Objective` and still expose a nontrivial overall
`Objective`, made of those of its sub-`Block`s; this is the standard
pattern of the "master" `Block`s that only carry linking constraints,
as `CapacitatedFacilityLocationBlock` in its knapsack formulation
([Chapter 12](12-sub-block.md#ch-12), [Recipe
R4](R4-cfl-lagrangian.md#rec-R4)).
