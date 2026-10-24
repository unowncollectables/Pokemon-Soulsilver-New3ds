# Development guide

This document is meant to keep the project structured as work progresses.

## Development philosophy

This project is intended to be a clean, explainable homebrew porting effort. The focus is on clarity over speed, correctness over shortcuts, and reproducibility over undocumented assumptions.

## Branching and workflow

A simple branching strategy works well here:

- `main` — stable project branch for reviewed changes
- `feature/*` — work in progress for a specific task or feature
- `fix/*` — bug-fix branches
- `docs/*` — documentation and repo cleanup branches

## Contributor expectations

- keep changes small and reviewable
- explain the purpose of the change clearly
- document build or setup changes when relevant
- do not add ROMs, dumps, or copyrighted assets
- test locally when possible

## Suggested local workflow

1. Create a feature or fix branch.
2. Make the change in small increments.
3. Run any appropriate local validation.
4. Update relevant docs if the build or developer workflow changes.
5. Open a clean pull request with a summary and notes.

## Review standards

Good PRs should include:

- a clear summary of what changed
- a reason for the change
- any testing performed
- notes on risks or follow-up work

## Common project principles

- keep naming consistent across documentation and code
- document non-obvious changes
- preserve the legal and safety boundaries of the project
- prefer maintainability over clever but fragile solutions

## Long-term goal

The intent is to build a project that is still experimental, but structured enough to be understandable, contributor-friendly, and easy to extend as development matures.
