#!/usr/bin/env python3
"""Gates for merging a Cursor pull request and verifying GitHub Pages."""

from __future__ import annotations

import sys
import unittest
from datetime import datetime, timedelta, timezone
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))

import cursor_automerge as gate  # noqa: E402


NOW = datetime(2026, 11, 17, 8, 0, tzinfo=timezone.utc)
AGENT = "bc-4b8d14e4-c8eb-4ce0-a6c4-1bca159d79b6"
SHA = "76b9215a639f7a2f3535909de61e53d975e8d897"


def pull(**overrides):
    body = (
        "Docs fix.\n\nCloses #1\n\n"
        f"https://cursor.com/agents/{AGENT}\n"
    )
    value = {
        "number": 2,
        "draft": False,
        "merged": False,
        "state": "open",
        "head_ref": "cursor/issue-1-c8db",
        "head_sha": SHA,
        "head_repo": "nathanaherne/cruisin",
        "base_ref": "master",
        "base_repo": "nathanaherne/cruisin",
        "default_branch": "master",
        "body": body,
        "html_url": "https://github.com/nathanaherne/cruisin/pull/2",
        "user_login": "nathanaherne",
        "issue_labels": [],
    }
    value.update(overrides)
    return value


def agent_comment(status="completed", body_extra=""):
    return {
        "user": "github-actions[bot]",
        "created_at": "2026-11-17T05:49:21Z",
        "body": (
            f"<!-- cursor-issue-agent:v1 issue=1 run=10 status={status} agent={AGENT} "
            f"pr=2 branch=cursor/issue-1-c8db -->\n"
            f"- **Pull request:** https://github.com/nathanaherne/cruisin/pull/2\n"
            "- **Branch:** `cursor/issue-1-c8db`\n"
            + body_extra
        ),
    }


def check(
    name="validate",
    conclusion="success",
    status="completed",
    sha=SHA,
    app="github-actions",
    started_at="",
    completed_at="",
):
    return {
        "name": name,
        "status": status,
        "conclusion": conclusion,
        "head_sha": sha,
        "app_slug": app,
        "started_at": started_at,
        "completed_at": completed_at,
    }


def doc(path="docs/CURSOR_ISSUE_AUTOMATION.md", patch="+This test reached a pull request.\n", content=""):
    return {
        "filename": path,
        "status": "modified",
        "previous_filename": "",
        "patch": patch,
        "content": content or "GitHub issue to Cursor automation\n\nThis test reached a pull request.\n",
    }


def context(**overrides):
    value = {
        "pr": pull(),
        "comments": [agent_comment()],
        "files": [doc()],
        "check_runs": [check()],
        "protection_contexts": [],
        "reviews": [],
        "comparison_status": "ahead",
        "mergeable_state": "clean",
        "markers": [],
        "run_id": "99",
        "now": NOW,
    }
    value.update(overrides)
    return value


class RiskTest(unittest.TestCase):
    def test_docs_and_readme_are_low_risk(self):
        self.assertEqual(gate.classify_path("docs/CURSOR_ISSUE_AUTOMATION.md"), "low")
        self.assertEqual(gate.classify_path("docs/index.html"), "low")
        self.assertEqual(gate.classify_path("docs/site.css"), "low")
        self.assertEqual(gate.classify_path("docs/pic.png"), "low")
        self.assertEqual(gate.classify_path("README.md"), "low")
        self.assertEqual(gate.classify_path("cruisin-brain/README.md"), "low")
        self.assertEqual(gate.risk_reasons([doc()]), [])

    def test_firmware_workflow_script_and_dependency_are_risky(self):
        self.assertEqual(gate.classify_path("cruisin-brain/cruisin-brain.ino"), "vehicle_control")
        self.assertEqual(gate.classify_path("cruisin-oled/pins.h"), "vehicle_control")
        self.assertEqual(gate.classify_path(".github/workflows/cursor_automerge.yml"), "infrastructure")
        self.assertEqual(gate.classify_path("scripts/cursor_automerge.py"), "automation")
        self.assertEqual(gate.classify_path("tests/cursor_automerge_test.py"), "automation")
        self.assertEqual(gate.classify_path("package.json"), "dependency")
        self.assertEqual(gate.classify_path("docs/app.js"), "unsupported_file")

    def test_unknown_or_empty_change_fails_closed(self):
        self.assertEqual(gate.classify_path("archive.bin"), "unknown_path")
        self.assertEqual(gate.classify_path("docs/../scripts/x.py"), "unknown_path")
        self.assertEqual(gate.risk_reasons([]), ["unknown_path"])

    def test_a_text_change_with_no_patch_fails_closed_and_an_image_does_not(self):
        hidden = {"filename": "docs/notes.md", "status": "modified", "patch": "", "changes": 40}
        image = {"filename": "docs/pic.png", "status": "modified", "patch": "", "changes": 1}
        self.assertTrue(any(item.startswith("patch_unavailable:") for item in gate.risk_reasons([hidden])))
        self.assertEqual(gate.risk_reasons([image]), [])

    def test_secret_added_to_a_doc_is_risky_and_a_removed_secret_is_not(self):
        leaked = doc(patch="+token ghp_abcdefghijklmnopqrstuvwxyz0123456789\n")
        removed = doc(patch="-token ghp_abcdefghijklmnopqrstuvwxyz0123456789\n")
        self.assertTrue(gate.risk_reasons([leaked]))
        self.assertEqual(gate.risk_reasons([removed]), [])

    def test_rename_into_firmware_is_risky(self):
        reasons = gate.risk_reasons(
            [
                {
                    "filename": "cruisin-brain/notes.md",
                    "previous_filename": "docs/notes.md",
                    "status": "renamed",
                    "patch": "",
                }
            ]
        )
        self.assertTrue(any(item.startswith("vehicle_control:") for item in reasons))


class OriginTest(unittest.TestCase):
    def test_authorised_cursor_pull_request_passes(self):
        self.assertEqual(gate.origin_problem(pull(), [agent_comment()]), "")

    def test_running_marker_is_enough_before_the_completed_comment(self):
        comment = agent_comment("running")
        comment["body"] = (
            f"<!-- cursor-issue-agent:v1 issue=1 run=10 status=running agent={AGENT} -->\n"
        )
        self.assertEqual(gate.origin_problem(pull(), [comment]), "")

    def test_fork_wrong_branch_missing_marker_and_other_author_are_rejected(self):
        self.assertEqual(
            gate.origin_problem(pull(head_repo="someone/cruisin"), [agent_comment()]),
            "origin_fork",
        )
        self.assertEqual(
            gate.origin_problem(pull(head_ref="feature/docs"), [agent_comment()]),
            "origin_branch",
        )
        self.assertEqual(gate.origin_problem(pull(), []), "origin_agent")
        self.assertEqual(
            gate.origin_problem(pull(user_login="someone"), [agent_comment()]),
            "origin_author",
        )

    def test_completed_marker_for_a_different_pull_request_is_rejected(self):
        comment = agent_comment()
        comment["body"] = comment["body"].replace("/pull/2", "/pull/9").replace("pr=2", "pr=9")
        self.assertEqual(gate.origin_problem(pull(), [comment]), "origin_pr_mismatch")

    def test_no_cursor_label_and_issue_mismatch_are_rejected(self):
        self.assertEqual(
            gate.origin_problem(pull(issue_labels=["no-cursor"]), [agent_comment()]),
            "origin_label",
        )
        self.assertEqual(
            gate.origin_problem(pull(head_ref="cursor/issue-8-abcd"), [agent_comment()]),
            "origin_issue",
        )

    def test_only_the_latest_agent_comment_has_to_name_this_pull_request(self):
        older = agent_comment()
        older["created_at"] = "2026-11-17T01:00:00Z"
        older["body"] = older["body"].replace("/pull/2", "/pull/9").replace("pr=2", "pr=9")
        newer = agent_comment()
        newer["created_at"] = "2026-11-17T05:00:00Z"
        self.assertEqual(gate.origin_problem(pull(), [older, newer]), "")
        newer["body"] = newer["body"].replace("/pull/2", "/pull/9").replace("pr=2", "pr=9")
        self.assertEqual(gate.origin_problem(pull(), [older, newer]), "origin_pr_mismatch")

    def test_a_human_forged_marker_does_not_count(self):
        forged = agent_comment()
        forged["user"] = "nathanaherne"
        self.assertEqual(gate.origin_problem(pull(), [forged]), "origin_agent")


class CheckReviewAndCurrencyTest(unittest.TestCase):
    def test_missing_failed_pending_and_foreign_checks_do_not_pass(self):
        self.assertEqual(gate.checks_problem([], SHA, []), "missing_check:validate")
        self.assertEqual(
            gate.checks_problem([check(conclusion="failure")], SHA, []),
            "failed_check:validate",
        )
        self.assertEqual(
            gate.checks_problem([check(status="in_progress", conclusion="")], SHA, []),
            "pending_check:validate",
        )
        self.assertEqual(
            gate.checks_problem([check(app="other-app")], SHA, []),
            "unattributed_check:validate",
        )
        self.assertEqual(gate.checks_problem([check()], SHA, None), "protection_unreadable")

    def test_a_newer_failure_is_not_hidden_by_an_older_success(self):
        older = check(conclusion="success", started_at="2026-11-17T00:00:00Z")
        newer = check(conclusion="failure", started_at="2026-11-17T01:00:00Z")
        recovered = check(conclusion="success", started_at="2026-11-17T02:00:00Z")
        self.assertEqual(gate.checks_problem([older, newer], SHA, []), "failed_check:validate")
        self.assertEqual(gate.checks_problem([newer, older], SHA, []), "failed_check:validate")
        self.assertEqual(gate.checks_problem([older, newer, recovered], SHA, []), "")
        self.assertEqual(
            gate.checks_problem([check(conclusion="failure"), check(conclusion="success")], SHA, []),
            "failed_check:validate",
        )

    def test_a_required_check_from_another_app_can_pass_when_it_succeeded(self):
        runs = [check(), check(name="security", app="github-code-scanning")]
        self.assertEqual(gate.checks_problem(runs, SHA, ["security"]), "")
        missing_app = [check(), check(name="security", app="")]
        self.assertEqual(gate.checks_problem(missing_app, SHA, ["security"]), "unattributed_check:security")

    def test_a_failed_extra_check_blocks_and_success_on_another_sha_does_not_count(self):
        runs = [check(), check(name="lint", conclusion="failure")]
        self.assertEqual(gate.checks_problem(runs, SHA, []), "failed_check:lint")
        self.assertEqual(
            gate.checks_problem([check(sha="0" * 40)], SHA, []),
            "missing_check:validate",
        )

    def test_unprotected_branch_adds_no_contexts_and_a_ruleset_check_is_required(self):
        self.assertEqual(
            gate.contexts_from_branch({"name": "master", "protected": False, "protection": {"required_status_checks": {"contexts": []}}}),
            [],
        )
        self.assertIsNone(gate.contexts_from_branch({"protected": True, "protection": {}}))
        ruleset = {
            "rules": [
                {
                    "type": "required_status_checks",
                    "parameters": {"required_status_checks": [{"context": "security"}]},
                }
            ]
        }
        self.assertEqual(gate.contexts_from_ruleset(ruleset), ["security"])
        self.assertTrue(
            gate.ruleset_applies(
                {"conditions": {"ref_name": {"include": ["~DEFAULT_BRANCH"], "exclude": []}}},
                "master",
                True,
            )
        )
        self.assertFalse(
            gate.ruleset_applies(
                {"conditions": {"ref_name": {"include": ["refs/heads/release"], "exclude": []}}},
                "master",
                True,
            )
        )

    def test_protection_context_is_required_as_well_as_validate(self):
        self.assertEqual(
            gate.checks_problem([check()], SHA, ["security"]),
            "missing_check:security",
        )
        runs = [check(), check(name="security")]
        self.assertEqual(gate.checks_problem(runs, SHA, ["security"]), "")

    def test_changes_requested_blocks_and_a_later_dismissal_does_not(self):
        self.assertEqual(gate.review_problem(None), "reviews_unreadable")
        self.assertEqual(
            gate.review_problem(
                [{"user": "nathanaherne", "state": "CHANGES_REQUESTED", "submitted_at": "2026-11-17T00:00:00Z"}]
            ),
            "changes_requested",
        )
        self.assertEqual(
            gate.review_problem(
                [
                    {"user": "nathanaherne", "state": "CHANGES_REQUESTED", "submitted_at": "2026-11-17T00:00:00Z"},
                    {"user": "nathanaherne", "state": "DISMISSED", "submitted_at": "2026-11-17T01:00:00Z"},
                ]
            ),
            "",
        )

    def test_behind_diverged_and_conflict_do_not_merge(self):
        self.assertEqual(gate.currency_problem("behind", "clean"), "not_current")
        self.assertEqual(gate.currency_problem("diverged", "clean"), "not_current")
        self.assertEqual(gate.currency_problem("ahead", "dirty"), "conflict")
        self.assertEqual(gate.currency_problem("ahead", "unknown"), "mergeable_unknown")
        self.assertEqual(gate.currency_problem("ahead", "clean"), "")


class DecisionTest(unittest.TestCase):
    def test_low_risk_authorised_pr_with_green_checks_is_ready(self):
        decision = gate.decide(context())
        self.assertTrue(decision["merge"])
        self.assertEqual(decision["status"], "ready")
        self.assertTrue(decision["docs_changed"])
        self.assertTrue(decision["comment_on_issue"])

    def test_failed_check_untrusted_pr_and_risky_change_do_not_merge(self):
        failed = gate.decide(context(check_runs=[check(conclusion="failure")]))
        self.assertFalse(failed["merge"])
        self.assertEqual(failed["status"], "blocked-checks")
        untrusted = gate.decide(context(pr=pull(head_repo="other/cruisin")))
        self.assertEqual(untrusted["status"], "blocked-untrusted")
        self.assertFalse(untrusted["comment_on_issue"])
        risky = gate.decide(context(files=[{"filename": "cruisin-brain/lights.ino", "status": "modified", "patch": ""}]))
        self.assertEqual(risky["status"], "needs-human")
        self.assertTrue(risky["notify_human"])
        self.assertFalse(risky["merge"])

    def test_missing_ci_and_duplicate_evaluating_marker_do_not_merge(self):
        missing = gate.decide(context(check_runs=[]))
        self.assertEqual(missing["reason"], "missing_check:validate")
        self.assertFalse(missing["merge"])
        marker = {
            "pr": "2",
            "sha": SHA,
            "status": "evaluating",
            "run": "50",
            "created_at": (NOW - timedelta(minutes=2)).isoformat(),
        }
        duplicate = gate.decide(context(markers=[marker]))
        self.assertEqual(duplicate["status"], "skip")
        self.assertEqual(duplicate["reason"], "in_flight")

    def test_same_outcome_is_not_posted_twice_and_a_deployed_marker_stops(self):
        markers = [{"pr": "2", "sha": SHA, "status": "blocked-checks", "reason": "missing_check:validate"}]
        self.assertTrue(gate.already_posted(markers, 2, SHA, "blocked-checks", "missing_check:validate"))
        self.assertFalse(gate.already_posted(markers, 2, SHA, "ready", "gates_passed"))
        deployed = [{"pr": "2", "sha": SHA, "status": "deployed", "reason": "smoke_passed"}]
        self.assertTrue(gate.already_posted(deployed, 2, SHA, "deployed", "smoke_passed"))
        self.assertTrue(gate.already_posted(deployed, 2, SHA, "verify-deploy", "already_merged"))

    def test_a_newer_terminal_marker_releases_the_evaluating_lock(self):
        evaluating = {
            "pr": "2",
            "sha": SHA,
            "status": "evaluating",
            "run": "50",
            "created_at": (NOW - timedelta(minutes=2)).isoformat(),
        }
        done = {
            "pr": "2",
            "sha": SHA,
            "status": "deploy-blocked",
            "reason": "pages_not_triggered",
            "run": "50",
            "created_at": (NOW - timedelta(minutes=1)).isoformat(),
        }
        decision = gate.decide(context(markers=[evaluating, done]))
        self.assertNotEqual(decision["reason"], "in_flight")
        self.assertEqual(decision["status"], "ready")

    def test_an_already_merged_pull_request_is_not_merged_again(self):
        decision = gate.decide(context(pr=pull(merged=True)))
        self.assertFalse(decision["merge"])
        self.assertEqual(decision["status"], "verify-deploy")

    def test_a_behind_branch_requests_an_update_instead_of_merging(self):
        decision = gate.decide(context(comparison_status="behind"))
        self.assertFalse(decision["merge"])
        self.assertTrue(decision["update_branch"])
        unknown = gate.decide(context(comparison_status=""))
        self.assertFalse(unknown["merge"])
        self.assertFalse(unknown["update_branch"])

    def test_changes_requested_does_not_merge(self):
        decision = gate.decide(
            context(
                reviews=[{"user": "nathanaherne", "state": "CHANGES_REQUESTED", "submitted_at": "2026-11-17T00:00:00Z"}]
            )
        )
        self.assertEqual(decision["status"], "blocked-reviews")
        self.assertFalse(decision["merge"])


class DeployTest(unittest.TestCase):
    def test_pages_urls_follow_the_docs_publishing_root(self):
        self.assertEqual(
            gate.pages_url("docs/CURSOR_ISSUE_AUTOMATION.md"),
            "https://nathanaherne.github.io/cruisin/CURSOR_ISSUE_AUTOMATION.html",
        )
        self.assertEqual(gate.pages_url("docs/index.md"), "https://nathanaherne.github.io/cruisin/")
        self.assertEqual(
            gate.pages_url("docs/guide/setup.md"),
            "https://nathanaherne.github.io/cruisin/guide/setup.html",
        )

    def test_readme_only_change_does_not_require_a_pages_rebuild(self):
        result = gate.deployment_result(False, [], [], "github_token")
        self.assertEqual(result["outcome"], "merged")
        self.assertEqual(result["reason"], "pages_source_unchanged")

    def test_github_token_merge_without_a_pages_deployment_is_blocked(self):
        result = gate.deployment_result(True, [], [], "github_token")
        self.assertEqual(result["outcome"], "deploy-blocked")
        self.assertEqual(result["reason"], "pages_not_triggered")

    def test_missing_deployment_ids_keep_the_first_listing(self):
        chosen = gate.select_deployment(
            [
                {"environment": "github-pages", "state": "failure", "id": 0},
                {"environment": "github-pages", "state": "success", "id": 0},
            ]
        )
        self.assertEqual(chosen["state"], "failure")

    def test_a_newer_failed_deployment_beats_an_older_success(self):
        result = gate.deployment_result(
            True,
            [
                {"environment": "github-pages", "state": "success", "id": 1, "sha": SHA},
                {"environment": "github-pages", "state": "failure", "id": 2, "sha": SHA},
            ],
            [
                {
                    "status_code": 200,
                    "expect": 200,
                    "snippet": "old page",
                    "snippet_required": True,
                    "body": "old page",
                    "error": "",
                }
            ],
            "pages_push_token",
        )
        self.assertEqual(result["outcome"], "deploy-failed")
        self.assertEqual(result["reason"], "pages_deployment_failed")

    def test_failed_deployment_and_failed_smoke_check(self):
        failed = gate.deployment_result(
            True,
            [{"environment": "github-pages", "state": "failure", "sha": SHA}],
            [],
            "pages_push_token",
        )
        self.assertEqual(failed["outcome"], "deploy-failed")
        smoke = gate.deployment_result(
            True,
            [{"environment": "github-pages", "state": "success", "sha": SHA}],
            [
                {
                    "url": "https://nathanaherne.github.io/cruisin/CURSOR_ISSUE_AUTOMATION.html",
                    "status_code": 404,
                    "expect": 200,
                    "snippet": "This test reached a pull request.",
                    "snippet_required": True,
                    "body": "missing",
                    "error": "",
                }
            ],
            "pages_push_token",
        )
        self.assertEqual(smoke["reason"], "smoke_failed")

    def test_successful_deployment_needs_the_published_sentence(self):
        page = "This test reached a pull request."
        result = gate.deployment_result(
            True,
            [{"environment": "github-pages", "state": "success", "sha": SHA, "target_url": "https://github.com/nathanaherne/cruisin/actions/runs/1"}],
            [
                {
                    "url": "https://nathanaherne.github.io/cruisin/CURSOR_ISSUE_AUTOMATION.html",
                    "status_code": 200,
                    "expect": 200,
                    "snippet": page,
                    "snippet_required": True,
                    "body": f"<html>{page}</html>",
                    "error": "",
                }
            ],
            "pages_push_token",
        )
        self.assertEqual(result, {"outcome": "deployed", "reason": "smoke_passed"})

    def test_comment_records_the_merge_and_redacts_secrets(self):
        body = gate.render_comment(
            pr=2,
            sha=SHA,
            status="deploy-blocked",
            reason="pages_not_triggered",
            repo="nathanaherne/cruisin",
            run_id="99",
            pr_url="https://github.com/nathanaherne/cruisin/pull/2",
            merge_sha="abc123",
            detail="token ghp_abcdefghijklmnopqrstuvwxyz0123456789",
        )
        parsed = gate.parse_automerge_marker(body)
        self.assertEqual(parsed["status"], "deploy-blocked")
        self.assertEqual(parsed["sha"], SHA)
        self.assertIn("Merge SHA", body)
        self.assertNotIn("ghp_", body)


class WorkflowShapeTest(unittest.TestCase):
    def triggers(self, name: str) -> str:
        text = (Path(__file__).resolve().parents[1] / ".github" / "workflows" / name).read_text()
        return text.split("permissions:", 1)[0]

    def test_issue_dispatch_is_not_given_a_second_issue_workflow(self):
        issues = self.triggers("cursor_issues.yml")
        merge = self.triggers("cursor_automerge.yml")
        validate = self.triggers("validate.yml")
        self.assertIn("issues:", issues)
        self.assertNotIn("workflow_run:", issues)
        self.assertNotIn("issues:", merge)
        self.assertIn("workflow_run:", merge)
        self.assertIn("pull_request:", validate)
        self.assertNotIn("issues:", validate)

    def test_automerge_does_not_call_cursor_or_take_administration(self):
        root = Path(__file__).resolve().parents[1]
        source = (root / "scripts" / "cursor_automerge.py").read_text()
        workflow = (root / ".github" / "workflows" / "cursor_automerge.yml").read_text()
        self.assertNotIn("api.cursor.com", source)
        self.assertNotIn("administration:", workflow)
        self.assertNotIn("pull_request_target:", workflow)
        self.assertIn("persist-credentials: false", workflow)
        issues = (root / ".github" / "workflows" / "cursor_issues.yml").read_text()
        self.assertIn("pull-requests: read", issues)
        self.assertNotIn("contents: write", issues)

    def test_merge_methods_prefer_squash_without_dropping_the_others(self):
        self.assertEqual(gate.merge_methods(True, True, False), ["squash", "merge"])
        self.assertEqual(gate.merge_methods(False, False, False), ["merge"])


if __name__ == "__main__":
    unittest.main()
