"""Untrusted Fourier candidates from three first-vertex PDE solutions.

The existing Arb verifier checks the exported fields independently. This
module's mesh checks and floating Fourier transforms have no proof authority.
"""
from pathlib import Path


def prepare(basis_path, folder):
    import numpy as np

    with Path(basis_path).open() as stream:
        header = stream.readline().split()
        if header[0] != 'TORSION_ROTATION_BASIS_V1':
            raise ValueError('Unexpected basis format')
        n, m, _, qa, qp, ra, rp, nv, nt = map(int, header[1:])
        if (qa, qp, ra, rp) != (0, 0, 1, 0):
            raise ValueError('Expected the two first-vertex directions')
        numbers = np.fromfile(stream, sep=' ')
    if nv != 1+n*m*(m+1)//2 or nt != n*m*m:
        raise ValueError('Unexpected mesh dimensions')
    if numbers.size != 2*nv+3*nt+12*nv or not np.isfinite(numbers).all():
        raise ValueError('Incomplete or nonfinite basis fields')
    coordinates = numbers[:2*nv].reshape(nv, 2)
    fields = numbers[2*nv+3*nt:].reshape(12, nv)
    theta = 2*np.pi/n
    vertices = np.column_stack((np.cos(np.arange(n)*theta),
                                np.sin(np.arange(n)*theta)))
    angles = np.mod(np.arctan2(coordinates[:, 1], coordinates[:, 0]), 2*np.pi)
    sector = np.floor(angles/theta).astype(np.int64) % n
    following = (sector+1) % n
    a = np.rint(m*(coordinates[:, 0]*vertices[following, 1]
                  - coordinates[:, 1]*vertices[following, 0])/np.sin(theta)).astype(np.int64)
    b = np.rint(m*(vertices[sector, 0]*coordinates[:, 1]
                  - vertices[sector, 1]*coordinates[:, 0])/np.sin(theta)).astype(np.int64)
    center = np.abs(coordinates).sum(axis=1) < 1e-12
    if center.sum() != 1 or np.any((a < 0) | (b < 0) | (a+b > m)):
        raise ValueError('Invalid fan lattice coordinates')
    ray = (a == 0) & ~center
    sector[ray] = following[ray]
    a[ray], b[ray] = b[ray], 0
    stride = (m+1)**2
    key = sector*stride+a*(m+1)+b
    key[center] = n*stride
    if np.unique(key).size != nv:
        raise ValueError('Duplicate fan lattice node')
    lookup = np.full(n*stride+1, -1, dtype=np.int64)
    lookup[key] = np.arange(nv)
    previous_key = (key-stride) % (n*stride)
    previous_key[center] = n*stride
    previous = lookup[previous_key]
    if np.any(previous < 0) or np.unique(previous).size != nv:
        raise ValueError('Rotation is not a vertex permutation')
    rotation = np.array([[np.cos(theta), -np.sin(theta)],
                         [np.sin(theta), np.cos(theta)]])
    defect = np.max(np.linalg.norm(coordinates[previous]-coordinates@rotation, axis=1))
    if defect > 1e-10:
        raise ValueError('Incorrect rotation coordinates')
    indices = np.empty((n, nv), dtype=np.int64)
    indices[0] = np.arange(nv)
    for j in range(1, n):
        indices[j] = previous[indices[j-1]]
    if not np.array_equal(previous[indices[-1]], indices[0]):
        raise ValueError('Rotation cycle failed')

    # At vertex j, radial/tangential directions are R_j e_1 / R_j e_2.
    # Their scalar derivatives and curl potentials need only pullback by R_j^T.
    # The real / negative imaginary FFT parts give cosine / sine sums.
    fourier = [np.fft.rfft(fields[i][indices], axis=0) for i in [5, 6, 9, 10]]
    del indices
    folder = Path(folder)
    folder.mkdir(exist_ok=False)
    for k in range(1, n//2+1):
        norm = np.sqrt(n if 2*k == n else n/2)

        def direction(axis, phase):
            parts = [fourier[axis][k], fourier[axis+2][k]]
            return [(z.real if phase == 0 else -z.imag)/norm for z in parts]

        specs = [('rr', 0, 0, 0, 0), ('tt', 1, 0, 1, 0)]
        if 2*k != n:
            specs += [('rt_re', 0, 0, 1, 0), ('rt_im', 0, 0, 1, 1)]
        for name, qa, qp, ra, rp in specs:
            uq, psiq = direction(qa, qp)
            ur, psir = direction(ra, rp)
            with (folder/f'k{k}_{name}.txt').open('w') as stream:
                stream.write(f'TORSION_ROTATED_FIELDS_V1 {n} {m} {k} {qa} {qp} {ra} {rp} {nv}\n')
                np.savetxt(stream, coordinates, fmt='%.17g')
                for values in [fields[4], uq, ur, fields[8], psiq, psir]:
                    np.savetxt(stream, values, fmt='%.17g')
                stream.write('END_ROTATED_FIELDS\n')
    return dict(n=n, m=m, coordinate_defect=float(defect),
                state_rotation_defect=float(np.max(np.abs(fields[4]-fields[4][previous]))),
                cycle='PASS', pde_solves=3, auxiliary_flux_fits=3)
