#!/usr/bin/env python3
"""Explore larger regular polygons using the unchanged rigorous C verifiers.

Radii are proposed from printed enclosures, then independently checked by
second_lifting_cert before mode_cert consumes those exact reference balls.
No estimate inferred from mesh convergence is used as a certificate.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
from decimal import Decimal, localcontext
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import time
from certificate_build import build_verifiers

ROOT = Path(__file__).resolve().parents[2]


def run(cmd, log):
    start = time.monotonic()
    with log.open('w') as stream:
        result = subprocess.run([str(x) for x in cmd], stdout=stream,
                                stderr=subprocess.STDOUT)
    if result.returncode:
        raise RuntimeError(f'Command failed ({result.returncode}); see {log}')
    return time.monotonic() - start


def ball(text, key):
    match = re.search(re.escape(key) + r'\[([^ ]+) \+/- ([^\]]+)\]', text)
    if not match:
        raise ValueError(f'Missing finite ball: {key}')
    return Decimal(match[1]), Decimal(match[2])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('first', type=int)
    parser.add_argument('last', type=int)
    parser.add_argument('--m', type=int, default=64)
    parser.add_argument('--jobs', type=int, default=1)
    parser.add_argument('--cpu-cores', type=int, default=1,
                        help='maximum CPU cores shared by this run and all subprocesses')
    parser.add_argument('--stop-on-failure', action='store_true')
    parser.add_argument('--flux-eps', default='1e-13',
                        help='relative tolerance for the auxiliary FreeFEM curl fit only')
    parser.add_argument('--entry-centers', action='store_true',
                        help='use lifting centers as references, avoiding a full floating Hessian')
    parser.add_argument('--archive', type=Path, required=True)
    args = parser.parse_args()
    if not 3 <= args.first <= args.last or args.m < 1 or args.jobs < 1 or args.cpu_cores < 1:
        parser.error('require 3 <= first <= last, m >= 1, jobs >= 1, cpu-cores >= 1')
    # Children inherit affinity and niceness, including BLAS worker threads.
    available = sorted(os.sched_getaffinity(0))
    os.sched_setaffinity(0, available[:args.cpu_cores])
    os.nice(10)
    for variable in ['OMP_NUM_THREADS', 'OPENBLAS_NUM_THREADS', 'MKL_NUM_THREADS',
                     'BLIS_NUM_THREADS', 'NUMEXPR_NUM_THREADS']:
        os.environ[variable] = '1'
    archive = args.archive.resolve()
    archive.mkdir(parents=True, exist_ok=False)
    sources = archive / 'sources'
    sources.mkdir()
    paths = [Path(__file__).resolve(), ROOT/'MaxTorsionValidation/freefem/torsion_hessian.edp',
             ROOT/'MaxTorsionValidation/freefem/second_variation_lifting.edp',
             ROOT/'MaxTorsionValidation/flint/second_lifting_cert.c', ROOT/'MaxTorsionValidation/flint/mode_cert.c',
             ROOT/'MaxTorsionValidation/flint/Makefile', ROOT/'MaxTorsionValidation/freefem/certificate_build.py']
    for path in paths:
        shutil.copy2(path, sources/path.name)
    ff = os.environ.get('FREEFEM_BIN', 'FreeFem++')
    manifest = dict(first=args.first, last=args.last, m=args.m, precision=192,
                    jobs=args.jobs, cpu_affinity=sorted(os.sched_getaffinity(0)),
                    nice=os.nice(0), entry_centers=args.entry_centers, freefem=shutil.which(ff),
                    flux_eps=args.flux_eps, stop_on_failure=args.stop_on_failure,
                    geom_radius='1e-13', direction_radius='1e-13',
                    sources={str(p.relative_to(ROOT)): hashlib.sha256((sources/p.name).read_bytes()).hexdigest()
                             for p in paths})
    (archive/'MANIFEST.json').write_text(json.dumps(manifest, indent=2)+'\n')
    verifiers, manifest['verifier_build'] = build_verifiers(archive, run)
    (archive/'MANIFEST.json').write_text(json.dumps(manifest, indent=2)+'\n')
    cert = [verifiers['second_lifting_cert']]
    options = ['--prec', '192', '--geom-radius', '1e-13',
               '--direction-radius', '1e-13', '--check-regular-inputs']
    for n in range(args.first, args.last+1):
        start = time.monotonic()
        folder = archive/f'n{n}'
        folder.mkdir()
        inputs = folder/'inputs'
        inputs.mkdir()
        modes = folder/'modes.txt'
        print(f'START n={n} m={args.m}', flush=True)
        centers = {k: [Decimal(0) for _ in range(4)] for k in range(n)} if args.entry_centers else {}
        if not args.entry_centers:
            run([ff, '-v', '0', '-nw', ROOT/'MaxTorsionValidation/freefem/torsion_hessian.edp',
                 '-n', n, '-m', args.m], modes)
            for line in modes.read_text().splitlines():
                if line.startswith('MODE_DATA '):
                    parts = line.split()
                    centers[int(parts[1])] = [Decimal(x) for x in parts[2:6]]
        if set(centers) != set(range(n)):
            raise RuntimeError('Incomplete reference modes')
        # Queue entries across modes so all requested workers can stay busy.
        # The reference file is written once, after every center is available,
        # and remains immutable throughout the containment checks.
        specs = []
        for k in range(1, n//2+1):
            specs += [(k, 'rr', 0, 0, 0, 0), (k, 'tt', 1, 0, 1, 0)]
            if 2*k != n:
                specs += [(k, 'rt_re', 0, 0, 1, 0), (k, 'rt_im', 0, 0, 1, 1)]

        def evaluate(spec):
            k, name, qa, qp, ra, rp = spec
            tag = f'k{k}_{name}'
            export = inputs/f'{tag}.txt'
            run([ff, '-v', '0', '-nw', ROOT/'MaxTorsionValidation/freefem/second_variation_lifting.edp',
                 '-n', n, '-m', args.m, '-k', k, '-qa', qa, '-qp', qp,
                 '-ra', ra, '-rp', rp, '-flux-eps', args.flux_eps,
                 '-export-second', export], folder/f'{tag}_ff.log')
            with export.open() as stream:
                header = stream.readline().split()
            expected = ['TORSION_SECOND_V2', *map(str, (n, args.m, k, qa, qp, ra, rp))]
            if header[:8] != expected:
                raise RuntimeError(f'Export metadata does not match requested entry: {tag}')
            log = folder/f'{tag}_estimate.log'
            run(cert+[export]+options, log)
            output = log.read_text()
            if 'exact_regular_fan_inputs=PASS' not in output or 'exact_mesh_topology=PASS' not in output:
                raise RuntimeError('Missing exact-input checks')
            with localcontext() as ctx:
                ctx.prec = 60
                mid, spread = ball(output, 'F_hessian_center=')
                error, error_spread = ball(output, 'F_hessian_error<=')
                index = ['rr', 'tt', 'rt_re', 'rt_im'].index(name)
                reference = mid if args.entry_centers else centers[k][index]
                conjugate = (-mid if name == 'rt_im' else mid) if args.entry_centers else centers[n-k][index]
                differences = [abs(mid-reference),
                               abs((-mid if name == 'rt_im' else mid)-conjugate)]
                proposed = (max(differences)+spread+abs(error)+error_spread)*Decimal('1.00001')+Decimal('1e-25')
            return k, name, export, mid, proposed

        evaluated = []
        with ThreadPoolExecutor(max_workers=args.jobs) as pool:
            for item in pool.map(evaluate, specs):
                evaluated.append(item)
                k, name, _, mid, _ = item
                if args.entry_centers:
                    index = ['rr', 'tt', 'rt_re', 'rt_im'].index(name)
                    centers[k][index] = mid
                    centers[n-k][index] = -mid if name == 'rt_im' else mid
                print(f'ENTRY_EVALUATED n={n} k={k} entry={name} elapsed={time.monotonic()-start:.1f}s', flush=True)
        if args.entry_centers:
            modes.write_text(''.join(f'MODE_DATA {j} '+ ' '.join(str(x) for x in centers[j])+'\n'
                                     for j in range(n)))
        radii = {0: '0'}
        for k in range(1, n//2+1):
            radius = str(max(item[4] for item in evaluated if item[0] == k))
            radii[k] = radii[n-k] = radius

        # The decimal calculation only proposes a radius. These rigorous
        # containment checks are authoritative, including conjugate modes.
        def verify(item):
            k, name, export, _, _ = item
            log = folder/f'k{k}_{name}_cert.log'
            run(cert+[export]+options+['--reference', modes, '--entry', name,
                                       '--max-radius', radii[k]], log)
            markers = re.findall(r'ENTRY_CERTIFIED k=(\d+) conjugate_k=(\d+) '
                                 r'entry=(\S+) max_radius=(\S+)', log.read_text())
            if markers != [(str(k), str(n-k), name, radii[k])]:
                raise RuntimeError('Missing or mismatched entry certificate')
            with export.open('rb') as src, (Path(str(export)+'.gz')).open('wb') as raw:
                with gzip.GzipFile(fileobj=raw, mode='wb', filename='', mtime=0) as dst:
                    shutil.copyfileobj(src, dst)
            export.unlink()
            return k

        checked = {}
        with ThreadPoolExecutor(max_workers=args.jobs) as pool:
            for k in pool.map(verify, evaluated):
                checked[k] = checked.get(k, 0) + 1
                if checked[k] == (2 if 2*k == n else 4):
                    print(f'ENTRIES_CERTIFIED n={n} k={k} radius={float(radii[k]):.8e} elapsed={time.monotonic()-start:.1f}s', flush=True)
        radius_file = folder/'radii.txt'
        radius_file.write_text(''.join(f'MODE_RADIUS {k} {radii[k]}\n' for k in range(n)))
        command = [str(x) for x in [verifiers['mode_cert'], '--prec', '192',
                   '--radius-file', radius_file, '--exact-regular-similarities',
                   '--expect-symbols', n, '--expect-negative', 2*n-4,
                   '--expect-unresolved', 4, modes]]
        with (folder/'mode_cert.log').open('w') as log:
            result = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT)
        if result.returncode not in (0, 3):
            raise RuntimeError(f'Mode verifier execution failed; see {folder / "mode_cert.log"}')
        status = 'CERTIFIED' if result.returncode == 0 else 'NOT_CERTIFIED'
        summary = dict(n=n, m=args.m, status=status, seconds=time.monotonic()-start,
                       mode_exit_code=result.returncode)
        (folder/'RESULT.json').write_text(json.dumps(summary, indent=2)+'\n')
        print(json.dumps(summary), flush=True)
        with (archive/'results.jsonl').open('a') as out:
            out.write(json.dumps(summary)+'\n')
        if args.stop_on_failure and result.returncode:
            break
    hashes = []
    for path in sorted(archive.rglob('*')):
        if path.is_file():
            hashes.append(f'{hashlib.sha256(path.read_bytes()).hexdigest()}  {path.relative_to(archive)}\n')
    (archive/'SHA256SUMS').write_text(''.join(hashes))
    (archive/'FINISHED').write_text('Sweep finished; see results.jsonl for tested n and outcomes.\n')


if __name__ == '__main__':
    main()
