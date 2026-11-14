# 15. The methods factory {#ch-15}

[Source: `SMS++/include/Block.h` (methods factory),
`BinaryKnapsackBlock/include/BinaryKnapsackBlock.h`]

The `Configuration` machinery of [Chapter 11](11-configuration.md#ch-11) can tell a `Block`
*which* parts of its representation to build and *which* `:Solver`
to attach, but it cannot, by itself, reach inside a `:Block` and
*change its data* (a weight, a cost, a capacity) without knowing the
concrete `:Block` type at compile time. The *methods factory* is the
mechanism that closes this gap: it lets a
generic driver (a configuration file, a scenario generator, a
stochastic-block realiser) invoke a data-changing member function
of an unknown `:Block` by *name*.

## 15.1 The problem it solves {#sec-15-1}

A `:Block`'s data-changing methods live in its *specialised*
interface: `MCFBlock::chg_cost(...)`,
`BinaryKnapsackBlock::chg_weight(...)`,
`CapacitatedFacilityLocationBlock::...`. Calling any of them
requires a pointer of the concrete type and a compile-time
dependency on that type. But much framework code is deliberately
*type-agnostic*: a `StochasticBlock` that realises scenario data
into "some inner `Block`" ([Chapter 1](01-introduction.md#ch-1)) does not know, and should
not need to know, whether that inner `Block` is a `UCBlock`, a
`CapacitatedFacilityLocationBlock`, or something written next
year. It only knows that "the data to change is called such-and-
such, and here are the new values".

The methods factory provides exactly this indirection: a
(bi-directional) map from a *name*, a `std::string` readable at
runtime (e.g., from a configuration file), to a *pointer to a function
that changes a `Block`*. With it, type-agnostic code can
retrieve the function by name and call it on a base `Block*`,
without ever naming the concrete `:Block` class.

## 15.2 Adapter functions over a base `Block*` {#sec-15-2}

The functions stored in the factory do *not* point directly to
`:Block` member functions, because a member function pointer is
tied to the concrete class and would defeat the purpose. Instead,
each entry is an *adapter* function whose first parameter is a
base `Block*`; the adapter `static_cast`s the pointer to the
concrete `:Block` and forwards to the real method
(`Block.h:6048-6090`). Their types are

```cpp
template< typename ... Args >
using FunctionType =
 std::function< void( Block * , Args ... , ModParam , ModParam ) >;

template< typename ... Args >
using QueryType = std::function< void( const Block * , Args ... ) >;
```

and the framework can build these adapters
automatically for member functions that already have the right
shape, or a user can write one by hand to bridge any existing
`:Block` method (even one that changes a single item at a time)
to a factory-compatible signature.

The generic adapter signature carries, after the data arguments,
the familiar two `ModParam`s (`issuePMod`, `issueAMod`) so that
the factory-driven change participates in the Janus discipline of
[Chapter 8](08-modification-janus.md#ch-8) exactly as a direct call would.

## 15.3 The standard signature families {#sec-15-3}

To make the factory useful, the data-changing functions need a
small, shared vocabulary of signatures, so that the factory is
densely enough populated to be worth consulting. SMS++ predefines
a few "method-set" signature families (`Block.h:723-777`); the six
basic ones are parameterised by two independent choices:

- the *data type* carried: none, `MF_dbl_sp` (a
  `std::span<const double>`), or `MF_int_sp` (a
  `std::span<const int>`);
- the *slice form* addressing *which* elements change: a
  `Range` (a contiguous `[start, stop)` interval) or a `Subset`
  (an arbitrary set of indices, with a `bool` flag saying whether
  it is already ordered).

The six resulting types are:

| Type | Arguments encoded |
|---|---|
| `MS_rngd`     | `Range` |
| `MS_sbst`     | `Subset &&, bool` |
| `MS_sp_dbl_rngd` | `MF_dbl_sp, Range` |
| `MS_sp_dbl_sbst` | `MF_dbl_sp, Subset &&, bool` |
| `MS_sp_int_rngd` | `MF_int_sp, Range` |
| `MS_sp_int_sbst` | `MF_int_sp, Subset &&, bool` |

A function in the `MS_sp_dbl_rngd` family thus has the shape
"`(Block*, MF_dbl_sp data, Range slice, ModParam, ModParam)`":
"apply the `double` values in `data`, the h-th to the h-th index,
to the contiguous range `slice` of the relevant data structure".
The span has to be at least as long as the slice, and a shorter
one makes the function throw `std::invalid_argument` rather than
read past its end. This is precisely the shape of
`BinaryKnapsackBlock::chg_weights(MF_dbl_sp, Range, ModParam,
ModParam)`, which `BinaryKnapsackBlock` registers into the factory
under the name `"BinaryKnapsackBlock::chg_weights"`. It registers
`chg_weights` and `chg_profits` in both the `MS_sp_dbl_rngd` and
the `MS_sp_dbl_sbst` families, and `chg_capacity` in
`MS_sp_dbl_rngd`. `MCFBlock` does the same for its own data, with
`chg_costs`, `chg_ucaps` and `chg_dfcts` in the `MS_sp_dbl_*`
families and `close_arcs` and `open_arcs` in the `MS_*` ones. The framework nudges
`:Block` authors to give their data-changing methods one of these
shapes, precisely so they slot into the factory for free.

Four more setter families cover data that do not fit the six
basic shapes: `MS_sp_dbl2_rngd` and `MS_sp_dbl2_sbst` take two
`MF_dbl_sp` before the slice, for items that carry two values of
the same kind (the h-th value of each span going to the h-th index
of the slice), while `MS_dbl` and `MS_dbl2` take one or two
`double` and no slice at all, for a parameter that is a single
scalar (say, the size of a unit). Finally, the factory also holds
*queries*, i.e., adapters of `const` member functions that *read*
data rather than change them: their type is
`QueryType< Args... >`, a function of a `const Block *` with no
`ModParam`, and the families `MS_qry_dbl_rngd`, `MS_qry_dbl_sbst`,
`MS_qry_int_rngd`, `MS_qry_int_sbst`, `MS_qry_dbl2_rngd`,
`MS_qry_dbl2_sbst`, `MS_qry_dbl` and `MS_qry_dbl2` mirror the
setters, writing the answer through a mutable span (`MF_dbl_msp`
or `MF_int_msp`, the h-th value for the h-th index of the slice)
or into a `double &`, and taking a `c_Subset &`, which is not
consumed, in place of the `Subset &&`.

## 15.4 `register_method`, `get_method`, `get_method_name` {#sec-15-4}

The three operations on the factory are static members of `Block`:

```cpp
template< class F >
static void register_method( std::string && name , F * function );
template< class F >
static F * get_method( const std::string & name );
template< class F >
static const std::string & get_method_name( F * fnct );
```

where `F` is the type of the function, which selects the factory (one
per type), and a second form
`register_method< dBlock , Args ... >( name , & dBlock::method )`
builds the adapter of a member function of `dBlock` by itself.
`register_method()` associates `name` with `function` (it throws
`std::invalid_argument` if `name` is empty); a later registration under
the same name replaces the earlier one and deletes the function it
held, so that a pointer obtained before from `get_method()` is then
dangling. `get_method()` returns the function registered under `name`,
or `nullptr` if there is none, and `get_method_name()` is the reverse
lookup. Each factory is a bimap, i.e., a bijection between the names
and the functions it holds, which gives the following.

**Lemma 15.1.** *After any sequence of calls to `register_method()` for
one type `F`, in which no function pointer is registered under two
names, `get_method_name( get_method( n ) ) == n` for every registered
name `n`, and `get_method( get_method_name( f ) ) == f` for every
registered function `f`.*

*Proof.* By induction on the number of calls. The empty factory
satisfies the claim vacuously. A call with a new name inserts the pair
(`n`, `f`), and since `f` is under no other name the map stays a
bijection; a call with a name already present replaces the function of
that pair, and the map again stays a bijection by the same assumption.
The two identities hold for every bijection between two sets. $\square$

The assumption is necessary: registering the same pointer under two
names violates the uniqueness of the right side of the bimap, and the
second registration does not take effect.

The variants `get_method_fs()` and `get_method_name_fs()` select
the factory by the parameter type list of the function, given
either as template arguments or by a final `MS_..._S::args()`
dummy parameter (as in the example below); `get_query_fs()` does
the same for the query factories.

Registration is conventionally done once, in the `:Block`'s
protected `static_initialization()` function, so that the
`:Block`'s data-changing methods are available in the factory as
soon as the class is loaded. The methods are nonetheless *public*,
so any code with access to a `:Block` may register additional
adapter functions, which is useful when the `:Block` author did not
anticipate a particular form of bulk change, or simply did not get
around to registering one (`Block.h:6282-6299`).

## 15.5 Example: changing weights by name {#sec-15-5}

```cpp
#include "BinaryKnapsackBlock.h"

#include <functional>

using namespace SMSpp_di_unipi_it;

// type-agnostic code: it only has a base Block * and a string
void apply_new_doubles( Block * blk , const std::string & method_name ,
                        const std::vector< double > & values ,
                        Block::Range slice )
{
 // the adapter registered under method_name in the MS_sp_dbl_rngd
 // factory, with no knowledge of the concrete :Block type
 auto fun = Block::get_method_fs( method_name ,
                                  Block::MS_sp_dbl_rngd::args() );
 if( ! fun )
  throw( std::invalid_argument( "no such method: " + method_name ) );

 // this changes the data of the Block and issues the Modification(s),
 // as a direct call would
 std::invoke( *fun , blk , Block::MF_dbl_sp( values ) , slice ,
              eModBlck , eModBlck );
 }

int main( void )
{
 // a knapsack with 3 items, capacity 10, binary items
 BinaryKnapsackBlock bkb;
 bkb.load( 3 , 10 , { 1 , 2 , 3 } , { 4 , 5 , 6 } , { true , true , true } );

 // change the weights of items [ 0 , 3 ) without naming the class
 Block * anon = & bkb;          // seen only as a base Block *
 apply_new_doubles( anon , "BinaryKnapsackBlock::chg_weights" ,
                    { 5.0 , 6.0 , 7.0 } , Block::Range( 0 , 3 ) );
 return( 0 );
 }
```

The function `apply_new_doubles` has no compile-time dependency on
`BinaryKnapsackBlock`: it works for *any* `:Block` that has
registered an `MS_sp_dbl_rngd` method under the given name. Passing
`"MCFBlock::chg_costs"` instead, with an `MCFBlock`, would change
arc costs through the very same function. The string follows the
recommended naming convention (`ClassName::method_name`) and is
exactly what a configuration file or a scenario generator would
carry; this is how the `DataMapping`s of a `StochasticBlock`
(`DataMapping.h`) push scenario data into an inner `:Block` without a
compile-time dependency on its type, storing the name of each function
and retrieving it with `get_method()`.

## 15.6 Relation to the class-name factories {#sec-15-6}

The methods factory is one of two distinct factory mechanisms in
SMS++, and they should not be confused:

- the methods factory of this chapter maps a *method name* to a
  *function that changes an existing `Block`*;
- the class factories of [Chapter 18](18-factories-netcdf.md#ch-18) map a *class name* to a
  *constructor*, so that a `Block`, `Solver`, `Configuration`,
  `Solution` or `Change` of a type named in a file can be
  *created* without compile-time knowledge of the type
  (`Block::new_Block(name)`, `Solver::new_Solver(name)`, ...).

Both rest on the same idea (deferring a type decision to a runtime
string), but one *constructs objects* while the other *mutates an
existing one*. The `StochasticBlock` realiser mentioned in [§15.1](15-methods-factory.md#sec-15-1)
uses the methods factory to push scenario data into an inner
`Block` it was handed; it would use the class factory if it had
to *build* that inner `Block` from a serialised description.

## 15.7 The dark side: registration versus the linker {#sec-15-7}

Selecting classes and methods at runtime from a string has a drawback,
worth stating plainly since it bites newcomers.

The registration of a method (or of a class, in the factories of
[§15.6](15-methods-factory.md#sec-15-6) and [Chapter 18](18-factories-netcdf.md#ch-18)) is performed by code that runs at program
*initialization*, typically by a static initializer in the
translation unit of the `:Block`. That translation unit, however,
receives *no visible call* from the rest of the program: the
only thing the program does is later ask the factory for
`"BinaryKnapsackBlock::chg_weights"` by string. From the
compiler's and linker's point of view, nothing in the program
references the symbols of that translation unit, and therefore an
aggressive linker may conclude that the unit is *unused* and drop
it from the executable. When that happens, the static initializer
never runs, the registration never occurs, and the program fails
at runtime with "such-and-such not present in the factory", even
though the code is right there in the source tree.

This is a tension between "resolve everything at runtime by name" and
"let the linker remove what is statically unreachable", rather than a
defect of the code, and avoiding it requires some care at build time,
i.e., forcing the linker to retain the relevant translation units even
though it sees no reference to them. The
mechanics (the linker flags and the SMS++ registration macros that
make them work) are the subject of [Chapter 18](18-factories-netcdf.md#ch-18); here we only flag
the phenomenon, so that a reader who hits an inexplicable "not
found in factory" error knows that the cause is almost always a
translation unit the linker has optimised away, not a missing
registration in the source.

## 15.8 Idioms {#sec-15-8}

*Data-changing methods in the standard shapes.* A
`:Block` author who writes `chg_*()` methods in the
`MS[_dbl/int]_[rngd/sbst]` shapes gets factory registration almost
for free and makes the `:Block` usable by all the type-agnostic
machinery (scenario generation, meta-configuration, distributed
drivers). A method with an idiosyncratic signature can still be
registered, but only through a hand-written adapter.

*Registration in `static_initialization()`.* The conventional home
for `register_method` calls is the `:Block`'s
`static_initialization()`, run once when the class is loaded.
Registering elsewhere is allowed (the methods are public) but
should be the exception, for cases the author did not foresee.

*The `ClassName::method` naming.* Factory names
are arbitrary strings, but following the `ClassName::method_name`
convention avoids collisions and makes a configuration file
self-documenting.
