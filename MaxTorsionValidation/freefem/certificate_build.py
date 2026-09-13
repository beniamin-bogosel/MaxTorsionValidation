"""Build private verifiers from a sweep's archived source snapshots."""
import hashlib
from pathlib import Path
import shutil


def build_verifiers(archive, run):
    """Return private executable paths and their recorded build identity.

    A fresh directory and forced build prevent a shared, stale executable
    from being mistaken for a binary compiled from the archived sources.
    """
    archive = Path(archive)
    build = archive / 'build'
    build.mkdir(exist_ok=False)
    names = ('second_lifting_cert', 'mode_cert')
    for filename in ('Makefile', *(name + '.c' for name in names)):
        shutil.copy2(archive / 'sources' / filename, build / filename)
    command = ['make', '-B', '-C', build, *names]
    run(command, archive / 'build.log')

    # Compile this diagnostic with the same compiler, flags, headers and
    # libraries. Its result is metadata, never part of a sign certificate.
    (build / 'build_identity.c').write_text('''#include <stdio.h>
#include <flint/flint.h>
int main(void) {
#ifdef __VERSION__
    printf("compiler=%s\\n", __VERSION__);
#else
    puts("compiler=unavailable");
#endif
    printf("flint_headers=%s\\nflint_runtime=%s\\n", FLINT_VERSION, flint_version);
    return 0;
}
''')
    (build / 'identity.mk').write_text('''include Makefile
build_identity: build_identity.c
\t$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)
''')
    identity_log = archive / 'build_identity.log'
    try:
        run(['make', '-B', '-C', build, '-f', 'identity.mk', 'build_identity'],
            archive / 'build_identity_compile.log')
        run([build / 'build_identity'], identity_log)
        identity = identity_log.read_text().strip()
    except RuntimeError as error:
        # Verifier compilation already succeeded; unavailable diagnostic
        # metadata must not change the mathematics or stop a valid run.
        identity = f'unavailable: {error}'
    executables = {name: build / name for name in names}
    metadata = dict(command=[str(x) for x in command], identity=identity,
                    executables={name: dict(path=str(path.relative_to(archive)),
                        sha256=hashlib.sha256(path.read_bytes()).hexdigest())
                        for name, path in executables.items()})
    return executables, metadata
