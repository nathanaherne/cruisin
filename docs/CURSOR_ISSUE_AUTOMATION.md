# GitHub issue to Cursor automation

`.github/workflows/cursor_issues.yml` is the only workflow that starts a Cursor cloud agent for a GitHub issue. It dispatches `scripts/dispatch_cursor_issue.py`. A second workflow must not be added for that same `issues` event.

The agent opens a pull request from a new branch. The dispatcher does not merge and does not deploy. After `Validate` succeeds, `.github/workflows/cursor_automerge.yml` runs from the default branch and can merge a low-risk pull request. Whouno keeps its own engineering workflow.

## When it runs

| Trigger | Result |
| --- | --- |
| Issue opened or reopened | One dispatch, unless `no-cursor` is present |
| Label `cursor-ready` added | Dispatch again after a completed, failed, or blocked run |
| Actions → Cursor issue implementation → Run workflow | Dispatch that issue number |
| Any other label, an edit, a pull request, or `github-actions[bot]` | No dispatch |
| Open pull request from `cursor/issue-<number>-…` in this repository | No second agent. Close that pull request before `cursor-ready` or a rerun starts another one |

A second event waits while the latest status is `queued` for 20 minutes or `running` for 360 minutes. Re-running the workflow is an explicit retry. Status comments are `queued`, `running`, `blocked`, `failed`, and `completed`. `running` links the Cursor agent. `completed` links the pull request.

## Repositories

This automation belongs on:

- `nathanaherne/techaus_com_au`
- `nathanaherne/artofgaslighting_com`
- `nathanaherne/cruisin` (`master`)
- `nathanaherne/nathanaherne`
- `nathanaherne/kids-electric-car`

`nathanaherne/nathanaherne_com` is a separate private repository. It does not get this workflow.

## Remaining secret

Whouno already has Actions secret `CURSOR_API_KEY`. GitHub does not let another repository read that value. A personal account has no account-level Actions secret, so each repository above needs its own copy. Create a user or team service-account key at [Cursor Dashboard → Integrations](https://cursor.com/dashboard/integrations). Team Admin API keys do not work.

```bash
gh secret set CURSOR_API_KEY --repo nathanaherne/techaus_com_au
gh secret set CURSOR_API_KEY --repo nathanaherne/artofgaslighting_com
gh secret set CURSOR_API_KEY --repo nathanaherne/cruisin
gh secret set CURSOR_API_KEY --repo nathanaherne/nathanaherne
gh secret set CURSOR_API_KEY --repo nathanaherne/kids-electric-car
```

`gh` prompts for the value and does not print it. Without the secret, the workflow comments `blocked` on the issue.

The same Integrations page is where the Cursor GitHub app is granted access to a repository. If the app cannot open pull requests, the issue comment shows the Cursor API error. Approve repository access there. The issue workflow's own token can only read the repo and write issue comments.

## Safe auto-merge

`Validate` (`.github/workflows/validate.yml`) runs the dispatcher tests and the auto-merge tests on every pull request. Its job name is `validate`. A missing or skipped `validate` check is not success.

`Cursor safe auto-merge` starts from `workflow_run` after `Validate` completes on a pull request, or from Actions → Cursor safe auto-merge → Run workflow with a pull request number. The run uses the workflow on `master`. It does not check out the pull request head and it does not call the Cursor API.

It merges only when all of these are true:

- The head branch matches `cursor/issue-<number>-…`, the pull request body links that issue with `Closes`/`Fixes`/`Resolves` and links exactly one `https://cursor.com/agents/bc-…` agent, and the author is `nathanaherne`.
- A `github-actions[bot]` comment on that issue has a `cursor-issue-agent:v1` marker for the same agent and issue. A completed marker must name this pull request.
- The head repository is `nathanaherne/cruisin`, the base is `master`, and the issue is not labelled `no-cursor`.
- The newest `validate` check on that head SHA was completed by GitHub Actions with `success`. An older success does not cover a newer failure. Every other newest check on that SHA succeeded. `validate` must come from GitHub Actions. Any other required check must come from a GitHub App and succeed. Required contexts from the branch summary and from repository rulesets are included. If that summary or a ruleset body cannot be read, the pull request is not merged. The dedicated branch-protection endpoint is not used; this token receives 403 there.
- The branch is ahead of or identical to `master`, the pull request is mergeable, and the latest review from each reviewer is not `CHANGES_REQUESTED`.
- Every changed path is a low-risk docs or README change. Firmware (`cruisin-brain`, `cruisin-oled`), workflows, scripts, tests, dependencies, secrets, and anything unrecognised need a human. The gate asks `nathanaherne` for a review and comments on the issue.

A second run for the same head SHA does not merge again after `deployed` or `merged`, and it does not repeat the same status. Before merging it posts `evaluating`. Another run leaves that head alone while `evaluating` is the newest marker and is less than 15 minutes old. A newer terminal marker, including `deploy-blocked`, lets a later run verify the deployment again. The Pages decision uses the newest `github-pages` deployment for the merge SHA, not an older success.

## Deploy

GitHub Pages publishes `master` `/docs` to <https://nathanaherne.github.io/cruisin/> with the legacy Jekyll build. `docs/CURSOR_ISSUE_AUTOMATION.md` is <https://nathanaherne.github.io/cruisin/CURSOR_ISSUE_AUTOMATION.html>. The site root returns 404 until `docs` contains an index file. That 404 is not the smoke check.

GitHub does not start a Pages build for a commit created with the workflow `GITHUB_TOKEN`. There is no other deploy credential. When Actions secret `CRUISIN_PAGES_PUSH_TOKEN` is set, the merge API call uses it so the existing Pages build runs. The token has to be a fine-grained personal access token for `nathanaherne` with contents read and write on `nathanaherne/cruisin` only, from an account with a verified email. Do not grant any other permission and do not put the value in an issue or pull request.

Without that secret, a docs change is still merged with `GITHUB_TOKEN` when the gates pass, then the issue comment status is `deploy-blocked` / `pages_not_triggered`. The comment includes the merge SHA and does not claim the site changed. A README-only change does not touch the Pages source; the comment says so and records the current automation page status.

After a Pages deployment for the merge SHA succeeds, the workflow requests each changed docs URL and requires the expected status. A text page must contain a sentence from the file.

## Bootstrap

This gate cannot merge its own pull request. `workflow_run` is registered only once the workflow file is on `master`, and a pull request that edits `.github/workflows` or `scripts/` is `needs-human`. Merging the implementation pull request, after `validate` is green, is the activation. Later low-risk Cursor pull requests do not need a manual merge.

The Cursor GitHub app token used to open that pull request cannot write branch protection or rulesets (`POST /repos/nathanaherne/cruisin/rulesets` returns 403, `administration` is required). `master` currently has no protection and no ruleset. Until the owner adds one, GitHub itself will not enforce `validate`; this workflow still refuses a missing check. The ruleset to add, with no bypass actors, is a pull request rule with zero required approvals plus a strict required status check `validate` from GitHub Actions (app id `15368`), and no force-push or deletion. Zero approvals is deliberate: a required approving review cannot be satisfied by `GITHUB_TOKEN`, and that would stop the authorised auto-merge.

## Pull requests already open

- Cruisin pull request #2 (`cursor/issue-1-c8db`, agent `bc-4b8d14e4-c8eb-4ce0-a6c4-1bca159d79b6`) only adds a sentence to this document. It is the kind of change the gate can merge, and it is not merged here. Its head `76b9215a639f7a2f3535909de61e53d975e8d897` has no check runs. After this workflow is on `master`, run `Validate` for that pull request, then run `Cursor safe auto-merge` with pull request number `2`. Do not merge `master` into that branch first: the files API must still show only the docs change.
- TecHaus pull request #3 is in `nathanaherne/techaus_com_au`. This repository does not merge or edit it.

