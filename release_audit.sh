#!/usr/bin/env bash

set -euo pipefail

ROUTINE_AUDIT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${ROUTINE_AUDIT_ROOT}"

failed=0

check_ignored() {
    local target="$1"
    if ! git check-ignore -q "${target}"; then
        echo "FAIL: ${target} is not ignored"
        failed=1
    fi
}

check_ignored "config.json"
check_ignored "data/local_economy_v1.db"
check_ignored "logs/routine.log"
check_ignored "build-linux/kernel/routine"
check_ignored "modules/private_module.so"

mapfile -d '' public_files < <(
    git ls-files --cached --others --exclude-standard -z
)

ROUTINE_AUDIT_TEMP="$(mktemp -d)"
trap 'rm -rf "${ROUTINE_AUDIT_TEMP}"' EXIT

for forbidden in config.json bot_token.txt; do
    for path in "${public_files[@]}"; do
        if [[ "${path}" == "${forbidden}" ]]; then
            echo "FAIL: forbidden runtime file would be published: ${path}"
            failed=1
        fi
    done
done

if (( ${#public_files[@]} > 0 )); then
    if printf '%s\0' "${public_files[@]}" |
       xargs -0 rg -l \
         '(^|[^A-Za-z0-9_-])[A-Za-z0-9_-]{20,30}\.[A-Za-z0-9_-]{5,10}\.[A-Za-z0-9_-]{20,}' \
         >"${ROUTINE_AUDIT_TEMP}/token_hits.txt" 2>/dev/null; then
        echo "FAIL: possible Discord token found in:"
        sed 's/^/  /' "${ROUTINE_AUDIT_TEMP}/token_hits.txt"
        failed=1
    fi
    if printf '%s\0' "${public_files[@]}" |
       xargs -0 rg -l 'BEGIN ([A-Z ]+ )?PRIVATE KEY' \
         >"${ROUTINE_AUDIT_TEMP}/key_hits.txt" 2>/dev/null; then
        echo "FAIL: private key material found in:"
        sed 's/^/  /' "${ROUTINE_AUDIT_TEMP}/key_hits.txt"
        failed=1
    fi
fi

if git ls-files | rg -q \
   '(^|/)(config\.json|data/.*|logs/.*|.*\.(so|dll|dylib|exe))$'; then
    echo "FAIL: tracked runtime artifact detected"
    failed=1
fi

if ! git diff --check; then
    failed=1
fi

if (( failed != 0 )); then
    echo "Public release audit failed."
    exit 1
fi

echo "Public release audit passed."
