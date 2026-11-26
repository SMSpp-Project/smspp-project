# SMS++<br>a Structured Modeling System for mathematical models

A User Manual

**Antonio Frangioni**  
*Dipartimento di Informatica, Università di Pisa*  
`frangio@di.unipi.it`

**Donato Meoli**  
*Dipartimento di Informatica, Università di Pisa*  
`donato.meoli@outlook.com`

*Version 0.8.0 (November 2026)*

---

## Abstract

SMS++ is an open-source C++20 software framework for structured
mathematical optimization. Its central abstraction is the `Block`, a
nested unit that carries both the *semantics* of a mathematical
sub-problem (its problem data, in natural form) and, on demand, an
*abstract representation* of that sub-problem (its `Variable`s,
`Constraint`s, `Objective`) suitable for general-purpose solvers. A
`Block` can have any number of sub-`Block`s, recursively, and any
number of attached `Solver`s. Specialized `Solver`s exploit the
structure of a specific `:Block`, while general-purpose ones operate on
the abstract representation. The framework handles the lazy
propagation of changes through the `Modification` mechanism (and the
newer `Change` one, in beta), the configuration of `Block` trees and
`Solver` chains, the reformulation, relaxation and restriction of
`Block`s through the `R3Block` mechanism, and coarse-grained parallel
computation.

This manual is a self-contained companion to the
[SMS++ Doxygen reference](https://smspp.gitlab.io/smspp-project/) and to the
[SMS++ Wiki](https://gitlab.com/smspp/smspp-project/-/wikis/home). It
introduces the concepts and idioms of SMS++ progressively, threads
three running `Block` examples (`MCFBlock`, `BinaryKnapsackBlock`,
`CapacitatedFacilityLocationBlock`) through the exposition, and
collects a handful of end-to-end recipes that recombine the concepts
into runnable programs.

## Acknowledgements

The development of SMS++ would not have been possible without the
contributions of many co-authors over the years, prominent among them
Rafael Durbano Lobato, Donato Meoli, Enrico Gorgone, Federica Di
Pasquale, Francesca Demelas, Kostas Tavlaridis-Gyparakis, Niccolò
Iardella, and Benoit Tran. Specific contributions are credited in the
source files and in the project's [`CHANGELOG.md`](https://gitlab.com/smspp/smspp-project/-/blob/develop/CHANGELOG.md).

Recent work on SMS++, including much of what is documented here, has
been carried out in the framework of the
[**RESILIENT project**](https://resilient-project.github.io); the
project is acknowledged in the form indicated in the umbrella
project's [`README.md`](https://gitlab.com/smspp/smspp-project/-/blob/develop/README.md).

## License

SMS++ is distributed under the [GNU Lesser General Public License
v3.0](https://opensource.org/licenses/lgpl-3.0.html) (LGPL-3); this
manual is distributed under the same terms.

---

## Conventions

* **Class and method names** are typeset in `monospace`, e.g. `Block`,
  `add_Modification( ... )`. Concrete classes derived from an abstract base
  are sometimes written `:Solver`, `:Block` to emphasise their derived nature,
  following the convention adopted in the slide decks that accompany the
  project.
* **File-path and source-tree references** are typeset in `monospace`
  too, e.g. [`SMS++/include/Block.h`](https://gitlab.com/smspp/smspp/-/blob/develop/include/Block.h). Whenever a precise pointer into
  the source is given, the form is `file:line`.
* **Doxygen cross-references** are URLs to the public Doxygen at
  `https://smspp.gitlab.io/smspp-project/`. The Doxygen is generated
  with `CREATE_SUBDIRS = YES`, so each class page lives in a two-level
  hashed subdirectory; a typical URL has the form
  `https://smspp.gitlab.io/smspp-project/<xx>/<yy>/class_s_m_spp__di__unipi__it_1_1_<name>.html`,
  where `<xx>/<yy>` is a hash computed by Doxygen from the class name.
  If a cross-reference link should ever break (for instance because
  the Doxygen has been regenerated with different settings), the
  reader can navigate from
  <https://smspp.gitlab.io/smspp-project/annotated.html>, the
  alphabetical class index, to reach the page from a stable entry
  point. A handful of classes (notably `MCFBlock`) are documented as
  part of a Doxygen *group* rather than as a stand-alone class page;
  for these the link points to the header-file page
  (`_<header>_8h.html`), which is the canonical entry.
* **Wiki cross-references** are to
  `https://gitlab.com/smspp/smspp-project/-/wikis/home` and are
  written `[Wiki: page-name]`.
* **Mathematical notation** follows the conventions adopted in the
  slide decks. Decision variables and parameters are usually
  italicized; sets are uppercase calligraphic where needed for
  clarity.
* **Lemmas, propositions and theorems** are numbered by chapter (Lemma
  8.2 is the second numbered statement of [Chapter
  8](chapters/08-modification-janus.md#ch-8)). Each states its
  hypotheses on the code of the version covered, and its proof points
  to the methods whose behavior it relies on. A statement about what a
  `:Block` or a `:Solver` *must* do (a contract) is given as a
  definition, and the results that rely on it list it among their
  hypotheses.
* **Status admonitions** flag the maturity of a feature explicitly.
  Four forms appear in the text:
    * **Status — mature.** The feature is stable, tested, in
      `develop` and in tagged releases. Implicit unless stated.
    * **Status — beta.** The feature is in `develop` but its
      interface or semantics may change in incompatible ways.
    * **Status — under development.** The feature is partly present
      but is actively being reworked; the manual describes both what
      exists today and the direction of the intended revision.
    * **Status — planned.** The feature is documented as intended
      but is not yet released. The manual indicates explicitly that
      it cannot be used today.

When in doubt about a claim made in this manual, the authoritative
sources are, in order of decreasing specificity: the source code in
the public repository <https://gitlab.com/smspp/smspp-project>, the
public Doxygen reference, and the SMS++ Wiki.

---

## Table of contents (high-level)

* **Part I: Foundations.**
    * [Chapter 1](chapters/01-introduction.md#ch-1). Introduction.
    * [Chapter 2](chapters/02-installation.md#ch-2). Installation and
      first build.
    * [Chapter 3](chapters/03-mental-model.md#ch-3). The mental model.
* **Part II: Concepts and Idioms.**
    * [Chapter 4](chapters/04-block.md#ch-4). Block.
    * [Chapter 5](chapters/05-variable-constraint-objective.md#ch-5).
      Variable, Constraint, Objective.
    * [Chapter 6](chapters/06-solver.md#ch-6). Solver.
    * [Chapter 7](chapters/07-physical-abstract.md#ch-7). Physical vs
      Abstract representation.
    * [Chapter 8](chapters/08-modification-janus.md#ch-8).
      Modification and the Janus discipline.
    * [Chapter 9](chapters/09-solution.md#ch-9). Solution.
    * [Chapter 10](chapters/10-r3block.md#ch-10). R3Block:
      Reformulation, Relaxation, Restriction.
    * [Chapter 11](chapters/11-configuration.md#ch-11). Configuration.
    * [Chapter 12](chapters/12-sub-block.md#ch-12). Sub-Block and the
      recursive flow of Modifications.
    * [Chapter 13](chapters/13-function-family.md#ch-13). The Function family.
    * [Chapter 14](chapters/14-lag-benders-bfunction.md#ch-14).
      LagBFunction and BendersBFunction.
    * [Chapter 15](chapters/15-methods-factory.md#ch-15). The methods factory.
    * [Chapter 16](chapters/16-change.md#ch-16). Change.
    * [Chapter 17](chapters/17-parallel.md#ch-17). Parallel and
      asynchronous computation.
    * [Chapter 18](chapters/18-factories-netcdf.md#ch-18). Factories
      and netCDF serialisation.
* **Part III: End-to-end Recipes.**
    * [Recipe R1](chapters/R1-mcf.md#rec-R1). Solving a Min-Cost Flow instance.
    * [Recipe R2](chapters/R2-knapsack-reopt.md#rec-R2). Reoptimizing
      a Binary Knapsack.
    * [Recipe R3](chapters/R3-cfl-three-ways.md#rec-R3). CFL three
      ways: cuts / MCF relaxation / Lagrangian.
    * [Recipe R4](chapters/R4-cfl-lagrangian.md#rec-R4). CFL via
      Lagrangian decomposition with `PrimalProximalHeur`.
    * [Recipe R5](chapters/R5-cfl-benders.md#rec-R5). CFL via Benders
      cuts with a user-cut callback.
* **Part IV: Developer's Guide (Appendix).**
    * [Appendix A](chapters/A-writing-block.md#app-A). Writing a new `:Block`.
    * [Appendix B](chapters/B-writing-solver.md#app-B). Writing a new
      `:Solver`.
    * [Appendix C](chapters/C-writing-modification.md#app-C). Writing
      a new `:Modification` / `:Change`.
    * [Appendix D](chapters/D-concept-block-map.md#app-D).
      Concept-to-Block map.

Bibliography, glossary, and index follow.
