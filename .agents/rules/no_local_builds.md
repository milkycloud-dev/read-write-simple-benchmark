# Rule: No Local Builds or Tests (GitHub-Only CI)

1. Do NOT download or install compilers, toolchains, or heavy build artifacts locally on the machine (storage is strictly limited).
2. Do NOT run local benchmarks or tests on the machine.
3. All compilation, tests, and artifact verification must be performed exclusively in GitHub Actions.
4. When testing is needed, create temporary private GitHub repositories.
