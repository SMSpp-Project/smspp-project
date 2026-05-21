#!/usr/bin/env python3
# ONE-SHOT bulk transformer used on 2026-05-21 to (a) add explicit {#id}
# anchors to every numbered heading and (b) convert plain-text inter-section
# references (Chapter N, §N.M, Recipe RN, Appendix X, Section N.M) into
# Markdown links. Kept for reference / re-derivation only.
#
# WARNING: NOT idempotent. It linkifies any "Chapter N"/"§N.M"/… it finds in
# prose, including text that is ALREADY inside a link, so running it a second
# time on already-linked sources produces broken nested links. Run it only
# against a pristine, un-linked snapshot. For day-to-day maintenance, add the
# {#id} and the Markdown link by hand per the convention in MAINTENANCE §2/§10.2.
#
# Usage: python3 crossref-bulk.py <manual-root-src> <out-dir>
#   (reads <src>/index.md and <src>/chapters/*.md; writes transformed copies)
# ---------------------------------------------------------------------------
"""Add explicit {#id} anchors to numbered headings and turn plain-text
inter-section references (Chapter N, §N.M, Recipe RN, Appendix X, Section
N.M) into Markdown links. Single source: links work in MkDocs natively and
in the Pandoc PDF via crossref-link.lua."""
import re, sys, os

CH = {1:"01-introduction.md",2:"02-installation.md",3:"03-mental-model.md",
4:"04-block.md",5:"05-variable-constraint-objective.md",6:"06-solver.md",
7:"07-physical-abstract.md",8:"08-modification-janus.md",9:"09-solution.md",
10:"10-r3block.md",11:"11-configuration.md",12:"12-sub-block.md",
13:"13-function-family.md",14:"14-lag-benders-bfunction.md",
15:"15-methods-factory.md",16:"16-change.md",17:"17-parallel.md",
18:"18-factories-netcdf.md"}
APP = {"A":"A-writing-block.md","B":"B-writing-solver.md",
"C":"C-writing-modification.md","D":"D-concept-block-map.md"}
REC = {1:"R1-mcf.md",2:"R2-knapsack-reopt.md",3:"R3-cfl-three-ways.md",
4:"R4-cfl-lagrangian.md",5:"R5-cfl-benders.md"}

STATS = {}
def bump(k,n=1): STATS[k]=STATS.get(k,0)+n

# ---------- pass 1: add {#id} to headings (line-based) ----------
def add_ids(text):
    out=[]
    for line in text.split("\n"):
        if line.startswith("#") and "{#" not in line:
            m=re.match(r'^# (\d+)\. ', line)
            if m: line=line.rstrip()+f" {{#ch-{int(m.group(1))}}}"; bump("id-ch")
            else:
                m=re.match(r'^## (\d+)\.(\d+) ', line)
                if m: line=line.rstrip()+f" {{#sec-{int(m.group(1))}-{int(m.group(2))}}}"; bump("id-sec")
                else:
                    m=re.match(r'^# Appendix ([A-D]) ', line)
                    if m: line=line.rstrip()+f" {{#app-{m.group(1)}}}"; bump("id-app")
                    else:
                        m=re.match(r'^## ([A-D])\.(\d+) ', line)
                        if m: line=line.rstrip()+f" {{#sec-{m.group(1)}-{int(m.group(2))}}}"; bump("id-sec-app")
                        else:
                            m=re.match(r'^# Recipe (R\d+) ', line)
                            if m: line=line.rstrip()+f" {{#rec-{m.group(1)}}}"; bump("id-rec")
        out.append(line)
    return "\n".join(out)

# ---------- pass 2: linkify references (prose only) ----------
SEP = r'(?:\s*(?:,|–|-|and|to)\s*'   # separator + next item opener

RE_REC = re.compile(r'\b(Recipes?)\s+(R\d+'+SEP+r'R?\d+)*)')
RE_CH  = re.compile(r'\bChapters?\s+\d+(?:\s*(?:,|–|-|and|to)\s*\d+)*')
RE_APP = re.compile(r'\b(Appendi(?:x|ces))\s+([A-D](?:\s*(?:,|–|-|and|to)\s*[A-D])*)')
RE_SEC = re.compile(r'§\s*(?:([A-D])\.(\d+)|(\d+)\.(\d+)|(\d+))')
RE_SECTION = re.compile(r'\bSection\s+(?:(\d+)\.(\d+)|(\d+))')

def make(prefix):
    def link_ch_num(d):
        n=int(d.group(0)); return f"[{d.group(0)}]({prefix}{CH[n]}#ch-{n})"
    def repl_ch(m):
        s=m.group(0); nums=re.findall(r'\d+',s)
        if len(nums)==1:
            n=int(nums[0]); bump("ch"); return f"[{s}]({prefix}{CH[n]}#ch-{n})"
        bump("ch",len(nums)); return re.sub(r'\d+',link_ch_num,s)
    def repl_rec(m):
        kw=m.group(1); lst=m.group(2); nums=re.findall(r'\d+',lst)
        if len(nums)==1:
            n=int(nums[0]); bump("rec"); return f"[{kw} {lst}]({prefix}{REC[n]}#rec-R{n})"
        bump("rec",len(nums))
        linked=re.sub(r'R?(\d+)',lambda d:f"[R{d.group(1)}]({prefix}{REC[int(d.group(1))]}#rec-R{int(d.group(1))})",lst)
        return f"{kw} {linked}"
    def repl_app(m):
        kw=m.group(1); lst=m.group(2); letters=re.findall(r'[A-D]',lst)
        if len(letters)==1:
            L=letters[0]; bump("app"); return f"[{kw} {lst}]({prefix}{APP[L]}#app-{L})"
        bump("app",len(letters))
        linked=re.sub(r'[A-D]',lambda d:f"[{d.group(0)}]({prefix}{APP[d.group(0)]}#app-{d.group(0)})",lst)
        return f"{kw} {linked}"
    def repl_sec(m):
        bump("sec")
        if m.group(1):
            L,n=m.group(1),int(m.group(2)); return f"[{m.group(0)}]({prefix}{APP[L]}#sec-{L}-{n})"
        if m.group(3):
            c,n=int(m.group(3)),int(m.group(4)); return f"[{m.group(0)}]({prefix}{CH[c]}#sec-{c}-{n})"
        c=int(m.group(5)); return f"[{m.group(0)}]({prefix}{CH[c]}#ch-{c})"
    def repl_section(m):
        bump("section")
        if m.group(1):
            c,n=int(m.group(1)),int(m.group(2)); return f"[{m.group(0)}]({prefix}{CH[c]}#sec-{c}-{n})"
        c=int(m.group(3)); return f"[{m.group(0)}]({prefix}{CH[c]}#ch-{c})"
    def transform_prose(seg):
        seg=RE_REC.sub(repl_rec,seg)
        seg=RE_CH.sub(repl_ch,seg)
        seg=RE_APP.sub(repl_app,seg)
        seg=RE_SECTION.sub(repl_section,seg)
        seg=RE_SEC.sub(repl_sec,seg)
        return seg
    return transform_prose

CODE_RE = re.compile(r'```.*?```|`[^`]*`', re.DOTALL)
def linkify(text, prefix):
    tp=make(prefix); out=[]; last=0
    for m in CODE_RE.finditer(text):
        out.append(tp(text[last:m.start()])); out.append(m.group(0)); last=m.end()
    out.append(tp(text[last:]))
    return "".join(out)

def process(path, prefix):
    with open(path) as f: text=f.read()
    text=add_ids(text)
    text=linkify(text, prefix)
    return text

if __name__=="__main__":
    src=sys.argv[1]; dst=sys.argv[2]
    files=[("index.md","chapters/")]+[(f,"") for f in sorted(os.listdir(os.path.join(src,"chapters")))
                                       if f.endswith(".md")]
    for rel,prefix in files:
        spath=os.path.join(src, rel if rel=="index.md" else "chapters/"+rel)
        dpath=os.path.join(dst, rel if rel=="index.md" else "chapters/"+rel)
        os.makedirs(os.path.dirname(dpath), exist_ok=True)
        with open(dpath,"w") as f: f.write(process(spath, prefix))
    print("STATS:", dict(sorted(STATS.items())))
