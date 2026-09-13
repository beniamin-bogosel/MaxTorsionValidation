#!/usr/bin/env python3
"""Refresh the course's traceability files and tables from local sources.

Does not run certification or modify the manuscript/code. The copied
bibliography is maintained separately because it has a course-only entry.
"""
import ast
from datetime import date
from decimal import Decimal
import hashlib
import json
from pathlib import Path
import re

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
CODE = ROOT / 'MaxTorsionValidation'


def functions(path):
    source = path.read_text()
    if path.suffix == '.py':
        return sorted(
            ({'name': node.name, 'line': node.lineno}
             for node in ast.walk(ast.parse(source))
             if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef))),
            key=lambda item: item['line'])
    if path.suffix == '.c':
        pattern = r'^(?:static\s+)?(?:inline\s+)?(?:unsigned\s+)?(?:void|int|long|char|arb_ptr|tri_key_t)\s*\**\s*(\w+)\s*\('
    elif path.suffix == '.edp':
        pattern = r'^problem\s+(\w+)\s*\('
    else:
        return []
    return [{'name': match.group(1), 'line': source.count('\n', 0, match.start()) + 1}
            for match in re.finditer(pattern, source, re.M)]


def scientific(value):
    value = Decimal(value)
    exponent = value.adjusted()
    mantissa = value.scaleb(-exponent)
    return rf'${mantissa}\,10^{{{exponent}}}$'


def main():
    paths = [ROOT / 'theory/torsional_rigidity_regular_polygon.tex',
             ROOT / 'theory/torsional_rigidity_regular_polygon.aux',
             ROOT / 'theory/torsional_rigidity_regular_polygon.bib',
             ROOT / 'Papers/Polya_MinLoc_reviewed.pdf', ROOT / 'PROVENANCE.md']
    paths += [CODE / 'flint' / name for name in (
        'second_lifting_cert.c', 'mode_cert.c', 'majorant_cert.c',
        'residual_cert.c', 'second_variation_cert.c', 'Makefile')]
    paths += [CODE / 'freefem' / name for name in (
        'certify_with_rotations.py', 'rotation_cache.py', 'certificate_build.py',
        'second_variation_lifting_rotations.edp', 'torsion_hessian_rotations.edp',
        'test_extended_certificates.py', 'summarize_extended_certificates.py')]
    paths += [CODE / 'results' / f'extended_m{m}_table_data.json' for m in (64, 128)]
    paths += [CODE / 'results/validity_review_20260909' / name for name in (
        'README.md', 'n21-replay-provenance.json', 'archive-integrity.json',
        'archive-hash-mismatches.json')]
    records = [{'path': str(path.relative_to(ROOT)),
                'sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
                'functions': functions(path) if path.suffix in ('.py', '.c', '.edp') else []}
               for path in paths]
    (HERE / 'sources.json').write_text(json.dumps({
        'snapshot_date': date.today().isoformat(),
        'scope': 'Sources consulted for the course, not a new certification run',
        'files': records}, indent=2) + '\n')
    index = ['# Source locations for Lecture 3\n',
             'Line numbers refer to the source hashes in [sources.json](sources.json). '
             'The lecture explains every function listed below; named FreeFEM '
             'problems are included as entry points.\n']
    for item in records:
        if not item['functions']:
            continue
        index += [f"## [{item['path']}](../{item['path']})\n", '| Function / problem | Line |', '| --- | ---: |']
        index += [f"| `{entry['name']}` | {entry['line']} |" for entry in item['functions']]
        index += ['']
    (HERE / 'source_index.md').write_text('\n'.join(index) + '\n')

    # Preserve the manuscript's current numbers without importing its whole
    # auxiliary file (which would conflict with the standalone lecture labels).
    aux = (ROOT / 'theory/torsional_rigidity_regular_polygon.aux').read_text()
    labels = re.findall(r'\\newlabel\{([^}]+)\}\{\{([^}]+)\}', aux)
    (HERE / 'manuscript_numbers.tex').write_text(
        '% Snapshot of manuscript labels; regenerate with refresh_sources.py.\n' +
        ''.join(rf'\expandafter\def\csname manuscript:{label}\endcsname{{{number}}}' + '\n'
                for label, number in labels))

    tables = ['% Generated from archived table JSON; no new computations.\n']
    for m in (64, 128):
        rows = json.loads((CODE / 'results' / f'extended_m{m}_table_data.json').read_text())
        with_time = m == 128
        tables += [rf'\subsection*{{The $m={m}$ computations}}',
                   r'\begin{center}\small',
                   r'\begin{tabular}{@{}rrr' + ('rrl' if with_time else 'rl') + '@{}}',
                   r'\toprule',
                   '$n$ & Triangles & Negative / required & Upper bound & ' +
                   ('Minutes & ' if with_time else '') + r'Outcome\\\midrule']
        for row in rows:
            assert row['m'] == m and row['triangles'] == row['n'] * m * m
            assert row['required'] == 2 * row['n'] - 4
            assert Decimal(row['table_upper']) >= Decimal(row['upper'])
            certified = row['status'] == 'CERTIFIED'
            if certified:
                assert row['negative'] == row['required']
                assert row['additional_unresolved'] == 0 and Decimal(row['table_upper']) < 0
            cells = [str(row['n']), str(row['triangles']),
                     f"{row['negative']} / {row['required']}", scientific(row['table_upper'])]
            if with_time:
                cells.append(f"{row['seconds'] / 60:.1f}")
            cells.append('certified' if certified else 'inconclusive')
            tables.append(' & '.join(cells) + r'\\')
        tables += [r'\bottomrule', r'\end{tabular}\end{center}', '']
    (HERE / 'results_tables.tex').write_text('\n'.join(tables))
    print(f'Recorded {len(records)} source files and generated the m=64,128 tables.')


if __name__ == '__main__':
    main()
