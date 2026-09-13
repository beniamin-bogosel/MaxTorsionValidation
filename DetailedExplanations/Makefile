DOCS = lecture1 lecture2 lecture3 course
LATEX = pdflatex -interaction=nonstopmode -halt-on-error -file-line-error
.DELETE_ON_ERROR:

.PHONY: all refresh
all: $(DOCS:%=%.pdf)

%.pdf: %.tex preamble.tex lecture1_body.tex lecture2_body.tex lecture3_body.tex references.bib manuscript_numbers.tex results_tables.tex
	$(LATEX) $*.tex > $*.build.log
	bibtex $* >> $*.build.log
	$(LATEX) $*.tex >> $*.build.log
	$(LATEX) $*.tex >> $*.build.log

refresh:
	python3 refresh_sources.py
