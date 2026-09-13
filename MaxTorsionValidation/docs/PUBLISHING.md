# Preparing the GitHub source repository

The source distribution contains the root documentation and prompt history,
the three-lecture course in `DetailedExplanations/`, the original supplied
programs in `Code/`, and maintained software in
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
git add .gitignore README.md REVIEW.md PACKAGE_RECHECK.md PROVENANCE.md PromptHistory.md \
  Code DetailedExplanations MaxTorsionValidation
git diff --cached --stat
git status --short
```

Do not force-add ignored folders. `.gitignore` does not remove files already
tracked by an existing repository; check the staged list if publishing from
a different checkout. The course includes its LaTeX sources, bibliography,
generated reference/table inputs, PDFs, and build instructions. Its ordinary
PDF build works without the separately distributed manuscript and archives;
refreshing its source snapshot requires those local research files.

The [Section 8 guide](REPRODUCING_SECTION_8.md) contains commands for future
builds and certificate runs. The latest check scope and results are recorded
in [PACKAGE_RECHECK.md](../../PACKAGE_RECHECK.md); the earlier mathematical
review is in [REVIEW.md](../../REVIEW.md).

For the complete research release, supply the author/citation and license
metadata described in [PROVENANCE.md](../../PROVENANCE.md). Distribute the
manuscript and immutable certificate data separately. The
[archive guide](../results/README.md) identifies the directories, checksum
exception, and replay evidence that belong together. Add a persistent data
URL when a deposit exists; no placeholder URL is presented as a download.
