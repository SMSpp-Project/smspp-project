# 15. The methods factory {#ch-15}

[Source: [`SMS++/include/Block.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Block.h) (methods factory),
[`SMSFactory.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/SMSFactory.h),
[`DataMapping.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/DataMapping.h);
[`BinaryKnapsackBlock/include/BinaryKnapsackBlock.h`](https://gitlab.com/smspp/binaryknapsackblock/-/blob/develop/include/BinaryKnapsackBlock.h),
[`UCBlock/include/UCBlock.h`](https://gitlab.com/smspp/ucblock/-/blob/develop/include/UCBlock.h),
[`InvestmentBlock/src/InvestmentFunction.cpp`](https://gitlab.com/smspp/investmentblock/-/blob/develop/src/InvestmentFunction.cpp)]

We are concerned in this chapter with the *methods factory*, i.e., the
mechanism by which a generic driver (a configuration file, a scenario
generator, the realization of a scenario in a `StochasticBlock`) changes, or
reads, the data of a `:Block` whose concrete type it does not know at compile
time, by calling a function that it retrieves by *name*. The `Configuration`
machinery of [Chapter 11](11-configuration.md#ch-11) can tell a `Block` which
parts of its representation to build and which `:Solver` to attach; however,
it cannot reach inside a `:Block` and change a weight, a cost or a capacity
(at least, not without knowing the concrete type), and the methods factory
fills this gap.

We first state the problem ([§15.1](15-methods-factory.md#sec-15-1)) and
describe the adapter functions on which the factory rests
([§15.2](15-methods-factory.md#sec-15-2)). Then we list the standard
signatures that select a factory ([§15.3](15-methods-factory.md#sec-15-3)),
and we give the operations on it, with the invariant they keep and the
contract of the setters and of the queries that a `:Block` registers
([§15.4](15-methods-factory.md#sec-15-4)). In the examples of
[§15.5](15-methods-factory.md#sec-15-5) we go from the change of the weights
of a knapsack to a consumer that works only through setters and queries found
by name. Last, we compare the methods factory with the class factories
([§15.6](15-methods-factory.md#sec-15-6)), we discuss when a method is in the
factory and why the linker may drop it
([§15.7](15-methods-factory.md#sec-15-7)), and we collect the idioms
([§15.8](15-methods-factory.md#sec-15-8)).

## 15.1 The problem it solves {#sec-15-1}

The data-changing methods of a `:Block` live in its specialized interface,
e.g., `MCFBlock::chg_cost( ... )` or `BinaryKnapsackBlock::chg_weight( ...
)`, and calling one of them requires a pointer of the concrete type, hence a
compile-time dependency on that type. Yet, much code of the framework is
deliberately type-agnostic. A `StochasticBlock` that writes the data of a
scenario into "some inner `Block`" ([Chapter 1](01-introduction.md#ch-1))
does not know, and should not need to know, whether that inner `Block` is a
`UCBlock`, a `CapacitatedFacilityLocationBlock` or a class written next year:
it only knows that "the data to change is called such-and-such, and here are
the new values".

This indirection is provided by the methods factory, a (bidirectional) map
from a *name*, i.e., a `std::string` that can be read at run time (e.g., from
a configuration file), to a *pointer to a function that changes a `Block`*.
With it, type-agnostic code retrieves the function by its name and calls it
on a base `Block *`, without ever naming the concrete `:Block` class.

## 15.2 Adapter functions over a base `Block*` {#sec-15-2}

The functions in the factory do not point to member functions of a `:Block`,
since a pointer to a member function is tied to the concrete class and would
defeat the purpose. Instead, each entry is an *adapter*, a function whose
first parameter is a base `Block *`, which casts the pointer to the concrete
`:Block` and forwards the call to the real method
([`Block.h:6511-6525`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Block.h#L6511-6525)
for a setter,
[`Block.h:6568-6580`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Block.h#L6568-6580)
for a query). Their types are

```cpp
template< typename... Args >
using FunctionType =
  std::function< void( Block * , Args... , ModParam , ModParam ) >;

template< typename... Args >
using QueryType = std::function< void( const Block * , Args... ) >;
```

and the framework builds the adapter for a member function that already has
the right shape (cf. [§15.4](15-methods-factory.md#sec-15-4)). One can also
write an adapter by hand to bridge any method of a `:Block` (even one that
changes a single item at a time), but the result must have one of the standard
signatures of [§15.3](15-methods-factory.md#sec-15-3), since these are the
only types for which a factory exists
([§15.4](15-methods-factory.md#sec-15-4)); it is allocated with `new` and
handed to the factory, which owns it.

A setter carries, after its data arguments, the usual two `ModParam`s
(`issuePMod` and `issueAMod`). Hence, a change made through the factory
follows the discipline of the physical and abstract `Modification`s of
[Chapter 8](08-modification-janus.md#ch-8) as a direct call does.

## 15.3 The standard signature families {#sec-15-3}

The factory is useful only if many `:Block`s register their methods in the
same few shapes, and SMS++ therefore predefines 18 families of signatures
([`Block.h:723-777`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Block.h#L723-777)).
Six basic ones are given by two independent choices: (i) the type of the data,
i.e., none, `MF_dbl_sp` (a `std::span< const double >`) or `MF_int_sp` (a
`std::span< const int >`), and (ii) the form of the slice, which selects the
elements that change, i.e., a `Range` (a contiguous interval `[start , stop)`)
or a `Subset` (an arbitrary set of indices, with a `bool` that is true if it
is ordered).

<span id="tab-15-1"></span>**Table 15.1.** The six basic setter families, and
the arguments that each takes before the two `ModParam`s.

| Type | Arguments encoded |
|---|---|
| `MS_rngd`     | `Range` |
| `MS_sbst`     | `Subset &&, bool` |
| `MS_sp_dbl_rngd` | `MF_dbl_sp, Range` |
| `MS_sp_dbl_sbst` | `MF_dbl_sp, Subset &&, bool` |
| `MS_sp_int_rngd` | `MF_int_sp, Range` |
| `MS_sp_int_sbst` | `MF_int_sp, Subset &&, bool` |

A function of the family `MS_sp_dbl_rngd` thus has the shape
`(Block*, MF_dbl_sp data, Range slice, ModParam, ModParam)`, and it writes the
$h$-th value of `data` into the $h$-th index of the range `slice` of the data
it changes. Of course, the span has to be at least as long as the slice:
the contract of the families
([`Block.h:6189-6194`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Block.h#L6189-6194))
requires the function to throw `std::invalid_argument` for a shorter one,
instead of reading past its end. `BinaryKnapsackBlock` and `MCFBlock` do so,
after clipping the slice to the existing items. This is the shape of
`BinaryKnapsackBlock::chg_weights( MF_dbl_sp , Range , ModParam , ModParam )`,
which `BinaryKnapsackBlock` registers under the name
`"BinaryKnapsackBlock::chg_weights"`. It registers `chg_weights` and
`chg_profits` in both the families `MS_sp_dbl_rngd` and `MS_sp_dbl_sbst`, and
`chg_capacity` in `MS_sp_dbl_rngd`; `MCFBlock` does the same with `chg_costs`,
`chg_ucaps` and `chg_dfcts` in the families `MS_sp_dbl_*`, and with
`close_arcs` and `open_arcs` in the families `MS_*`. Hence, one can register
the data-changing methods of a `:Block` almost for free if they have one of
these shapes.

Four more setter families cover the data that do not fit the six basic
shapes. `MS_sp_dbl2_rngd` and `MS_sp_dbl2_sbst` take two `MF_dbl_sp` before
the slice, for items that carry two values of the same kind (the $h$-th value
of each span going to the $h$-th index of the slice). `MS_dbl` and `MS_dbl2`
take one or two `double` and no slice, for a parameter that is a single
scalar (say, the size of a unit). Also, the factory holds *queries*, i.e.,
adapters of `const` member functions that read data instead of changing
them. Their type is `QueryType< Args... >`, a function of a `const Block *`
with no `ModParam`, and the eight families `MS_qry_dbl_rngd`,
`MS_qry_dbl_sbst`, `MS_qry_int_rngd`, `MS_qry_int_sbst`, `MS_qry_dbl2_rngd`,
`MS_qry_dbl2_sbst`, `MS_qry_dbl` and `MS_qry_dbl2` mirror the setters: they
write the answer through a mutable span (`MF_dbl_msp` or `MF_int_msp`, the
$h$-th value for the $h$-th index of the slice) or into a `double &`, and
they take a `c_Subset &`, which is not consumed, in place of the
`Subset &&`.

`UCBlock` uses half of them
([`UCBlock.h:2883-3041`](https://gitlab.com/smspp/ucblock/-/blob/develop/include/UCBlock.h#L2883-3041)).
It registers `"UCBlock::resize_unit"` in four setter families, with one span
or two (for the units whose size is a pair) and with a `Subset` or a `Range`,
and the getter `"UCBlock::get_resize_unit_linearization"` in the four query
families of the same shapes. Furthermore, the getter
`"UCBlock::get_replicate_linearization"` goes with the setter
`"UCBlock::replicate"`, and `"UCBlock::has_design"` is an `MS_qry_dbl` that
answers 1 or 0. The lines are resized in the same way, by
`"UCBlock::resize_line"` and `"UCBlock::get_resize_line_linearization"`, and
the setters `"UCBlock::set_active_power_demand"`,
`"UCBlock::set_energy_demand"`, `"UCBlock::set_pollutant_budget"` and
`"UCBlock::set_pollutant_min_budget"` change the demands and the budgets. Note
that a name may be registered in several families: there is one factory per
type, and a name has to be unique only within one of them.

## 15.4 `register_method`, `get_method`, `get_method_name` {#sec-15-4}

The three operations on the factory are static members of `Block`:

```cpp
template< class F >
static void register_method( std::string && name, F * function );
template< class F >
static F * get_method( const std::string & name );
template< class F >
static const std::string & get_method_name( F * fnct );
```

where `F`, the type of the function, selects the factory. `F` must be one of
the 18 standard types of [§15.3](15-methods-factory.md#sec-15-3), i.e.,
`FunctionType< Args... >` or `QueryType< Args... >` with the `Args` of one of
the families. In fact, the accessor of the map of a type is only declared in
`Block.h` and is defined only by its 18 specializations, and therefore any
other `F` is a compile error, instead of a new and empty factory in which a
lookup would fail without notice
([`Block.h:8537-8561`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Block.h#L8537-8561)).
Each specialization defers to a function defined once in `Block.cpp`, which
holds the map; hence, there is one map per family in the program, even when
the modules are shared libraries (a static variable of a function template in
a header is unique only insofar as the linker merges its copies, which is not
granted across shared libraries). A new family is thus a change of the core:
its map, the specialization of the accessor and the function in `Block.cpp`.

A second form, `register_method< dBlock , Args ... >( name , & dBlock::method
)`, builds the adapter of a member function of `dBlock`, a setter or, if the
member function is `const`, a query. Its `Args` are those given, and they are
never deduced, which allows the member function to be overloaded; a third
form takes the family as a final dummy argument, e.g.,
`Block::MS_sp_dbl_rngd::args()`.

`register_method()` associates `name` with `function`, and it throws
`std::invalid_argument` if `name` is empty. The factory takes ownership of
`function`, which must therefore be allocated with `new` (as the forms with a
member function do) and never deleted by the caller. A function already held
in the factory, under `name` or under another name, is left where it is and
the call does nothing; otherwise, a later registration under the same name
replaces the earlier one and then deletes the function it held, so that a
pointer obtained before from `get_method()` is no longer valid. `get_method()`
returns the function registered under `name`, or `nullptr` if there is none,
while `get_method_name()` is the reverse lookup, which returns the empty
string for a function that is not in the factory. With $M_F$ the map of the
type $F$ of the function, registration reads as follows.

<span id="alg-15-1"></span>**Algorithm 15.1** (registration of a function
$f$ under a name $n$).

```text
1  procedure REGISTER(n, f)               ▷ M_F, the factory of the type of f
2     if n is the empty string then throw invalid_argument
3     if f is a function of M_F then return
4     if n is a name of M_F then
5        g ← M_F(n);  M_F(n) ← f;  delete g
6     else
7        insert the pair (n, f) in M_F
```

This is `register_method< F >()` in
[`Block.h:6466-6480`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Block.h#L6466-6480)
(since SMS++ 0.9.0 for line 3 and for the order of line 5, which before
deleted the old function first); the forms with a member function wrap it
into an adapter before line 1. Each factory is a bimap, i.e., a bijection
between the names and the functions it holds, which gives the following.

<span id="lem-15-1"></span>**Lemma 15.1.** *After any sequence of calls to
`register_method()` for one type `F`, `get_method_name( get_method( n ) ) ==
n` for every registered name `n`, and `get_method( get_method_name( f ) ) ==
f` for every registered function `f`.*

*Proof.* By induction on the number of calls. The empty factory satisfies the
claim vacuously. A call with the empty name throws at line 2, before touching
the map. A call whose function is already in the factory changes nothing, by
line 3 of [Algorithm 15.1](15-methods-factory.md#alg-15-1). Any other call has
a function $f$ held under no name: a new name inserts the pair (`n`, `f`) at
line 7, and a name already present has its function replaced at line 5 by $f$,
the function deleted being the one just removed from the map; in both cases
the map stays a bijection. Both identities hold for any bijection between two
sets. $\square$

A function registered a second time under a new name keeps its first name,
since line 3 returns without notice; hence, a function meant to be found
under two names has to be registered as two distinct objects, as the forms
with a member function do by allocating a new adapter at each call.

The variants `get_method_fs()` and `get_method_name_fs()` select the factory
by the parameter type list of the function, given either as template
arguments or by a final dummy argument `MS_...::args()` (cf. the first
example of [§15.5](15-methods-factory.md#sec-15-5)), and `get_query_fs()`
does the same for the query factories.

Registration is usually done in the protected `static_initialization()` of
the `:Block`, and the methods of the `:Block` are then in the factory as soon
as the class is loaded ([§15.7](15-methods-factory.md#sec-15-7)). However,
the three operations are public, and any code with access to a `:Block` may
register further adapters, e.g., when the author of the `:Block` did not
foresee a particular form of bulk change
([`Block.h:6331-6348`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Block.h#L6331-6348)).
We remark that `static_initialization()` may run more than once, i.e., again
for each derived class that does not redefine it
([`Block.h:8184-8216`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Block.h#L8184-8216)).
Since a new adapter replaces, and deletes, the one registered under the same
name, a `static_initialization()` that registers methods should do so only at
its first call (e.g., in the initialization of a static local variable);
otherwise, a pointer obtained before from `get_method()` becomes invalid. The
`:Block`s of [§15.3](15-methods-factory.md#sec-15-3) register at every call,
which is harmless since no class of the distribution derives from them.

### The contract of a setter and of a query

Since the factory is consulted by code that does not know the `:Block`, what a
name promises has to be fixed once and for all; the core fixes it as a
contract for the `:Block` that registers a function
([`Block.h:6350-6430`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Block.h#L6350-6430)).

- Several writers may act on the same datum, in no fixed order. Hence, a
  setter recomputes the data it owns out of the fields of the `:Block`, and it
  never increments the values it finds there; the fields keep the base datum,
  and what a getter reads back is assembled out of them (an increment would
  make the outcome depend on which writer acted last).
- A consumer that has to know whether a `:Block` supports an operation looks
  its name up in the factory, and the name either resolves or it does not: a
  `:Block` registers a name if and only if it supports what the name promises,
  and no method is run to find out.
- A call can still be refused, say, for a value that the `:Block` cannot
  represent, and this is reported by throwing. The two cases are disjoint: a
  missing name means that the operation is not supported, an exception means
  that this one call is wrong.
- A query may require state, typically a solved sub-tree and a `:Solver` that
  gives the multipliers it reads, and the `:Block` that registers it declares
  which. It also declares whether, and with what, the query answers when the
  solve has ended infeasible, since an unbounded dual direction gives no
  derivative, but the coefficients of a condition on the parameters; read
  elsewhere, a query gives back what the previous solve left, and no error is
  reported.
- That a setter and the query mirroring it address the same parameters, in the
  same number and order, is a convention like the naming one, which the
  factory does not check.

## 15.5 Examples {#sec-15-5}

### Changing weights by name

The following function changes the data of any `:Block` that has registered
an `MS_sp_dbl_rngd` setter under the given name.

```cpp
// type-agnostic code: it only has a base Block * and a string
void apply_new_doubles( Block * blk , const std::string & method_name ,
                        const std::vector< double > & values ,
                        Block::Range slice )
{
 auto fun = Block::get_method_fs( method_name ,
                                  Block::MS_sp_dbl_rngd::args() );
 if( ! fun )
  throw( std::invalid_argument( "no such method: " + method_name ) );

 // changes the data of the Block and issues the Modification(s), as a
 // direct call would
 std::invoke( *fun , blk , Block::MF_dbl_sp( values ) , slice , eModBlck ,
              eModBlck );
 }
```

If `bkb` is a `BinaryKnapsackBlock` with three items, loaded in memory, one
can write

```cpp
apply_new_doubles( &bkb , "BinaryKnapsackBlock::chg_weights" ,
                   { 5.0 , 6.0 , 7.0 } , Block::Range( 0 , 3 ) );
```

to change the weights of the three items. Note that `apply_new_doubles` has
no compile-time dependency on `BinaryKnapsackBlock`: with an `MCFBlock` and
the name `"MCFBlock::chg_costs"`, it would change the costs of the arcs. Its
name follows the recommended convention `ClassName::method_name`, and it is
the string that a configuration file or a scenario generator carries.

### Scenario data: `SimpleDataMapping`

The `DataMapping`s of a `StochasticBlock` write the data of a scenario into an
inner `:Block` in this way. A `SimpleDataMapping< SetFrom , SetTo , DataType
>` writes a part of the data of a scenario through a setter of type
`FunctionType< std::span< const DataType > , SetTo >`, or `FunctionType<
std::span< const DataType > , SetTo && , bool >` when `SetTo` is a `Subset`
([`DataMapping.h:828-830`](https://gitlab.com/smspp/smspp/-/blob/develop/include/DataMapping.h#L828-830)),
i.e., through a setter of one of the standard families, and its `set_data()`
takes the data as a `std::span< const double >`. When the mapping is
serialized it stores the name of its function (`get_method_name()`), and when
it is read back it retrieves the function by that name (`get_method()`); a
netCDF file can therefore describe the scenarios of a `:Block` of any type.

### A consumer of setters and queries: `InvestmentFunction`

The `InvestmentFunction` of `InvestmentBlock` is the value of a problem in
which some assets of an inner `Block` (e.g., the units and the lines of a
`UCBlock`) are resized or replicated. It changes their size through setters,
and it reads the coefficients of its linearizations back through queries, both
found by name. Which setter applies to a unit is decided by a capability test,
as in the second point of the contract: if the factory has a setter `"<class
of the unit>::resize"` in the family `MS_dbl`, the size of the unit is changed
by `"UCBlock::resize_unit"` and read back by
`"UCBlock::get_resize_unit_linearization"`, and otherwise the unit is
replicated by `"UCBlock::replicate"` and read back by
`"UCBlock::get_replicate_linearization"`
([`InvestmentFunction.cpp:1822-1831`](https://gitlab.com/smspp/investmentblock/-/blob/develop/src/InvestmentFunction.cpp#L1822-1831)).
These functions are resolved once, when the assets are grouped, and a missing
one is an error that names the form in which it was looked for
([`InvestmentFunction.cpp:2163-2189`](https://gitlab.com/smspp/investmentblock/-/blob/develop/src/InvestmentFunction.cpp#L2163-2189)):

```cpp
g.setter = Block::get_method_fs< Block::MF_dbl_sp , Block::Subset && ,
                                 bool >( g.setter_name );
g.linearization = Block::get_query_fs< Block::MF_dbl_msp ,
                                       Block::c_Subset & , bool >
                   ( g.linearization_name );
```

After each solve of the inner `Block`, the coefficients of a linearization are
read by calling the query on the indices of the group
([`InvestmentFunction.cpp:1979-1980`](https://gitlab.com/smspp/investmentblock/-/blob/develop/src/InvestmentFunction.cpp#L1979-1980)),

```cpp
std::invoke( *group.linearization , piece , Block::MF_dbl_msp( answer ) ,
             group.indices , group.ordered );
```

which requires, as in the fourth point of the contract, that the inner
`Block` has just been solved by a `:Solver` that gives its dual solution.
`InvestmentFunction` never names `UCBlock` as a type, since the names of the
functions are strings; a `:Block` of another class that registered functions
with the same names and shapes would be resized in the same way.

## 15.6 Relation to the class-name factories {#sec-15-6}

The methods factory is one of two distinct factory mechanisms of SMS++, which
should not be confused:

- the methods factory of this chapter maps the *name of a method* to a
  *function that changes (or reads) an existing `Block`*;
- the class factories of [Chapter 18](18-factories-netcdf.md#ch-18) map the
  *name of a class* to a *constructor*, so that a `Block`, `Solver`,
  `Configuration`, `Solution`, `Change` or `State` of a type named in a file
  can be created without compile-time knowledge of the type (e.g.,
  `Block::new_Block( name )` or `Solver::new_Solver( name )`).

Both defer a decision on a type to a string read at run time; however, one
constructs objects, while the other changes an existing one. The
`StochasticBlock` of [§15.1](15-methods-factory.md#sec-15-1) uses the methods
factory to write the data of a scenario into the inner `Block` it was handed,
and it would use the class factory to build that inner `Block` out of a
serialized description.

## 15.7 When a method is in the factory {#sec-15-7}

A method is registered by the `static_initialization()` of its `:Block`. This
is called by the macros `SMSpp_insert_in_factory_cpp_*` that put the `:Block`
in the class factory, i.e., when the static initializers of the translation
unit of the `:Block` run, at the start of the program or when the library that
holds the `:Block` is loaded
([`SMSFactory.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/SMSFactory.h),
[`Block.h:8184-8216`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Block.h#L8184-8216)).
Hence, whoever holds an object of a `:Block` also has its methods in the
factory, since the code of the `:Block` has been loaded to build the object. A
`:Block` that is not linked to the program, but is held by a *plugin* (since
SMS++ 0.9.0), is loaded when `Block::new_Block()`, `Solver::new_Solver()` or
`Solver::has_Solver()` is called with the name of a class that the program
does not have ([Chapter 18](18-factories-netcdf.md#ch-18)). Instead,
`get_method()` never loads a plugin, and the methods of a `:Block` that has
not been loaded yet are not in the factory. Also, the methods factory is not
protected by the lock of the class factories: `get_method()` must not run
concurrently with the loading of a plugin, i.e., with a `new_Block()`,
`new_Solver()` or `has_Solver()` that may load one.

The translation unit that registers a method receives no call from the rest
of the program, which only looks up a name such as
`"BinaryKnapsackBlock::chg_weights"` in the factory, and a linker that builds
the program out of static libraries may drop it; then the registration does
not happen, and the lookup of the name fails, although the code is in the
source tree. Why, and how this is prevented, is the subject of
[§18.2](18-factories-netcdf.md#sec-18-2) and
[§18.7](18-factories-netcdf.md#sec-18-7).

## 15.8 Idioms {#sec-15-8}

*Data-changing methods in the standard shapes.* An author of a `:Block` who
writes the methods `chg_*()` in the shapes `MS[_sp_dbl/_sp_int]_[rngd/sbst]`
(or `MS_[rngd/sbst]` with no data, [§15.3](15-methods-factory.md#sec-15-3))
makes the `:Block` usable by the type-agnostic machinery (scenario generation,
meta-configuration, distributed drivers). A method with another signature can
still be registered, but only through an adapter written by hand into one of
the standard signatures.

*Registration in `static_initialization()`.* The methods are registered there,
at its first call only if a derived class may inherit it
([§15.4](15-methods-factory.md#sec-15-4)), and registering elsewhere should be
the exception, for the cases that the author did not foresee.

*The `ClassName::method_name` naming.* It avoids collisions between the
names ([§15.5](15-methods-factory.md#sec-15-5)).

*A getter for each setter whose effect is read back.* A consumer that changes
a parameter and needs the sensitivity of the value to it (as
`InvestmentFunction` does) finds both functions by name. Registering the
query next to the setter, in the mirrored family and with the parameters in
the same order, makes the `:Block` usable by such a consumer.
