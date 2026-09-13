# Three lectures on torsion and validated local maximality

Start with [the combined course](course.pdf), or use the individual lectures:

1. [Lecture 1 — PDF](lecture1.pdf), [LaTeX](lecture1.tex): Laurain's distributed second derivative, vertex blocks, normalization by area, Fourier reduction, eigenvalues, and similarity zeros.
2. [Lecture 2 — PDF](lecture2.pdf), [LaTeX](lecture2.tex): Galerkin and linear-solver errors, equilibrated fluxes, the second-lifting identity, and a regularity proof attempt through ray sources.
3. [Lecture 3 — PDF](lecture3.pdf), [LaTeX](lecture3.tex): function-by-function explanation of the maintained verification chain, commands, archived results through n=25, and recorded runtimes.

The wrappers share `preamble.tex`, `references.bib`, and the three `lecture*_body.tex` files. Editing a body updates both its standalone lecture and [course.tex](course.tex). Display mathematics uses LaTeX display environments in these source files. No changes to the manuscript or numerical implementation are needed to use the course.

Lecture 2 adapts the ray argument of Bogosel–Bucur and the polygonal mixed-boundary shift theorem of Grisvard. It gives a qualitative argument for broken H² regularity of first and second material derivatives **at the fixed regular polygon**, including a local lifting for the second-derivative ray densities at the centre. It does not claim the manuscript's stronger uniform-neighborhood conjecture is proved. This distinction is also made explicitly in the PDF. The rate conclusion concerns exact Galerkin Hessians; a quadratic rate for the computed certificate radius additionally requires appropriate flux and algebraic error rates.

## Build

From the repository root:

```bash
make -C DetailedExplanations
```

This uses `pdflatex` and `bibtex` and produces all four PDFs. The LaTeX installation needs the standard packages listed in `preamble.tex`, including TikZ, listings, and xurl; no shell escape is used.

## Traceability

[source_index.md](source_index.md) lists the functions and their source locations. [sources.json](sources.json) records hashes of the consulted manuscript, code, local reviewed Bogosel–Bucur PDF, and numerical table records. The lecture cites the reviewed local PDF's Lemma 3.1 and Remark 3.2; numbering differs in the earlier arXiv version. The bibliography retains the manuscript's bibliographic keys.

[manuscript_numbers.tex](manuscript_numbers.tex) stores equation and statement numbers from the current manuscript auxiliary file. [results_tables.tex](results_tables.tex) is generated directly from the archived m=64 and m=128 JSON rows, checking dimensions, required counts, and outward rounding. The m=32 values are transcribed from the manuscript's safe-upper-bound table.

To intentionally refresh the source snapshot, manuscript numbers, and generated tables after updating the underlying project:

```bash
make -C DetailedExplanations refresh
make -C DetailedExplanations
```

Refreshing reads local sources only; it does not run certification. Keep the manuscript's `.aux` synchronized with its `.tex` before refreshing. Historical archives retain their original `code/` path strings after the packaging rename to `MaxTorsionValidation/`; the course explains how to locate them and records the existing n=21 checksum exception and successful entry replay.
