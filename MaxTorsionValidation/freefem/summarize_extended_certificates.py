#!/usr/bin/env python3
"""Audit completed sweep records and generate outward-rounded table data."""
import argparse
from decimal import Decimal, ROUND_CEILING, localcontext
import hashlib
import json
from pathlib import Path
import re


def audit(archive):
    manifest = json.loads((archive / 'MANIFEST.json').read_text())
    for source, expected in manifest['sources'].items():
        saved = archive / 'sources' / Path(source).name
        if hashlib.sha256(saved.read_bytes()).hexdigest() != expected:
            raise ValueError(f'Source hash mismatch: {saved}')
    for result_file in sorted(archive.glob('n*/RESULT.json')):
        result = json.loads(result_file.read_text())
        n, m = result['n'], result['m']
        folder = result_file.parent
        if m != manifest['m'] or manifest['precision'] != 192:
            raise ValueError(f'Unexpected mesh or precision: {folder}')
        radii = {}
        for line in (folder / 'radii.txt').read_text().splitlines():
            tag, k, radius = line.split()
            if tag != 'MODE_RADIUS' or int(k) in radii:
                raise ValueError(f'Invalid radius record: {folder}')
            radii[int(k)] = radius
        if set(radii) != set(range(n)):
            raise ValueError(f'Incomplete radius file: {folder}')
        entry_count = 0
        for k in range(1, n // 2 + 1):
            entries = ['rr', 'tt'] if 2*k == n else ['rr', 'tt', 'rt_re', 'rt_im']
            for entry in entries:
                log = (folder / f'k{k}_{entry}_cert.log').read_text()
                marker = re.findall(r'ENTRY_CERTIFIED k=(\d+) conjugate_k=(\d+) '
                                    r'entry=(\S+) max_radius=(\S+)', log)
                if (marker != [(str(k), str(n-k), entry, radii[k])]
                        or radii[k] != radii[n-k]
                        or 'exact_mesh_topology=PASS' not in log
                        or 'exact_regular_fan_inputs=PASS' not in log
                        or not (folder / 'inputs' / f'k{k}_{entry}.txt.gz').is_file()):
                    raise ValueError(f'Incomplete entry certificate: {folder}, {k}, {entry}')
                entry_count += 1
        log = (folder / 'mode_cert.log').read_text()
        count = re.search(r'summary symbols=(\d+) certified_negative=(\d+) '
                          r'unresolved=(\d+) radius=per-mode prec=192', log)
        if not count or int(count[1]) != n:
            raise ValueError(f'Incomplete mode certificate: {folder}')
        negative, unresolved = int(count[2]), int(count[3])
        if negative + unresolved != 2*n or unresolved < 4:
            raise ValueError(f'Unexpected inertia; inspect possible positive eigenvalues: {folder}')
        passed = negative == 2*n-4 and unresolved == 4
        if ((result['status'] == 'CERTIFIED') != passed
                or result['mode_exit_code'] != (0 if passed else 3)):
            raise ValueError(f'Result/count disagreement: {folder}')
        if passed and f'MODE_CERTIFIED expected_symbols={n} expected_negative={2*n-4} expected_unresolved=4' not in log:
            raise ValueError(f'Missing final assertion: {folder}')
        bounds = []
        failed = []
        modes_seen = set()
        for line in log.splitlines():
            mode = re.match(r'k=(\d+) ', line)
            if not mode:
                continue
            k = int(mode[1])
            if k in modes_seen:
                raise ValueError(f'Duplicate mode: {folder}')
            modes_seen.add(k)
            for branch, mid, rad, flag in re.findall(
                    r'lambda_(minus|plus)=\[([^ ]+) \+/- ([^\]]+)\] (NEG|\?)', line):
                with localcontext() as context:
                    context.prec = 80
                    upper = Decimal(mid) + Decimal(rad)
                bounds.append(upper)
                if flag != 'NEG':
                    failed.append([k, branch])
        if modes_seen != set(range(n)) or len(bounds) != 2*n-4:
            raise ValueError(f'Unexpected spectrum records: {folder}')
        upper = max(bounds)
        # Ceiling is also necessary for negative endpoints: do not round a
        # negative upper bound farther away from zero when preparing a table.
        table_upper = upper.quantize(Decimal(1).scaleb(upper.adjusted()-2),
                                     rounding=ROUND_CEILING)
        yield dict(n=n, m=m, triangles=n*m*m, negative=negative,
                   required=2*n-4, additional_unresolved=unresolved-4,
                   upper=str(upper), table_upper=str(table_upper),
                   status=result['status'], failed_branches=failed,
                   entry_count=entry_count, seconds=result['seconds'],
                   record=str(folder / 'mode_cert.log'))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('archives', nargs='+', type=Path)
    parser.add_argument('--json', type=Path, required=True)
    args = parser.parse_args()
    rows = sorted((row for archive in args.archives for row in audit(archive)),
                  key=lambda row: (row['m'], row['n']))
    if len({(row['m'], row['n']) for row in rows}) != len(rows):
        raise ValueError('Duplicate completed mesh/polygon records')
    args.json.write_text(json.dumps(rows, indent=2)+'\n')
    for row in rows:
        print(f"m={row['m']} n={row['n']}: {row['negative']}/{row['required']} negative, "
              f"upper<={row['table_upper']}, {row['status']}")


if __name__ == '__main__':
    main()
