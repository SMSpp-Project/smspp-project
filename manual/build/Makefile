# Makefile for the SMS++ User Manual (single source -> web + PDF).
# Run from manual/build/.
#
#   make pdf     # arXiv-ready PDF via Pandoc + pdflatex  -> ../SMS++-manual.pdf
#   make html    # static MkDocs site                     -> ./site/
#   make serve   # live MkDocs preview at 127.0.0.1:8000
#   make figures # (re)generate any missing .svg from .pdf in ../figures
#   make clean   # remove build artefacts
#
# Prerequisites:
#   pandoc, a LaTeX distribution (pdflatex + the usual packages),
#   python3 with mkdocs + mkdocs-material + pymdown-extensions,
#   poppler-utils (pdftocairo) for figure conversion.

MANUAL  := ..
FIGDIR  := $(MANUAL)/figures
PDF_OUT := $(MANUAL)/SMS++-manual.pdf

.PHONY: pdf html serve figures clean

pdf: figures
	pandoc --defaults=pandoc.yaml -o $(PDF_OUT)
	@echo "PDF written to $(PDF_OUT)"

html: figures
	mkdocs build -f mkdocs.yml
	@echo "Static site written to ./site/"

serve:
	mkdocs serve -f mkdocs.yml

# For every figure shipped as a .pdf, ensure a .svg counterpart exists
# for the web build (and vice versa is assumed already present).
figures:
	@for pdf in $(FIGDIR)/*.pdf; do \
	  svg="$${pdf%.pdf}.svg"; \
	  if [ ! -f "$$svg" ]; then \
	    echo "pdftocairo -svg $$pdf -> $$svg"; \
	    pdftocairo -svg "$$pdf" "$$svg"; \
	  fi; \
	done

clean:
	rm -rf ../../manual-site
	rm -f $(PDF_OUT)
