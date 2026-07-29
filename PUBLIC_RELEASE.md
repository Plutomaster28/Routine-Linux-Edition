# Public release preparation

The working tree is prepared to become an independent repository. No remote is
configured automatically.

## Before the first public push

1. Run:

   ```bash
   ./build_linux.sh
   ./release_audit.sh
   ```

2. Review `git status` and the complete staged diff.
3. Confirm `config.json` and `data/` remain ignored.
4. Create the empty GitHub repository without adding a generated README,
   license, or `.gitignore`.
5. Decide whether to preserve ancestry or start independent history.

## Independent history

The safest clean break is to copy the audited working tree, excluding `.git`
and ignored files, into a new directory and initialize Git there:

```bash
mkdir ../routine-linux-public
while IFS= read -r -d '' path; do
  [[ -e "${path}" ]] && printf '%s\0' "${path}"
done < <(git ls-files --cached --others --exclude-standard -z) |
  tar --null -T - -cf - |
  tar -C ../routine-linux-public -xf -
cd ../routine-linux-public
git init
git add .
git commit -m "Initial public release of Routine for Linux"
```

Then add the new remote and push from that new directory. This preserves the
current private checkout as a recovery point and guarantees that the new
repository does not inherit old remote metadata or historical objects.

Do not run those commands until the final diff is committed or otherwise
present in the working tree you are exporting.

## Release identity

Before publishing, choose the final GitHub owner/repository name and add:

- the private vulnerability-reporting contact in `SECURITY.md`;
- repository topics and description;
- branch protection and required Linux CI;
- a version tag after a clean CI run.
