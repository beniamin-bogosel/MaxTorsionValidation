#!/usr/bin/env python3
"""Reject malformed inputs and invalid geometry-to-symbol certificate links."""
import math
from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parent


def rejected(program, *args, expected_error=None):
    result = subprocess.run([str(ROOT / program), *map(str, args)],
                            capture_output=True, text=True)
    if result.returncode != 2:
        raise AssertionError((program, args, result.returncode,
                              result.stdout, result.stderr))
    assert "MODE_CERTIFIED" not in result.stdout
    assert "ENTRY_CERTIFIED" not in result.stdout
    if expected_error is not None:
        assert expected_error in result.stderr, (expected_error, result.stderr)
    return result


def pentagon_fan_fixture():
    """Small geometric fixture with zero, hence untrusted, PDE candidates."""
    vertices = [(0.0, 0.0)] + [
        (math.cos(2 * math.pi * j / 5), math.sin(2 * math.pi * j / 5))
        for j in range(5)
    ]
    lines = ["TORSION_SECOND_V2 5 1 1 0 0 0 0 6 5"]
    lines += [f"{x:.17g} {y:.17g}" for x, y in vertices]
    lines += [f"0 {j + 1} {(j + 1) % 5 + 1}" for j in range(5)]
    qx = [0.0] + [x * x / math.sqrt(2.5) for x, _ in vertices[1:]]
    qy = [0.0] + [x * y / math.sqrt(2.5) for x, y in vertices[1:]]
    for field in (qx, qy, qx, qy, *([[0.0] * 6] * 8)):
        lines.append(" ".join(f"{value:.17g}" for value in field))
    return lines


with tempfile.TemporaryDirectory() as directory:
    data = Path(directory) / "input.txt"
    for tail in ("1", "1 1 0.1", "1 1 0.1 0.1 0.1 0.1"):
        data.write_text("0 0 0.1 0.1 0.1 0.1 1\n" + tail)
        rejected("second_variation_cert", data)
    for value in ("nan", "inf", "-inf"):
        data.write_text(f"0 0 0.1 0.1 0.1 0.1 {value}\n")
        rejected("second_variation_cert", data)
        data.write_text(f"MODE_DATA 0 {value} 0 0 0\n"
                        "MODE_DATA 1 -1 -1 0 0\nMODE_DATA 2 -1 -1 0 0\n")
        rejected("mode_cert", "--exact-regular-similarities",
                 "--expect-symbols", 3, "--expect-negative", 2,
                 "--expect-unresolved", 4, data)
        for option in ("--geom-radius", "--direction-radius"):
            rejected("second_lifting_cert", ROOT / "test_second_lifting.txt",
                     "--check-regular-inputs", option, value)
        rejected("second_lifting_cert", ROOT / "test_second_lifting.txt",
                 "--check-regular-inputs", "--reference",
                 ROOT / "test_regular_n4_modes.txt", "--entry", "rr",
                 "--max-radius", value)
    # A nonfinite candidate must be rejected even if its boundary value would
    # subsequently be replaced by exact zero.
    lines = (ROOT / "test_second_lifting.txt").read_text().splitlines()
    header = lines[0].split()
    nv, nt = map(int, header[-2:])
    tokens = " ".join(lines[1 + nv + nt:]).split()
    tokens[4 * nv] = "nan"
    data.write_text("\n".join(lines[:1 + nv + nt]) + "\n" + " ".join(tokens))
    rejected("second_lifting_cert", data, "--check-regular-inputs")

    # A connected combinatorial disk with the right counts can still be the
    # wrong geometric fan.  Relabel two boundary vertices only in its
    # connectivity: all triangles stay nondegenerate, but the incidence is
    # incompatible with the exact pentagon coordinates.
    lines = pentagon_fan_fixture()
    data.write_text("\n".join(lines) + "\n")
    valid = subprocess.run([str(ROOT / "second_lifting_cert"), str(data),
                            "--check-regular-inputs"],
                           capture_output=True, text=True)
    assert valid.returncode == 0, (valid.stdout, valid.stderr)
    assert "exact_regular_fan_inputs=PASS" in valid.stdout
    swap = {1: 2, 2: 1}
    for row in range(7, 12):
        lines[row] = " ".join(str(swap.get(i, i))
                              for i in map(int, lines[row].split()))
    data.write_text("\n".join(lines) + "\n")
    wrong_fan = rejected("second_lifting_cert", data,
                         "--check-regular-inputs",
                         expected_error="exported mesh is not the canonical regular fan")
    assert "exact_mesh_topology=PASS" in wrong_fan.stdout

    # A known enclosure [-0.677, 0.677] fits these reference balls.  Move
    # only one center to ensure the direct and conjugate checks each reject.
    reference = Path(directory) / "reference.txt"
    reference.write_text("MODE_DATA 1 0 0 0 0\nMODE_DATA 3 0 0 0 0\n")
    containment_args = [ROOT / "test_second_lifting.txt",
                        "--check-regular-inputs", "--reference", reference,
                        "--entry", "rr", "--max-radius", "1"]
    valid = subprocess.run([str(ROOT / "second_lifting_cert"),
                            *map(str, containment_args)],
                           capture_output=True, text=True)
    assert valid.returncode == 0, (valid.stdout, valid.stderr)
    assert valid.stdout.count("ENTRY_CERTIFIED ") == 1
    rejected("second_lifting_cert", ROOT / "test_second_lifting.txt",
             "--check-regular-inputs", "--reference", reference,
             "--entry", "tt", "--max-radius", "1",
             expected_error="reference entry does not match direction metadata")
    reference.write_text("MODE_DATA 1 100 0 0 0\nMODE_DATA 3 0 0 0 0\n")
    rejected("second_lifting_cert", *containment_args,
             expected_error="certified entry misses reference ball")
    reference.write_text("MODE_DATA 1 0 0 0 0\nMODE_DATA 3 100 0 0 0\n")
    rejected("second_lifting_cert", *containment_args,
             expected_error="certified conjugate entry misses reference ball")

print("verified rejection of truncated/nonfinite inputs, wrong fan incidence, "
      "mismatched directions, and failed direct/conjugate containment")
