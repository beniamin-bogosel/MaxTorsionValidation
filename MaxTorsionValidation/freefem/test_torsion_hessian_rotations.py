#!/usr/bin/env python3
"""Compare the three-solve benchmark with the unchanged full-solve code.

Temporary source copies add only field/matrix output. No certificate driver
is called, and no validated sign claim is inferred from these comparisons.
"""
import argparse
from array import array
import gzip
import hashlib
import json
import math
import os
from pathlib import Path
import shutil
import subprocess
import time

ROOT = Path(__file__).resolve().parents[2]
EXPORT = r'''
string reviewOutput=getARGV("-review-output", "");
ofstream review(reviewOutput); review.precision(17);
review << n << " " << m << " " << Th.nv << endl;
for (int p=0; p<Th.nv; ++p) review << u[][p] << endl;
for (int i=0; i<n; ++i) {
  for (int p=0; p<Th.nv; ++p) review << Ux[i][][p] << endl;
  for (int p=0; p<Th.nv; ++p) review << Uy[i][][p] << endl;
}
for (int i=0; i<2*n; ++i) for (int j=0; j<2*n; ++j)
  review << HF(i,j) << endl;
'''


def modes(log, n):
    rows = {}
    for line in log.splitlines():
        if line.startswith('MODE_DATA '):
            parts = line.split()
            k = int(parts[1])
            if k in rows or len(parts) != 6:
                raise ValueError('Invalid symbol output')
            rows[k] = [float(x) for x in parts[2:]]
    if set(rows) != set(range(n)):
        raise ValueError('Incomplete symbol output')
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--archive', type=Path, required=True)
    args = parser.parse_args()
    archive = args.archive.resolve()
    archive.mkdir(parents=True, exist_ok=False)
    os.sched_setaffinity(0, {min(os.sched_getaffinity(0))})
    os.nice(10)
    for name in ['OMP_NUM_THREADS', 'OPENBLAS_NUM_THREADS', 'MKL_NUM_THREADS',
                 'BLIS_NUM_THREADS', 'NUMEXPR_NUM_THREADS']:
        os.environ[name] = '1'
    ff = os.environ.get('FREEFEM_BIN', 'FreeFem++')
    sources = {}
    for name in ['torsion_hessian', 'torsion_hessian_rotations']:
        source = ROOT / 'MaxTorsionValidation/freefem' / (name + '.edp')
        sources[name] = hashlib.sha256(source.read_bytes()).hexdigest()
        shutil.copy2(source, archive / source.name)
        (archive / (name + '_review.edp')).write_text(source.read_text() + EXPORT)
    cases = [(3, 8), (4, 8), (5, 16), (6, 16), (7, 12), (11, 32), (21, 16)]
    (archive / 'MANIFEST.json').write_text(json.dumps(dict(
        cases=cases, source_sha256=sources, absolute_tolerance=1e-10,
        cpu_affinity=sorted(os.sched_getaffinity(0)),
        freefem=shutil.which(ff)), indent=2) + '\n')
    results = []
    for n, m in cases:
        folder = archive / f'n{n}_m{m}'
        folder.mkdir()
        values, symbols, seconds = [], [], []
        for name in sources:
            data = folder / (name + '.txt')
            log = folder / (name + '.log')
            start = time.monotonic()
            with log.open('w') as output:
                subprocess.run([ff, '-v', '0', '-nw',
                                str(archive / (name + '_review.edp')),
                                '-n', str(n), '-m', str(m),
                                '-review-output', str(data)],
                               stdout=output, stderr=subprocess.STDOUT, check=True)
            seconds.append(time.monotonic() - start)
            output = log.read_text()
            symbols.append(modes(output, n))
            if name.endswith('_rotations') and (
                    'PDE_SOLVES torsion=1 first_vertex=2 total=3' not in output
                    or 'cycle=PASS' not in output):
                raise ValueError('Missing rotation checks')
            with data.open() as stream:
                header = list(map(int, stream.readline().split()))
                nv = 1 + n*m*(m+1)//2
                if header != [n, m, nv]:
                    raise ValueError('Unexpected mesh dimensions')
                numbers = array('d', (float(line) for line in stream))
                if len(numbers) != (1+2*n)*nv + 4*n*n:
                    raise ValueError('Incomplete field/matrix output')
                if not all(math.isfinite(x) for x in numbers):
                    raise ValueError('Nonfinite field/matrix output')
                values.append(numbers)
            with data.open('rb') as src, gzip.open(str(data)+'.gz', 'wb') as dst:
                shutil.copyfileobj(src, dst)
            data.unlink()
        errors = [abs(a-b) for a, b in zip(*values)]
        symbol_error = max(abs(a-b) for k in range(n)
                           for a, b in zip(symbols[0][k], symbols[1][k]))
        row = dict(n=n, m=m, state_error=max(errors[:nv]),
                   first_variation_error=max(errors[nv:(1+2*n)*nv]),
                   hessian_error=max(errors[(1+2*n)*nv:]),
                   symbol_error=symbol_error, original_seconds=seconds[0],
                   rotation_seconds=seconds[1])
        if any(row[key] > 1e-10 for key in [
                'state_error', 'first_variation_error', 'hessian_error', 'symbol_error']):
            raise ValueError(f'Comparison failed: {row}')
        results.append(row)
        print(json.dumps(row), flush=True)
    (archive / 'RESULTS.json').write_text(json.dumps(results, indent=2)+'\n')
    (archive / 'PASS').write_text('All first-variation fields, full Hessians and symbols agree within the stated floating-point tolerance.\n')


if __name__ == '__main__':
    main()
