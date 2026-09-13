# Preparing the GitHub source repository

The source distribution contains the root documentation and prompt history,
the original supplied programs in `Code/`, and maintained software in
`MaxTorsionValidation/`. Its existing `freefem/`, `flint/`, and `symbolic/`
subdirectories separate candidate generation, interval verification, and
exact algebraic checks.

The root `.gitignore` excludes the local manuscript (`theory/` and `Theory/`),
`Unused/`, reference PDFs in `Papers/`, local assistant metadata, Python
caches, compiled verifiers, and large generated result directories. Original
programs and small regression fixtures remain eligible for Git. The two
compact result summaries and the archive guide are included.

In an actual Git checkout, review and stage the intended source files with:

```sh
git add .gitignore README.md REVIEW.md PROVENANCE.md PromptHistory.md \
  Code MaxTorsionValidation
git diff --cached --stat
git status --short
```

Do not force-add ignored folders. `.gitignore` does not remove files already
tracked by an existing repository; check the staged list if publishing from
a different checkout. No Git initialization, staging, commit, remote setup,
or upload was performed during this packaging task: the workspace exposes
an empty, read-only `.git` placeholder rather than usable Git metadata.

The [Section 8 guide](REPRODUCING_SECTION_8.md) contains commands for future
builds and certificate runs. Packaging verification is limited to static
syntax, path, and exclusion checks; it does not provide a fresh numerical
certificate. The existing mathematical review is in [REVIEW.md](../../REVIEW.md).

For the complete research release, supply the author/citation and license
metadata described in [PROVENANCE.md](../../PROVENANCE.md). Distribute the
manuscript and immutable certificate data separately. The
[archive guide](../results/README.md) identifies the directories, checksum
exception, and replay evidence that belong together. Add a persistent data
URL when a deposit exists; no placeholder URL is presented as a download.
