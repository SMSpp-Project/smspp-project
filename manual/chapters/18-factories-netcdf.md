# 18. Factories and netCDF serialisation

[Source: `SMS++/include/SMSTypedefs.h` (factory macros),
`Block.h`, `Configuration.h`, `Solution.h`, `Change.h`]

Two cross-cutting mechanisms have been referred to throughout the
manual without being treated head-on: the *class-name factories*
that let an object be created from a string, and the *netCDF
serialisation* that lets a whole `Block` tree be written to and
read from disk. This chapter covers both, and discharges the
promise of §15.7 about the linker.

## 18.1 The class-name factories

Five of the framework's base classes carry a *factory*: a static
map from a class name (a `std::string`) to a constructor for that
class. The entry points are:

- `Block::new_Block(name, father)`
- `Solver::new_Solver(name)`
- `Configuration::new_Configuration(...)`
- `Solution::new_Solution(name)`
- `Change::new_Change(name)`

Each returns a freshly constructed object of the concrete type
named by the string, as a pointer to the base class. This is what
makes it possible to read "the inner `Block` is a `UCBlock`", or
"attach a `CPXMILPSolver`", or "this serialised object is a
`BinaryKnapsackBlockRngdChange`" from a configuration file or a
netCDF group and *build the right object* without the reading code
having any compile-time dependency on the concrete type. It is the
companion of the *methods* factory of Chapter 15: that one
*mutates* an existing object by method name, this one *creates*
objects by class name.

### Registering a class

A concrete class joins its factory through two macros
(`SMSTypedefs.h:299-377`):

- in the class declaration (private part of the `.h`):
  `SMSpp_insert_in_factory_h;`
- in *exactly one* `.cpp` (usually the class's own):
  `SMSpp_insert_in_factory_cpp_0( ClassName );` if the base-class
  constructor takes no argument, or
  `SMSpp_insert_in_factory_cpp_1( ClassName );` if it takes one
  (e.g. a `:Block`, whose constructor takes the father `Block*` —
  this is why `MCFBlock.cpp` uses the `_1` form, §4.5). *Template*
  classes use the `_t` variants
  (`SMSpp_insert_in_factory_cpp_0_t` / `_1_t`), which add the
  `template<>` specialisation the compiler requires; this is
  exactly the case of the `SimpleConfiguration<T>` family of
  Chapter 11, whose instantiations (e.g. `SimpleConfiguration<
  int >`) are registered with a `_t` macro. If the class name
  contains commas (a template instantiation with more than one
  parameter), it is wrapped in extra parentheses so the
  preprocessor does not read them as macro-argument separators.

The `_h` macro defines a tiny private `_init` class with a single
static member `_initializer`; the `_cpp` macro defines that
member. When the program starts, the `_initializer`'s constructor
runs and does two things: it registers the class in the factory of
its base, **and** it calls the class's `static_initialization()`
method — which is exactly where the methods-factory registrations
of Chapter 15 belong. So the two factory systems are wired up by
the same one-time static initialisation.

## 18.2 The dark side, discharged: factories versus the linker

This is the mechanism §15.7 warned about, now in full.

The registration code lives inside the static `_initializer` of a
translation unit, and *nothing in the rest of the program
references that translation unit by symbol* — the program only
ever asks the factory for the class by string. An aggressive
linker, seeing no references into the unit (especially when it
sits in a dynamic library), may drop it from the executable. The
`_initializer` then never runs, and the program fails at runtime
with

```
terminate called after throwing an instance of 'std::invalid_argument'
  what():  XXXX not present in YYYY factory
```

even though the class is present in the source
(`SMSTypedefs.h:324-358`). The cause is *never* a missing
registration in the source; it is a translation unit the linker
has optimised away.

There are three remedies, in increasing order of preference:

1. **Force an object into the executable.** Instantiate an object
   of the missing type somewhere reachable from `main()`. This
   requires `#include`-ing the type's header and is therefore the
   least clean option.
2. **Disable the linker optimisation.** Pass the linker flag that
   keeps unreferenced units: `--no-as-needed` for `g++` (as
   `-Wl,--no-as-needed` when linking through the compiler driver),
   `-all_load` for `clang++`. Note that the default behaviour
   varies across toolchains and even across OS distributions of
   the *same* toolchain, and that these flags can have unwanted
   side effects (they retain *everything*).
3. **Use `SMSpp_ensure_load( ClassName )`.** Placed at the top of
   a `.cpp` (with the class's header `#include`-d), this macro does
   the "dirty but cheap" work of guaranteeing that `ClassName` is
   registered, targeting just that one class rather than retaining
   every unit. It carries a small, non-zero runtime cost and is
   meant as the targeted last resort.

The phenomenon is not a defect of SMS++ but the price of resolving
types by name at runtime; the manual flags it because the symptom
("not present in factory") is mystifying until one knows the
cause.

## 18.3 Why netCDF

SMS++ serialises `Block`s, `Configuration`s, `Solution`s,
`State`s and `Change`s to
[netCDF](https://www.unidata.ucar.edu/software/netcdf/), a binary,
self-describing, hierarchical file format. The choice fits the
framework's structure for two reasons:

- netCDF files are organised into nested **groups**, which map
  directly onto the nested structure of a `Block` tree: a `Block`
  serialises into a group, its sub-`Block`s into nested
  sub-groups, recursively. The on-disk shape mirrors the in-memory
  shape.
- netCDF stores **native binary arrays** efficiently, which suits
  the large numeric data (cost vectors, constraint matrices,
  scenario trees) of the problems SMS++ targets, far better than a
  text format such as JSON would.

A practical bonus is tooling: because netCDF is a widely adopted
standard rather than a SMS++ invention, a SMS++ serialised file
can be inspected with general-purpose, third-party tools, no
SMS++-specific viewer required. The standard `ncdump` utility
turns any such file into readable text; `ncview` visualises array
data; and
[Panoply](https://www.giss.nasa.gov/tools/panoply/) — an
open-source graphical viewer available on all platforms, developed
by NASA's
[Goddard Institute for Space Studies](https://www.giss.nasa.gov/)
— offers a richer interactive exploration of the file's groups and
arrays.

## 18.4 The dark side of netCDF: groups are not free

netCDF buys the structural fit of §18.3 at a price that users must
know about: **creating a file with a very large number of groups
is inefficient — in fact, very inefficient.** netCDF groups are
designed to be a moderate-cardinality organising device, not a
container one instantiates by the thousand; the cost of writing a
file grows badly with the number of groups in it.

The naïve, "canonical" serialisation of a deeply or widely nested
`Block` tree can fall into exactly this trap. The standard example
is the `Solution` of a `UCBlock`: it contains, among other things,
the `Solution` of a `NetworkBlock` *for each time instant*, and
the canonical encoding would put each `NetworkBlockSolution` in
its own group. But the number of time instants can be large — 8760
for the hours of a year — and writing 8760 (or many more) groups
per solution makes solution files unbearably slow to produce.

To cope with this, the `Solution` classes involved (specifically,
the `NetworkBlockSolution` used as components of a
`UCBlockSolution`) implement a *bespoke* encoding that
**accumulates the information of all those sub-`Solution`s into a
single `netCDF::NcGroup`** rather than giving each its own,
trading the clean one-object-per-group correspondence for a
tolerable write cost.

It is important to be clear about the scope of this remedy:

> there is **no general, framework-wide mechanism** for collapsing
> many sub-objects into one group. The accumulation just described
> is a hand-written solution local to those specific `:Solution`
> classes, not a facility that any `:Block` or `:Solution` can
> simply switch on.

Whether a general mechanism could be provided — and what shape it
would take — is an open question. So the point for a user (and
especially for a `:Block`/`:Solution` author) is twofold: first,
that the inefficiency is real and can surface as a surprisingly
slow serialisation when a high-cardinality `:Block` or
`:Solution` is written in the naïve one-group-per-object way; and
second, that there is at present no built-in cure to reach for —
a high-cardinality `:Solution` that needs to avoid the cost must,
like `NetworkBlockSolution`, implement its own accumulating
encoding by hand. This is an inherent limitation of the chosen
file format rather than of SMS++'s design, but it currently has to
be addressed case by case.

## 18.5 The serialisation contract

Serialisation is symmetric and recursive. On the writing side,
`serialize(netCDF::NcGroup&)` writes the object into the given
group; a `Block`'s `serialize` writes its own data and then calls
`serialize` on each sub-`Block` into a nested group, so the whole
tree is written by one top-level call. There are convenience
overloads that take a filename or an `NcFile` and create the group
structure for you.

On the reading side, the contract has two levels. The *static*
`deserialize(netCDF::NcGroup&)` reads the mandatory string
attribute `"type"` from the group, passes it to the factory
(`new_Block`, `new_Configuration`, ...) to construct an empty
object of the right concrete type, and then calls that object's
*virtual* `deserialize` to fill it in. This is why the factory and
the serialisation are inseparable: deserialising an object of a
type not known at compile time *requires* the class-name factory
of §18.1 to turn the stored `"type"` string into the right object.

The same `"type"`-tag-plus-factory pattern is what lets a
`Configuration` file name an `RBlockSolverConfig` (Chapter 11), a
serialised solution name a `CapacitatedFacilityLocationSolution`
(Chapter 9), and a shipped `Change` name a
`BinaryKnapsackBlockRngdChange` (Chapter 16): in every case the
reader constructs the right concrete object from the stored class
name, with no compile-time knowledge of the type.

## 18.6 Idioms

**Register every concrete class in its factory.** A `:Block`,
`:Solver`, `:Configuration`, `:Solution` or `:Change` that is
meant to be created from a string (and almost all are) must carry
the `SMSpp_insert_in_factory_h` / `_cpp_k` macro pair. Forgetting
it makes the class invisible to `new_X(name)`.

**When a class is "not present in factory", suspect the linker
first.** The error almost never means a missing registration in
the source; it means the registering translation unit was
optimised away. Reach for `SMSpp_ensure_load(ClassName)` (with the
header included) as the targeted fix, or the linker flags of
§18.2 as the blunt one.

**Serialise the tree, not the pieces.** Because `serialize` /
`deserialize` recurse, a whole `Block` tree (with its
`Configuration`s and `Solution`s) round-trips through a single
top-level call. Build on that rather than serialising each
sub-`Block` separately.

**Use `ncdump` to debug serialised files.** When a serialised
`Block` does not deserialise as expected, `ncdump` on the file is
the fastest way to see what was actually written, including the
all-important `"type"` attributes.
