# SMS++ User Manual — Maintenance & Handoff Notes

*Internal document for whoever (human or AI assistant) maintains this
manual. It is deliberately excluded from the MkDocs site and from the
PDF build. Last updated 2026-05-20, for SMS++ **0.6.0**.*

Read this first. It captures what the manual is, the rules it was
written by, how it is built and deployed, and the traps already hit so
they are not re-hit.

---

## 1. What this is

A user manual for **SMS++** (open-source C++-20 framework for
structured optimization), version **0.6.0** (December 2025). Audience:
algorithm developers, `:Block`/`:Solver` authors, and sophisticated end
users who already write C++-20. It is a *companion* to the Doxygen API
reference and the project Wiki, not a replacement.

Architecture: concepts-first (Part II), with three running `Block`
examples threaded throughout, end-to-end recipes in Part III, and a
Developer's Guide in Part IV. The full plan is in `OUTLINE.md` (v0.2);
read it for the rationale and the detailed table of contents.

Single source = Markdown under `manual/chapters/`, built to **both** a
PDF (Pandoc + LaTeX) and a web site (MkDocs Material).

---

## 2. Authoring conventions — DO NOT drift from these

These were agreed with the author (Antonio Frangioni) and applied
uniformly. Preserve them on every update.

- **Language:** scientific English, *understatement*. No marketing
  language, no first-person "I", no colloquialisms.
- **No comparison with other modelling systems.** The manual speaks
  *only* of SMS++. (The intro paper's bibliography compares with
  AMPL/GAMS/JuMP/etc.; do **not** import that here.)
- **`:Block` / `:Solver` colon convention:** a leading colon denotes "a
  concrete class derived from the abstract base", following the slide
  decks. Use consistently.
- **Section numbering:** headings carry *manual* numbers (`## 1.1`,
  `## 4.2`, …). The web relies on them. For the PDF, LaTeX's automatic
  numbering is suppressed (`\setcounter{secnumdepth}{-1}` in
  `build/preamble.tex`) precisely so the manual numbers are not
  doubled. If you add a section, number it by hand and keep the
  sequence gap-free.
- **Status admonitions** (defined in `index.md` conventions): four
  forms — **Status — mature** (implicit), **Status — beta**,
  **Status — under development**, **Status — planned**. Use these exact
  spellings.
- **Doxygen cross-references** use the public Doxygen with
  `CREATE_SUBDIRS = YES`:
  `https://smspp.gitlab.io/smspp-project/<xx>/<yy>/class_s_m_spp__di__unipi__it_1_1_<name>.html`,
  where `<xx>/<yy>` is a hash. Group-documented classes (e.g.
  `MCFBlock`) have **no** class page — link the header page
  `.../<xx>/<yy>/_<name>_8h.html` instead. **Always verify each URL
  against the local Doxygen** (see §4) — the hashes are not guessable.
- **Citations / Sources:** when a claim comes from a file or tool, cite
  it. The Bibliography (`chapters/Z-bibliography.md`) is small and
  SMS++-centric; methodological refs were web-verified (keep them
  accurate if you add more).
- **Math delimiters — always `$…$` / `$$…$$`, never raw `\(…\)`.**
  Inline math is written `$…$`, display math `$$…$$`. The web build's
  `pymdownx.arithmatex` (generic mode) rewrites these into `\(…\)` /
  `\[…\]` *for you*; KaTeX auto-render (`build/katex-init.js`) then
  typesets them. Writing raw `\(…\)` in the source is therefore both
  unnecessary and discouraged — it bypasses the convention and is
  fragile across the PDF (Pandoc) path. As of 2026-05-21 **no chapter
  source contains `\(`** (verified by a tree-wide grep); keep it that
  way. NB: if you ever see *literal* `\(n\)` on the rendered site, that
  is **not** a source defect (the source already uses `$`) — see §10.
- **Cross-references are explicit-id Markdown links.** Numbered headings
  carry `{#id}` (`#ch-N`, `#sec-N-M`, `#app-X`, `#sec-X-M`, `#rec-RN`);
  in-text references are links to `FILE.md#id` (bare sibling filename
  from a chapter; `chapters/FILE.md#id` from `index.md`). The PDF build
  drops the file part via `build/crossref-link.lua`. Full rationale and
  the "when you add a section" rule are in §10.2. Do not hand-write a
  reference as plain text — link it.

---

## 3. Source of truth & verification discipline

- **Authoritative source code:** `/Users/frangio/Codes/SMS++` (umbrella
  + core + problem modules), read-only. **Never** run git operations or
  modify anything there.
- **Local Doxygen HTML:** under `SMS++-intro/html/` in the workspace —
  use it to confirm the exact two-level hash subdirectory of every
  Doxygen URL before writing it.
- **Every non-trivial claim and every code snippet must be checked
  against the real headers/sources.** Snippets are meant to compile
  against the real APIs. Concrete facts established this way that are
  easy to get wrong:
  - Linear `Function`s are built with **`LinearFunction`** (real, used
    throughout 0.6.0, e.g. `BinaryKnapsackBlock.cpp`): pattern
    `LinearFunction::v_coeff_pair` + `new LinearFunction(std::move(cp), const)`
    passed to `set_function(...)`. There is **no** successor class.
  - `ColVariable` type enum is `kContinuous` (=0), `kBinary`,
    `kInteger`, … — **not** `kReal`.
  - `BinaryKnapsackBlock` **does** register methods-factory methods
    (`chg_weights`/`chg_profits`/`chg_capacity`); the manual's
    methods-factory example (§15, R2) relies on this.
  - `Modification` mirroring uses `modify_coefficient(...)` /
    `modify_coefficients(...)` on `LinearFunction`; abstract changes
    arrive as `C05FunctionModLinRngd` / `C05FunctionModLinSbst`.
  - Destructors of `:Block`s should `Constraint::clear(...)` their
    constraint groups and `clear()` the objective's function *before*
    the `ColVariable` groups die (ThinVarDepInterface double-link
    invariant). See Appendix A.
- For high-stakes consistency passes, use a verification subagent that
  reads **all** chapters end-to-end (cross-refs, numbering, tone,
  Doxygen URL shape, status admonitions). One such pass was run; it is
  cheap insurance after any sizeable edit.

---

## 4. Layout

```
manual/
  OUTLINE.md            # the plan (v0.2) — rationale + detailed ToC
  MAINTENANCE.md        # this file
  index.md              # front matter (title, abstract, conventions, ToC)
  model.html            # local-only live preview (marked.js); not deployed
  SMS++-manual.pdf      # built PDF deliverable (commit it; see §6)
  chapters/             # 29 files, single source of all prose
    01-introduction.md … 18-factories-netcdf.md
    R1-mcf.md … R5-cfl-benders.md
    A-writing-block.md  B-writing-solver.md  C-writing-modification.md
    D-concept-block-map.md            # Appendix D (concept→Block table)
    Y-glossary.md  Z-bibliography.md   # back matter
  figures/              # .svg (web) + .pdf (LaTeX) for each figure
    SMSglobal0a, SMSglobal1a, SMSglobal4a, Function-doxy   # from slides
    block-lifecycle.{tex,pdf,svg}      # authored TikZ figure
  build/
    Makefile  pandoc.yaml  preamble.tex  mkdocs.yml
    figure-ext.lua  break-code.lua  katex-init.js
```

The three running Blocks: `MCFBlock` (leaf, wrapper solver),
`BinaryKnapsackBlock` (leaf, native DP solver),
`CapacitatedFacilityLocationBlock` (non-leaf, 4 formulations, R3Block →
MCF relaxation, hidden `BendersBFunction`). Appendix D maps every
concept to the Block/section that exercises it — keep it in sync if you
add or move material.

---

## 5. Building locally

From `manual/build/`:

```sh
make pdf      # -> ../SMS++-manual.pdf   (Pandoc + pdflatex)
make html     # -> ../../manual-site/    (MkDocs Material; sibling of manual/)
make serve    # live preview at 127.0.0.1:8000
make figures  # regenerate any missing .svg from .pdf in ../figures
```

PDF acceptance check: rebuild and confirm **zero** `Overfull \hbox`
warnings (compile the standalone `.tex` and grep the log). The current
build is ~112 pp, A4.

Pieces of the pipeline and *why they exist* (don't remove without
cause):

- `pandoc.yaml` — lists the 30 input files **in reading order**
  (front matter → chapters → recipes → appendices → back matter). Keep
  this list, the MkDocs `nav`, and `model.html` in sync when adding a
  chapter. `number-sections: false` (manual numbering is in the
  headings).
- `preamble.tex` — geometry (margins; author set them to 2cm),
  `\setcounter{secnumdepth}{-1}` (suppress LaTeX numbering),
  `\counterwithout{figure}{chapter}` (continuous figure numbers),
  `fvextra` + redefined `Shaded` env (framed, light-grey background,
  `breaklines` so code wraps), and a block of
  `\DeclareUnicodeCharacter` mappings (≤ ≥ ∞ ∈ → − … ●) so **pdflatex**
  can typeset those glyphs that appear in inline code / ASCII art.
- `crossref-link.lua` — for the PDF build only, rewrites inter-file
  cross-reference links `…<file>.md#anchor` to intra-document `#anchor`
  so Pandoc resolves them as internal hyperlinks (see §2 and §10.2).
  Leaves Doxygen `https://…` and figure links untouched.
- `break-code.lua` — makes inline `\texttt` breakable (zero-penalty
  `\allowbreak` after each char) so long paths/URLs/signatures don't
  overflow the right margin.
- `figure-ext.lua` — rewrites figure `.svg` → `.pdf` for the LaTeX
  build only. **Therefore every figure must exist in both `.svg` and
  `.pdf`.**

---

## 6. Adding or updating a figure

1. Source the diagram (reuse a slide PDF, or author one — see
   `figures/block-lifecycle.tex`, a `standalone` TikZ file).
2. Produce **both** forms in `figures/`:
   `pdflatex foo.tex` → `foo.pdf`, then
   `pdftocairo -svg foo.pdf foo.svg` (no `pdf2svg`/`inkscape`/
   `rsvg-convert` in the sandbox; `pdftocairo` and `pdflatex` are there).
3. Reference it from a chapter as `![caption](../figures/foo.svg)` (the
   `.svg`; the Lua filter swaps to `.pdf` for the PDF). Path is relative
   to the chapter file.
4. Commit **both** `.svg` and `.pdf` (and the `.tex` source if authored).
   Do **not** commit `*.aux` / `*.log`.

---

## 7. GitLab Pages deployment

Pages is **not** automatic; it is published by the umbrella's
`.gitlab-ci.yml` `pages` job (`when: manual` — trigger it by hand). The
manual was wired into the existing Doxygen `pages` job by adding, right
after `mv doc/html/ public/`:

```yaml
    - apk add --no-cache python3 py3-pip
    - pip install --no-cache-dir --break-system-packages mkdocs-material
    - mkdocs build -f manual/build/mkdocs.yml -d "$CI_PROJECT_DIR/public/manual"
```

Result: Doxygen at `…/smspp-project/`, manual at `…/smspp-project/manual/`.

**Gotchas already hit (check these before blaming the content):**

- The umbrella `.gitignore` had a `build/` rule that swallowed
  `manual/build/` (CI couldn't find `mkdocs.yml`). Fix: track it
  (`git add -f manual/build/` + a `!manual/build/` negation), or rename
  the folder. **`build/katex-init.js` must ship** (it renders the math)
  — `mkdocs.yml`'s `exclude_docs` excludes `build/` *selectively*
  (configs/sources) but keeps `katex-init.js`.
- `figures/*.svg` (and `.pdf`) must be committed — the same `.gitignore`
  family can drop them; without them the site has broken images.
- `site_dir` must be **outside** `docs_dir` (which is `manual/`); the CI
  uses `-d "$CI_PROJECT_DIR/public/manual"`. `mkdocs.yml`'s
  `use_directory_urls: false` is set; MkDocs rewrites relative image
  paths correctly for either URL mode **as long as the figures are
  present at build time**.
- Trigger the `pages` job on the branch that actually contains
  `manual/` (the job checks out master/develop submodules).
- README "web" link should point to `…/smspp-project/manual/`, not to
  `index.md` (a lone `.md` on GitLab has no sidebar/nav).

---

## 8. Updating for a new SMS++ version — checklist

1. **Bump version strings:** `index.md` (title/abstract), the closing
   line of `chapters/C-writing-modification.md`,
   `chapters/Z-bibliography.md`, and `OUTLINE.md`.
2. **Diff the source tree** (`/Users/frangio/Codes/SMS++`) against what
   the manual states; re-verify changed APIs and every code snippet.
   Re-verify any Doxygen URL whose class was added/renamed (hashes
   change).
3. **Beta / under-development / planned items** — revisit each, since
   these are the most likely to have moved:
   - `Change` mechanism (Ch16, **beta**) — has the interface settled?
   - "half-baked Solution" / always-materialised abstract `Variable`s
     (Ch7 §7.6, Ch9 §9.7, **under development**) — is the `Solver`-
     returns-`Solution` adoption path in?
   - parallelism write-starvation (Ch17, **under development**).
   - `BendersDecompositionSolver` (R5, **planned**) — if it lands, R5
     shrinks to a config change.
4. **Run a consistency subagent** over all chapters (see §3).
5. **Rebuild**: `make pdf` (confirm 0 overfull), `make html`; commit the
   refreshed `SMS++-manual.pdf`; re-run the `pages` job.

---

## 9. Where to find more

- `OUTLINE.md` — the design rationale and the original detailed ToC.
- The earlier drafting history is in the prior session transcripts; the
  TODO/■ list there records every decision and user correction made
  while writing v0.1.
- Authoritative when in doubt: SMS++ source > public Doxygen > Wiki.

---

## 10. Known issues & open requests (logged 2026-05-21)

Raised by the author after reviewing the rendered web build.

### 10.1 Literal `\(…\)` showing on the site (math not rendering)

**Symptom.** On a rendered HTML page (reported on a Part IV page — the
Bin Packing example in `chapters/A-writing-block.md`) the math appears
as raw text, e.g. `The Bin Packing problem: given \(n\) items of sizes
\(s_0, \dots, s_{n-1}\) and bins of identical capacity \(C\)`.

**This is *not* a source defect.** The Markdown already uses `$…$`
(`$n$`, `$s_0, \dots, s_{n-1}$`, `$C$`); no chapter source contains
`\(` (tree-wide grep). A clean local rebuild
(`mkdocs build -f build/mkdocs.yml`) produces the *correct* markup —
`given <span class="arithmatex">\(n\)</span> items …` — and ships
`build/katex-init.js` plus the KaTeX CDN `<script>`/`<link>` tags. So
`pymdownx.arithmatex` (the server-side `$→\(` rewrite) is working; the
literal `\(n\)` means **KaTeX auto-render did not execute client-side**
on the page being viewed.

**Real causes, in order of likelihood, and the fix:**

1. **Stale GitLab Pages deployment** — the live site predates the KaTeX
   wiring (or `katex-init.js` was not shipped in that build). Fix:
   rebuild and **re-trigger the `pages` job** (it is `when: manual`, see
   §7); confirm `…/manual/build/katex-init.js` is reachable on the live
   site (200, not 404).
2. **KaTeX CDN blocked/unreachable** in the viewer's network — the
   `cdn.jsdelivr.net` scripts fail to load, so `renderMathInElement` is
   undefined and nothing is typeset. Fix: confirm the CDN is reachable;
   for robustness consider **self-hosting** the KaTeX assets under the
   site (drop the three jsdelivr URLs into a local `katex/` dir and
   point `extra_javascript`/`extra_css` there), removing the runtime
   network dependency.
3. **Viewing raw HTML without JS** (e.g. a `file://` open, a fetch, or a
   `.md`-only view) — math is rendered client-side, so any JS-less view
   shows the `\(…\)` source. Not a bug; view the built site served by
   MkDocs / Pages.

**Optional hardening of `build/katex-init.js`:** it currently relies on
Material's `document$` observable. A `DOMContentLoaded` fallback (guarded
so math is not rendered twice, and a guard for `renderMathInElement`
being undefined) makes it fail safe if `document$` is ever unavailable.
Low-risk; do it if the CDN/self-host route does not fully settle it.

### 10.2 Inter-section cross-references are now clickable — DONE 2026-05-21

**What was the request.** Make the in-text references between
sections/chapters clickable. They used to be **plain text** (~244
`Chapter N`, ~178 `§N.N`, ~51 `Recipe R…`, ~24 `Appendix X`, plus the
`index.md` ToC), none of them a link.

**Why a naive `.md`-link does not work for both builds.** MkDocs and
Pandoc slugify heading text *differently* (MkDocs keeps the manual
number: `## 4.2 The identity rule` → `#42-the-identity-rule`; Pandoc
strips leading digits → `#the-identity-rule`). A link to an
auto-generated slug would resolve in only one of the two outputs. Worse,
Pandoc does **not** rewrite an inter-file `foo.md#id` link when it
concatenates the sources into the single PDF — it would emit a broken
`\href{foo.md#id}`.

**The scheme actually in use (keep to it).**

1. **Explicit heading ids.** Every *numbered* heading carries an
   explicit `{#id}` attribute (honoured by both `attr_list` in MkDocs
   and Pandoc's `header_attributes`). The id convention is:
   - chapter title `# N. …` → `{#ch-N}`
   - subsection `## N.M …` → `{#sec-N-M}`
   - appendix title `# Appendix X …` → `{#app-X}`
   - appendix subsection `## X.M …` → `{#sec-X-M}`
   - recipe title `# Recipe RN …` → `{#rec-RN}`
   (Un-numbered `##`/`###` headings — e.g. recipe `## Goal` — get no id;
   nothing links to them.)
2. **References are Markdown links to those ids.** From a chapter the
   target is the bare sibling filename (`[Chapter 7](07-physical-
   abstract.md#ch-7)`, `[§7.6](07-physical-abstract.md#sec-7-6)`); from
   `index.md` (manual root) it is prefixed with `chapters/`
   (`[Chapter 7](chapters/07-physical-abstract.md#ch-7)`). `§N` with no
   dot, and bare `Chapter N`, point at `#ch-N`.
3. **PDF path fix-up:** `build/crossref-link.lua` (wired into
   `pandoc.yaml` between `figure-ext.lua` and `break-code.lua`) turns any
   `…<file>.md#anchor` link, for the LaTeX build only, into
   `\hyperref[anchor]{text}`. Doxygen `https://…` links and figure links
   are left untouched. **Do not** simplify this to a plain `#anchor`
   link (which Pandoc renders as `\hyperlink{anchor}`): Pandoc wraps each
   heading as `\hypertarget{id}{\chapter{…}\label{id}}`, and the
   `\hypertarget` destination sits *before* the `\clearpage` that
   `\chapter` triggers — so a `\hyperlink` to a chapter lands at the
   bottom of the *previous* (often half-empty) page. `\hyperref[id]`
   instead targets the `\label{id}` placed *after* the sectioning command
   (the same anchor the auto table of contents uses), landing exactly on
   the chapter's page. Verified with pypdf: every in-text chapter link
   resolves to the chapter's start page, none to the stale hypertarget.

**References left as plain text on purpose:** those inside fenced code
blocks / inline code (e.g. `// … Chapter 8` in a C++ comment) — a link
there would render literally in the listing. The `Part I…IV` mentions
are also unlinked: Parts are nav groupings only (no heading, no anchor).

**Verification done (both builds).** MkDocs: headings emit
`id="ch-4"`/`id="sec-4-2"`, links rewritten to `…html#anchor`. PDF:
`pandoc -t latex` shows 481 `\hyperlink`, **0** `\href{…md…}`, every
`\hyperlink` has a `\hypertarget`; full `pdflatex` run is **0
`Overfull \hbox`**, 0 undefined references, 118 pp. The bulk transformer
used is kept at `build/crossref-bulk.py` for reference; it is **one-shot
and NOT idempotent** (it would re-linkify already-linked text), so never
re-run it on the current sources — add new ids/links by hand.

**When you add a section or a reference later:** give the new numbered
heading its `{#id}` per the convention above, and write the reference as
a Markdown link to `FILE.md#id` (add the `chapters/` prefix only when
linking from `index.md`). Re-run a build of each output to confirm.

*Status — done (2026-05-21).*

*End of maintenance notes.*
