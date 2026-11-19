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

GitHub does not start a Pages build for a commit created with the workflow `GITHUB_TOKEN`. When Actions secret `CRUISIN_PAGES_PUSH_TOKEN` is set, the merge API call uses it so the existing Pages build runs. `GET /repos/nathanaherne/cruisin/actions/secrets/CRUISIN_PAGES_PUSH_TOKEN` returns 403 for the Cursor app token, so this document cannot say whether the secret is already stored. The Pages deployment for merge `3edddf0085b8eb4958252306e1fc0caf965aae6c` succeeded because `nathanaherne` merged pull request #5 in the GitHub UI ([deployment 7918785343](https://github.com/nathanaherne/cruisin/actions/runs/44563547970)). That build does not prove the secret exists.

If the secret is absent, the owner action is one credential. Signed in as `nathanaherne`, open the [prefilled fine-grained token form](https://github.com/settings/personal-access-tokens/new?name=CRUISIN_PAGES_PUSH_TOKEN&description=Pages+merge+for+nathanaherne%2Fcruisin&target_name=nathanaherne&contents=write). Set repository access to only `nathanaherne/cruisin`, leave every permission except Contents unset (Contents is Read and write; GitHub also adds Metadata read), and generate the token from an account with a verified email. Then open [New repository secret](https://github.com/nathanaherne/cruisin/settings/secrets/actions/new) and save the value as Actions secret `CRUISIN_PAGES_PUSH_TOKEN`. Do not paste the token into an issue or pull request.

Without that secret, a docs change is still merged with `GITHUB_TOKEN` when the gates pass, then the issue comment status is `deploy-blocked` / `pages_not_triggered`. The comment includes the merge SHA and does not claim the site changed. A README-only change does not touch the Pages source; the comment says so and records the current automation page status.

After a Pages deployment for the merge SHA succeeds, the workflow requests each changed docs URL and requires the expected status. A text page must contain a sentence from the file.

## Bootstrap

`Validate` and `Cursor safe auto-merge` are active on `master`. The push of `3edddf0085b8eb4958252306e1fc0caf965aae6c` ran [Validate](https://github.com/nathanaherne/cruisin/actions/runs/44563549095) and the `validate` job succeeded. The auto-merge workflow then started from `workflow_run` and skipped its merge job because that Validate event was a push, not a pull request ([run 44563591829](https://github.com/nathanaherne/cruisin/actions/runs/44563591829)). A pull request that edits `.github/workflows` or `scripts/` stays `needs-human`. Later low-risk Cursor pull requests do not need a manual merge.

The Cursor GitHub app token used to open that pull request cannot write branch protection or rulesets (`POST /repos/nathanaherne/cruisin/rulesets` returns 403, `administration` is required). `master` currently has no protection and no ruleset. Until the owner adds one, GitHub itself will not enforce `validate`; this workflow still refuses a missing check. The ruleset to add, with no bypass actors, is a pull request rule with zero required approvals plus a strict required status check `validate` from GitHub Actions (app id `15368`), and no force-push or deletion. Zero approvals is deliberate: a required approving review cannot be satisfied by `GITHUB_TOKEN`, and that would stop the authorised auto-merge.

## Pull requests already open

- Cruisin pull request #4 (`cursor/issue-3-0089`) is closed and superseded by merged pull request #5 (`3edddf0085b8eb4958252306e1fc0caf965aae6c`). Its check handling kept the oldest run for a name, and its Pages decision accepted an older successful deployment. Do not merge it.
- Cruisin pull request #2 (`cursor/issue-1-c8db`, head `76b9215a639f7a2f3535909de61e53d975e8d897`, agent `bc-4b8d14e4-c8eb-4ce0-a6c4-1bca159d79b6`) only adds "This test reached a pull request." to this document. That path is low risk: no firmware, workflow, script, secret, or hidden patch. It is not merged. The head has no check runs, so the gate returns `blocked-checks` / `missing_check:validate` and does not update the branch. Compared with `master` the branch is diverged and `mergeable_state` is `dirty`. A green `validate` check would still not merge it; the gate would try to update the branch and stop on the conflict. Do not merge `master` into that branch and do not merge the pull request.
- TecHaus pull request #3 is in `nathanaherne/techaus_com_au`. This repository does not merge or edit it.

