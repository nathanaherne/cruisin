#!/usr/bin/env python3
"""Merge a low-risk Cursor issue pull request and verify GitHub Pages.

The decision functions are pure. The workflow on the default branch is the
only code that runs them. A pull request cannot change this gate and then
use that change to merge itself: workflow_run checks out the default branch,
not the pull request head.

This script does not call the Cursor API and does not open issues.
"""

from __future__ import annotations

import json
import os
import re
import sys
import time
import urllib.error
import urllib.request
from datetime import datetime, timezone

from dispatch_cursor_issue import parse_marker, redact

MARKER_RE = re.compile(r"<!--\s*cursor-automerge:v1\b([^>]*)-->", re.I)
FIELD_RE = re.compile(r"(\w+)=([^\s]+)")
BRANCH_RE = re.compile(r"^cursor/issue-(\d+)-[A-Za-z0-9._-]+$")
AGENT_RE = re.compile(r"https://cursor\.com/agents/(bc-[0-9a-fA-F-]{8,})")
CLOSES_RE = re.compile(r"(?i)\b(?:close[sd]?|fix(?:e[sd])?|resolve[sd]?)\s+#(\d+)")
PR_URL_RE = re.compile(r"https://github\.com/[^/\s]+/[^/\s]+/pull/\d+")
BRANCH_LINE_RE = re.compile(r"\*\*Branch:\*\* `([^`]+)`")
SECRET_RE = re.compile(
    r"(crsr_|cursor_|ghp_|gho_|ghu_|ghs_|ghr_|github_pat_|sk_live_|AKIA[0-9A-Z]{16})[A-Za-z0-9_\-]+"
    r"|-----BEGIN [A-Z ]*PRIVATE KEY-----",
    re.I,
)

REQUIRED_CHECK = "validate"
CHECK_APP = "github-actions"
HARD_STOP_STATUSES = frozenset({"deployed", "merged"})
SAFE_DOC_EXT = frozenset({".md", ".markdown", ".html", ".css", ".txt", ".svg", ".png", ".jpg", ".jpeg", ".gif", ".webp", ".ico"})
TEXT_EXT = frozenset({".md", ".markdown", ".html", ".css", ".txt", ".svg"})
CODE_EXT = frozenset(
    {".ino", ".h", ".hpp", ".c", ".cpp", ".cc", ".s", ".ld", ".py", ".js", ".mjs", ".cjs", ".sh", ".bash", ".yml", ".yaml", ".json", ".toml", ".xml"}
)
DEPENDENCY_NAMES = frozenset(
    {
        "requirements.txt",
        "requirements-dev.txt",
        "package.json",
        "package-lock.json",
        "yarn.lock",
        "pnpm-lock.yaml",
        "pipfile",
        "pipfile.lock",
        "poetry.lock",
        "pyproject.toml",
        "go.mod",
        "go.sum",
        "cargo.toml",
        "cargo.lock",
        "platformio.ini",
        "library.properties",
        "gemfile",
        "gemfile.lock",
    }
)
PAGES_BASE = "https://nathanaherne.github.io/cruisin/"


def _ext(name: str) -> str:
    base = name.rsplit("/", 1)[-1].lower()
    if "." not in base:
        return ""
    return "." + base.rsplit(".", 1)[-1]


def classify_path(path: str) -> str:
    """Return 'low' or a fail-closed reason code for one changed path."""
    norm = (path or "").replace("\\", "/").lstrip("/")
    if not norm or norm.startswith("../") or "/../" in f"/{norm}/" or "\x00" in norm:
        return "unknown_path"
    parts = norm.split("/")
    if any(part in ("", ".", "..") for part in parts):
        return "unknown_path"
    lower = norm.lower()
    name = parts[-1]
    lower_name = name.lower()
    if (
        lower_name.startswith(".env")
        or lower_name.endswith((".pem", ".key"))
        or lower_name in {"id_rsa", "credentials.json", "secrets.json"}
    ):
        return "secrets"
    if lower.startswith(".github/") or lower_name in {"codeowners", "dockerfile"}:
        return "infrastructure"
    if lower.startswith("scripts/") or lower.startswith("tests/"):
        return "automation"
    if lower_name in DEPENDENCY_NAMES or lower_name.endswith(".lock"):
        return "dependency"
    if lower.startswith("cruisin-brain/") or lower.startswith("cruisin-oled/"):
        if lower_name == "readme.md":
            return "low"
        return "vehicle_control"
    ext = _ext(name)
    if ext in CODE_EXT:
        return "vehicle_control" if ext in {".ino", ".h", ".hpp", ".c", ".cpp", ".cc", ".s", ".ld"} else "unsupported_file"
    if lower_name == "readme.md":
        return "low"
    if lower.startswith("docs/") and ext in SAFE_DOC_EXT:
        return "low"
    if "/" not in lower and ext in {".md", ".markdown"}:
        return "low"
    return "unknown_path"


def added_lines(patch: str) -> str:
    lines = []
    for line in (patch or "").splitlines():
        if line.startswith("+++"):
            continue
        if line.startswith("+"):
            lines.append(line[1:])
    return "\n".join(lines)


def secret_in_patch(patch: str) -> bool:
    return SECRET_RE.search(added_lines(patch)) is not None


def patch_hidden(item: dict) -> bool:
    """A text change with no patch can hide a secret or a code edit."""
    status = str(item.get("status") or "")
    if status not in {"added", "modified", "changed"}:
        return False
    if item.get("patch"):
        return False
    if _ext(str(item.get("filename") or "")) in SAFE_DOC_EXT - TEXT_EXT:
        return False
    if item.get("changes") == 0:
        return False
    return True


def risk_reasons(files: list[dict]) -> list[str]:
    if not files:
        return ["unknown_path"]
    reasons = []
    for item in files:
        names = [item.get("filename") or ""]
        previous = item.get("previous_filename") or ""
        if previous:
            names.append(previous)
        for name in names:
            reason = classify_path(name)
            if reason != "low":
                reasons.append(f"{reason}:{name}")
        if patch_hidden(item):
            reasons.append(f"patch_unavailable:{item.get('filename') or ''}")
        if secret_in_patch(item.get("patch") or ""):
            reasons.append(f"secrets:{item.get('filename') or ''}")
    return reasons


def pages_url(path: str, base: str = PAGES_BASE) -> str:
    """Map a docs/ path to the legacy Jekyll Pages URL. docs/ is the site root."""
    rel = path[5:] if path.startswith("docs/") else path
    rel = rel.lstrip("/")
    root = base if base.endswith("/") else base + "/"
    lower = rel.lower()
    if lower in {"index.md", "index.markdown", "index.html"}:
        return root
    if lower.endswith(".md") or lower.endswith(".markdown"):
        rel = rel.rsplit(".", 1)[0] + ".html"
    return root + rel


def snippet_from_text(text: str) -> str:
    for line in (text or "").splitlines():
        cleaned = line.strip().lstrip("#").strip(" \t`*")
        cleaned = re.sub(r"\[([^\]]+)\]\([^)]+\)", r"\1", cleaned)
        if cleaned.startswith("<!--") or cleaned.startswith("```"):
            continue
        if len(cleaned) >= 12:
            return cleaned[:80]
    compact = " ".join((text or "").split())
    return compact[:80]


def publication_targets(files: list[dict], base: str = PAGES_BASE) -> list[dict]:
    targets = []
    for item in files:
        path = item.get("filename") or ""
        if not path.startswith("docs/") or classify_path(path) != "low":
            continue
        ext = _ext(path)
        removed = item.get("status") == "removed"
        snippet = "" if removed or ext not in TEXT_EXT else snippet_from_text(item.get("content") or "")
        targets.append(
            {
                "path": path,
                "url": pages_url(path, base),
                "expect": 404 if removed else 200,
                "snippet": snippet,
                "snippet_required": (not removed) and ext in TEXT_EXT,
            }
        )
    return targets


def _check_rank(run: dict) -> tuple[str, str]:
    """Sort key for a check run. Missing timestamps lose to a stamped run."""
    return (str(run.get("started_at") or ""), str(run.get("completed_at") or ""))


def latest_check_runs(check_runs: list[dict], head_sha: str) -> list[dict]:
    """One run per name. A newer stamp wins; without stamps the first item wins.

    GitHub lists check runs newest first. Keeping the last item would treat an
    older success as current after a rerun failed, and the pull request would merge.
    """
    latest: dict[str, dict] = {}
    for run in check_runs or []:
        if run.get("head_sha") and run.get("head_sha") != head_sha:
            continue
        name = str(run.get("name") or "")
        current = latest.get(name)
        if current is None:
            latest[name] = run
            continue
        if _check_rank(run) > _check_rank(current):
            latest[name] = run
    return list(latest.values())


def checks_problem(check_runs: list[dict], head_sha: str, protection_contexts: list[str] | None) -> str:
    if protection_contexts is None:
        return "protection_unreadable"
    latest_runs = latest_check_runs(check_runs, head_sha)
    latest = {str(run.get("name") or ""): run for run in latest_runs}
    required = [REQUIRED_CHECK, *[name for name in protection_contexts if name and name != REQUIRED_CHECK]]
    for name in required:
        run = latest.get(name)
        if not run:
            return f"missing_check:{name}"
        app = str(run.get("app_slug") or "")
        if name == REQUIRED_CHECK:
            if app != CHECK_APP:
                return f"unattributed_check:{name}"
        elif not app:
            return f"unattributed_check:{name}"
        if run.get("status") != "completed":
            return f"pending_check:{name}"
        if run.get("conclusion") != "success":
            return f"failed_check:{name}"
    for run in latest_runs:
        name = str(run.get("name") or "")
        if run.get("status") != "completed":
            return f"pending_check:{name}"
        conclusion = str(run.get("conclusion") or "")
        if conclusion in {"success", "neutral", "skipped"}:
            continue
        return f"failed_check:{name}"
    return ""


def review_problem(reviews: list[dict] | None) -> str:
    if reviews is None:
        return "reviews_unreadable"
    latest: dict[str, str] = {}
    ordered = sorted(reviews, key=lambda item: str(item.get("submitted_at") or ""))
    for review in ordered:
        state = str(review.get("state") or "")
        if state not in {"CHANGES_REQUESTED", "APPROVED", "COMMENTED", "DISMISSED"}:
            continue
        latest[str(review.get("user") or "")] = state
    if any(state == "CHANGES_REQUESTED" for state in latest.values()):
        return "changes_requested"
    return ""


def _comment_branches(body: str) -> list[str]:
    fields = parse_marker(body)
    found = []
    if fields and fields.get("branch"):
        found.append(fields["branch"])
    found.extend(BRANCH_LINE_RE.findall(body or ""))
    return found


def _comment_prs(body: str) -> list[str]:
    return [url.rstrip("/") for url in PR_URL_RE.findall(body or "")]


def origin_problem(pr: dict, comments: list[dict]) -> str:
    if pr.get("draft"):
        return "origin_draft"
    if pr.get("head_repo") != pr.get("base_repo"):
        return "origin_fork"
    if pr.get("base_ref") != pr.get("default_branch"):
        return "origin_base"
    owner = str(pr.get("base_repo") or "").split("/")[0]
    if not owner or pr.get("user_login") != owner:
        return "origin_author"
    match = BRANCH_RE.match(str(pr.get("head_ref") or ""))
    if not match:
        return "origin_branch"
    issue = int(match.group(1))
    closed = {int(number) for number in CLOSES_RE.findall(str(pr.get("body") or ""))}
    if issue not in closed:
        return "origin_issue"
    agents = set(AGENT_RE.findall(str(pr.get("body") or "")))
    if len(agents) != 1:
        return "origin_agent"
    agent = next(iter(agents))
    labels = {str(label).lower() for label in pr.get("issue_labels") or []}
    if "no-cursor" in labels:
        return "origin_label"
    matched = []
    for comment in comments or []:
        if comment.get("user") != "github-actions[bot]":
            continue
        fields = parse_marker(comment.get("body") or "")
        if not fields:
            continue
        if fields.get("agent") != agent or str(fields.get("issue")) != str(issue):
            continue
        if fields.get("status") not in {"running", "completed"}:
            continue
        matched.append(comment)
    if not matched:
        return "origin_agent"
    # An older completed comment for a previous attempt must not veto a later one.
    latest = max(matched, key=lambda comment: str(comment.get("created_at") or ""))
    fields = parse_marker(latest.get("body") or "") or {}
    if fields.get("status") != "completed":
        return ""
    html_url = str(pr.get("html_url") or "").rstrip("/")
    head_ref = str(pr.get("head_ref") or "")
    body = latest.get("body") or ""
    prs = _comment_prs(body)
    branches = _comment_branches(body)
    marker_pr = str(fields.get("pr") or "")
    this_pr = html_url.rstrip("/").split("/pull/")[-1]
    if marker_pr and marker_pr != this_pr:
        return "origin_pr_mismatch"
    if prs and html_url not in prs:
        return "origin_pr_mismatch"
    if branches and head_ref not in branches:
        return "origin_pr_mismatch"
    return ""


def currency_problem(comparison_status: str | None, mergeable_state: str | None) -> str:
    if comparison_status in {"behind", "diverged"}:
        return "not_current"
    if mergeable_state == "dirty":
        return "conflict"
    if mergeable_state == "unknown":
        return "mergeable_unknown"
    if comparison_status not in {"ahead", "identical"}:
        return "not_current"
    if mergeable_state not in {"clean", "unstable", "has_hooks", None, ""}:
        return "not_mergeable"
    return ""


def parse_automerge_marker(body: str, created_at: str = "") -> dict | None:
    match = MARKER_RE.search(body or "")
    if not match:
        return None
    fields = dict(FIELD_RE.findall(match.group(1)))
    if not fields.get("pr") or not fields.get("status"):
        return None
    if created_at:
        fields["created_at"] = created_at
    return fields


def markers_from(comments: list[dict]) -> list[dict]:
    found = []
    for comment in comments or []:
        fields = parse_automerge_marker(comment.get("body") or "", str(comment.get("created_at") or ""))
        if fields:
            found.append(fields)
    return found


def _marker_stamp(marker: dict) -> datetime | None:
    created = str(marker.get("created_at") or "")
    if not created:
        return None
    if created.endswith("Z"):
        created = created[:-1] + "+00:00"
    try:
        stamp = datetime.fromisoformat(created)
    except ValueError:
        return None
    if stamp.tzinfo is None:
        stamp = stamp.replace(tzinfo=timezone.utc)
    return stamp


def latest_marker(markers: list[dict], pr_number: int, sha: str) -> dict | None:
    matched = [
        marker
        for marker in markers
        if str(marker.get("pr")) == str(pr_number) and marker.get("sha") == sha and _marker_stamp(marker)
    ]
    if not matched:
        return None
    return max(matched, key=lambda marker: _marker_stamp(marker) or datetime.min.replace(tzinfo=timezone.utc))


def in_flight(markers: list[dict], pr_number: int, sha: str, run_id: str, now: datetime) -> bool:
    """Another run is still deciding this head. A newer terminal marker releases it."""
    latest = latest_marker(markers, pr_number, sha)
    if not latest or latest.get("status") != "evaluating":
        return False
    if str(latest.get("run") or "") == str(run_id):
        return False
    stamp = _marker_stamp(latest)
    if stamp is None:
        return False
    return (now - stamp).total_seconds() < 15 * 60


def already_posted(markers: list[dict], pr_number: int, sha: str, status: str, reason: str) -> bool:
    for marker in markers:
        if str(marker.get("pr")) != str(pr_number) or marker.get("sha") != sha:
            continue
        if marker.get("status") in HARD_STOP_STATUSES:
            return True
        if marker.get("status") == status and marker.get("reason") == reason:
            return True
    return False


def select_deployment(deployments: list[dict]) -> dict | None:
    """The newest GitHub Pages deployment for this SHA. An older success does not count."""
    pages = [item for item in deployments or [] if item.get("environment") == "github-pages"]
    if not pages:
        return None
    # Higher deployment id is newer. If ids are missing, GitHub's newest-first
    # listing means the first item is the one that counts.
    return max(enumerate(pages), key=lambda pair: (int(pair[1].get("id") or 0), -pair[0]))[1]


def smoke_problem(item: dict) -> str:
    if item.get("error"):
        return "smoke_failed"
    if item.get("status_code") != item.get("expect"):
        return "smoke_failed"
    if item.get("snippet_required") and not item.get("snippet"):
        return "smoke_failed"
    if item.get("snippet_required") and item.get("snippet") not in (item.get("body") or ""):
        return "smoke_failed"
    return ""


def deployment_result(docs_changed: bool, deployments: list[dict], smoke: list[dict], token_kind: str) -> dict:
    if not docs_changed:
        return {"outcome": "merged", "reason": "pages_source_unchanged"}
    latest = select_deployment(deployments)
    if latest and str(latest.get("state") or "") == "success":
        if not smoke or any(smoke_problem(item) for item in smoke):
            return {"outcome": "deploy-failed", "reason": "smoke_failed"}
        return {"outcome": "deployed", "reason": "smoke_passed"}
    if latest and str(latest.get("state") or "") in {"failure", "error"}:
        return {"outcome": "deploy-failed", "reason": "pages_deployment_failed"}
    if token_kind == "github_token":
        return {"outcome": "deploy-blocked", "reason": "pages_not_triggered"}
    return {"outcome": "deploy-blocked", "reason": "pages_deployment_missing"}


def merge_methods(allow_squash: bool, allow_merge: bool, allow_rebase: bool) -> list[str]:
    methods = []
    if allow_squash:
        methods.append("squash")
    if allow_merge:
        methods.append("merge")
    if allow_rebase:
        methods.append("rebase")
    return methods or ["merge"]


def decide(ctx: dict) -> dict:
    """Return the gate decision. merge is true only when every gate passed."""
    pr = ctx["pr"]
    number = int(pr["number"])
    sha = str(pr.get("head_sha") or "")
    markers = ctx.get("markers") or []
    now = ctx.get("now") or datetime.now(timezone.utc)
    run_id = str(ctx.get("run_id") or "")
    base = {
        "pr": number,
        "sha": sha,
        "issue_number": None,
        "merge": False,
        "update_branch": False,
        "notify_human": False,
        "comment_on_issue": False,
        "comment_on_pr": False,
        "docs_changed": bool(publication_targets(ctx.get("files") or [])),
    }
    branch = BRANCH_RE.match(str(pr.get("head_ref") or ""))
    if branch:
        base["issue_number"] = int(branch.group(1))
    if pr.get("merged"):
        base.update({"status": "verify-deploy", "reason": "already_merged", "comment_on_issue": False, "comment_on_pr": False})
        return base
    if in_flight(markers, number, sha, run_id, now):
        base.update({"status": "skip", "reason": "in_flight"})
        return base
    origin = origin_problem(pr, ctx.get("comments") or [])
    if origin:
        base.update(
            {
                "status": "blocked-untrusted",
                "reason": origin,
                "comment_on_pr": True,
                "comment_on_issue": False,
                "issue_number": None,
            }
        )
        return base
    base["comment_on_issue"] = True
    base["comment_on_pr"] = True
    if ctx.get("errors"):
        base.update({"status": "failed", "reason": "lookup_failed"})
        return base
    checks = checks_problem(ctx.get("check_runs") or [], sha, ctx.get("protection_contexts"))
    if checks:
        status = "blocked-checks"
        base.update({"status": status, "reason": checks})
        return base
    reviews = review_problem(ctx.get("reviews"))
    if reviews:
        base.update({"status": "blocked-reviews", "reason": reviews})
        return base
    risks = risk_reasons(ctx.get("files") or [])
    if risks:
        base.update({"status": "needs-human", "reason": "risk", "detail_reasons": risks, "notify_human": True})
        return base
    currency = currency_problem(ctx.get("comparison_status"), ctx.get("mergeable_state"))
    if currency == "not_current":
        base.update(
            {
                "status": "blocked-not-current",
                "reason": currency,
                "update_branch": ctx.get("comparison_status") in {"behind", "diverged"},
            }
        )
        return base
    if currency in {"conflict", "mergeable_unknown"}:
        base.update({"status": "blocked-not-current", "reason": currency})
        return base
    if currency:
        base.update({"status": "failed", "reason": currency})
        return base
    if str(pr.get("state") or "") != "open":
        base.update({"status": "blocked-untrusted", "reason": "origin_closed"})
        return base
    base.update({"status": "ready", "reason": "gates_passed", "merge": True})
    return base


def render_comment(
    *,
    pr: int,
    sha: str,
    status: str,
    reason: str,
    repo: str,
    run_id: str,
    detail: str = "",
    pr_url: str = "",
    merge_sha: str = "",
    deployment_url: str = "",
    site_url: str = "",
    smoke: list[dict] | None = None,
) -> str:
    marker = (
        f"<!-- cursor-automerge:v1 pr={pr} sha={sha} status={status} "
        f"run={run_id} reason={reason or 'none'} -->"
    )
    lines = [
        marker,
        "### Cursor auto-merge",
        f"- **Status:** {status}",
        f"- **Reason:** {reason or 'none'}",
        f"- **Workflow run:** https://github.com/{repo}/actions/runs/{run_id}",
    ]
    if pr_url:
        lines.append(f"- **Pull request:** {pr_url}")
    if merge_sha:
        lines.append(f"- **Merge SHA:** `{merge_sha}`")
    if deployment_url:
        lines.append(f"- **Deployment:** {deployment_url}")
    if site_url:
        lines.append(f"- **Published site:** {site_url}")
    for item in smoke or []:
        lines.append(
            f"- **Smoke:** {item.get('url')} -> {item.get('status_code')} "
            f"(expected {item.get('expect')})"
        )
    if detail:
        lines.append(f"- **Detail:** {redact(detail)[:2000]}")
    return "\n".join(lines) + "\n"


def _request(url: str, method: str, token: str, payload: dict | None = None) -> object:
    data = None if payload is None else json.dumps(payload).encode()
    headers = {
        "Accept": "application/vnd.github+json",
        "Content-Type": "application/json",
        "User-Agent": "cursor-automerge",
        "Authorization": "Bearer " + token,
        "X-GitHub-Api-Version": "2022-11-28",
    }
    request = urllib.request.Request(url, data=data, method=method, headers=headers)
    try:
        with urllib.request.urlopen(request, timeout=60) as response:
            raw = response.read().decode()
            return json.loads(raw) if raw else {}
    except urllib.error.HTTPError as exc:
        detail = redact(exc.read().decode(errors="replace")[:800])
        error = urllib.error.HTTPError(exc.url, exc.code, exc.reason, exc.headers, None)
        error.detail = detail  # type: ignore[attr-defined]
        raise error


def _pages(url: str, cache_key: str = "") -> tuple[int, str, str]:
    fetch = url
    if cache_key:
        join = "&" if "?" in url else "?"
        fetch = f"{url}{join}automerge={urllib.request.quote(cache_key)}"
    request = urllib.request.Request(
        fetch,
        headers={"User-Agent": "cruisin-automerge", "Cache-Control": "no-cache"},
    )
    try:
        with urllib.request.urlopen(request, timeout=30) as response:
            return response.status, response.read(200_000).decode("utf-8", errors="replace"), ""
    except urllib.error.HTTPError as exc:
        body = exc.read(200_000).decode("utf-8", errors="replace")
        return exc.code, body, ""
    except urllib.error.URLError as exc:
        return 0, "", redact(str(exc.reason))


def _int_env(name: str, default: int) -> int:
    raw = os.environ.get(name, "").strip()
    return int(raw) if raw.isdigit() else default


def _comments(token: str, repo: str, number: int) -> list[dict]:
    comments: list[dict] = []
    for page in range(1, 6):
        batch = _request(
            f"https://api.github.com/repos/{repo}/issues/{number}/comments?per_page=100&page={page}",
            "GET",
            token,
        )
        if not isinstance(batch, list) or not batch:
            break
        comments.extend(item for item in batch if isinstance(item, dict))
        if len(batch) < 100:
            break
    return comments


def _post(token: str, repo: str, number: int, body: str) -> None:
    _request(
        f"https://api.github.com/repos/{repo}/issues/{number}/comments",
        "POST",
        token,
        {"body": body},
    )


def _check_runs(token: str, repo: str, sha: str) -> list[dict]:
    runs: list[dict] = []
    for page in range(1, 6):
        payload = _request(
            f"https://api.github.com/repos/{repo}/commits/{sha}/check-runs?per_page=100&page={page}",
            "GET",
            token,
        )
        if not isinstance(payload, dict):
            break
        batch = payload.get("check_runs") or []
        if not isinstance(batch, list) or not batch:
            break
        for item in batch:
            if not isinstance(item, dict):
                continue
            app = item.get("app") or {}
            runs.append(
                {
                    "name": item.get("name") or "",
                    "status": item.get("status") or "",
                    "conclusion": item.get("conclusion") or "",
                    "head_sha": item.get("head_sha") or sha,
                    "app_slug": (app.get("slug") if isinstance(app, dict) else "") or "",
                    "started_at": item.get("started_at") or "",
                    "completed_at": item.get("completed_at") or "",
                }
            )
        if len(batch) < 100:
            break
    return runs


def contexts_from_branch(payload: dict | None) -> list[str] | None:
    """Required check names from GET /branches/{branch}. None means unreadable."""
    if not isinstance(payload, dict) or "protected" not in payload:
        return None
    if not payload.get("protected"):
        return []
    protection = payload.get("protection") or {}
    required = protection.get("required_status_checks")
    if not isinstance(required, dict):
        return None
    contexts = [str(item) for item in required.get("contexts") or [] if item]
    for check in required.get("checks") or []:
        if isinstance(check, dict) and check.get("context"):
            contexts.append(str(check["context"]))
    return list(dict.fromkeys(contexts))


def contexts_from_ruleset(payload: dict | None) -> list[str] | None:
    if not isinstance(payload, dict):
        return None
    contexts = []
    for rule in payload.get("rules") or []:
        if not isinstance(rule, dict) or rule.get("type") != "required_status_checks":
            continue
        parameters = rule.get("parameters") or {}
        for check in parameters.get("required_status_checks") or []:
            if isinstance(check, dict) and check.get("context"):
                contexts.append(str(check["context"]))
            elif isinstance(check, str) and check:
                contexts.append(check)
    return contexts


def ruleset_applies(payload: dict, branch: str, is_default: bool) -> bool:
    conditions = payload.get("conditions") if isinstance(payload, dict) else None
    if not isinstance(conditions, dict):
        return True
    ref_name = conditions.get("ref_name") or {}
    include = [str(item) for item in ref_name.get("include") or []]
    exclude = [str(item) for item in ref_name.get("exclude") or []]
    ref = f"refs/heads/{branch}"

    def matches(rule: str) -> bool:
        if rule in {ref, "~ALL"}:
            return True
        if rule == "~DEFAULT_BRANCH" and is_default:
            return True
        if rule.endswith("/**") and ref.startswith(rule[:-3]):
            return True
        return False

    if any(matches(rule) for rule in exclude):
        return False
    if not include:
        return True
    return any(matches(rule) for rule in include)


def _protection_contexts(token: str, repo: str, branch: str, default_branch: str) -> list[str] | None:
    try:
        branch_payload = _request(f"https://api.github.com/repos/{repo}/branches/{branch}", "GET", token)
    except urllib.error.HTTPError:
        return None
    branch_contexts = contexts_from_branch(branch_payload if isinstance(branch_payload, dict) else None)
    if branch_contexts is None:
        return None
    try:
        rulesets = _request(f"https://api.github.com/repos/{repo}/rulesets?per_page=100", "GET", token)
    except urllib.error.HTTPError:
        return None
    if not isinstance(rulesets, list):
        return None
    contexts = list(branch_contexts)
    for ruleset in rulesets:
        if not isinstance(ruleset, dict) or ruleset.get("target") not in {None, "branch"}:
            continue
        if ruleset.get("enforcement") not in {None, "active"}:
            continue
        ruleset_id = ruleset.get("id")
        if not ruleset_id:
            return None
        try:
            detail = _request(f"https://api.github.com/repos/{repo}/rulesets/{ruleset_id}", "GET", token)
        except urllib.error.HTTPError:
            return None
        if not isinstance(detail, dict):
            return None
        if not ruleset_applies(detail, branch, branch == default_branch):
            continue
        extra = contexts_from_ruleset(detail)
        if extra is None:
            return None
        contexts.extend(extra)
    return list(dict.fromkeys(contexts))


def _reviews(token: str, repo: str, number: int) -> list[dict] | None:
    try:
        payload = _request(
            f"https://api.github.com/repos/{repo}/pulls/{number}/reviews?per_page=100",
            "GET",
            token,
        )
    except urllib.error.HTTPError:
        return None
    if not isinstance(payload, list):
        return None
    reviews = []
    for item in payload:
        if not isinstance(item, dict):
            continue
        user = item.get("user") or {}
        reviews.append(
            {
                "user": user.get("login") if isinstance(user, dict) else "",
                "state": item.get("state") or "",
                "submitted_at": item.get("submitted_at") or "",
            }
        )
    return reviews


def _files(token: str, repo: str, number: int) -> list[dict] | None:
    files: list[dict] = []
    try:
        for page in range(1, 6):
            payload = _request(
                f"https://api.github.com/repos/{repo}/pulls/{number}/files?per_page=100&page={page}",
                "GET",
                token,
            )
            if not isinstance(payload, list) or not payload:
                break
            for item in payload:
                if not isinstance(item, dict):
                    continue
                files.append(
                    {
                        "filename": item.get("filename") or "",
                        "status": item.get("status") or "",
                        "previous_filename": item.get("previous_filename") or "",
                        "patch": item.get("patch") or "",
                        "changes": item.get("changes"),
                    }
                )
            if len(payload) < 100:
                break
            if page == 5:
                return None
    except urllib.error.HTTPError:
        return None
    return files


def _file_text(token: str, repo: str, path: str, ref: str) -> str:
    quoted = urllib.request.quote(path)
    url = f"https://api.github.com/repos/{repo}/contents/{quoted}?ref={ref}"
    request = urllib.request.Request(
        url,
        headers={
            "Accept": "application/vnd.github.raw",
            "Authorization": "Bearer " + token,
            "User-Agent": "cursor-automerge",
            "X-GitHub-Api-Version": "2022-11-28",
        },
    )
    with urllib.request.urlopen(request, timeout=60) as response:
        return response.read(200_000).decode("utf-8", errors="replace")


def _pull(token: str, repo: str, number: int) -> dict:
    payload = _request(f"https://api.github.com/repos/{repo}/pulls/{number}", "GET", token)
    if not isinstance(payload, dict):
        raise RuntimeError("pull request lookup failed")
    head = payload.get("head") or {}
    base = payload.get("base") or {}
    head_repo = (head.get("repo") or {}) if isinstance(head, dict) else {}
    base_repo = (base.get("repo") or {}) if isinstance(base, dict) else {}
    user = payload.get("user") or {}
    return {
        "number": int(payload.get("number") or number),
        "draft": bool(payload.get("draft")),
        "merged": bool(payload.get("merged")),
        "state": payload.get("state") or "",
        "head_ref": head.get("ref") if isinstance(head, dict) else "",
        "head_sha": head.get("sha") if isinstance(head, dict) else "",
        "head_repo": head_repo.get("full_name") if isinstance(head_repo, dict) else "",
        "base_ref": base.get("ref") if isinstance(base, dict) else "",
        "base_repo": base_repo.get("full_name") if isinstance(base_repo, dict) else "",
        "default_branch": (base_repo.get("default_branch") if isinstance(base_repo, dict) else "") or "master",
        "body": payload.get("body") or "",
        "html_url": payload.get("html_url") or "",
        "title": payload.get("title") or f"PR #{number}",
        "user_login": user.get("login") if isinstance(user, dict) else "",
        "mergeable_state": payload.get("mergeable_state") or "unknown",
        "merge_commit_sha": payload.get("merge_commit_sha") or "",
        "issue_labels": [],
    }


def _load_issue(token: str, repo: str, number: int) -> tuple[list[str], list[dict]]:
    try:
        return _issue_labels(token, repo, number), _comments(token, repo, number)
    except urllib.error.HTTPError as exc:
        if exc.code == 404:
            return [], []
        raise


def _issue_labels(token: str, repo: str, number: int) -> list[str]:
    payload = _request(f"https://api.github.com/repos/{repo}/issues/{number}", "GET", token)
    if not isinstance(payload, dict):
        return []
    return [str(item.get("name") or "") for item in payload.get("labels") or [] if isinstance(item, dict)]


def _compare(token: str, repo: str, base: str, head: str) -> str:
    payload = _request(
        f"https://api.github.com/repos/{repo}/compare/{urllib.request.quote(base, safe='')}...{head}",
        "GET",
        token,
    )
    if not isinstance(payload, dict):
        return ""
    return str(payload.get("status") or "")


def _deployments(token: str, repo: str, sha: str) -> list[dict]:
    payload = _request(
        f"https://api.github.com/repos/{repo}/deployments?sha={sha}&environment=github-pages&per_page=20",
        "GET",
        token,
    )
    if not isinstance(payload, list):
        return []
    found = []
    for item in payload:
        if not isinstance(item, dict):
            continue
        statuses = _request(str(item.get("statuses_url") or ""), "GET", token) if item.get("statuses_url") else []
        state = ""
        target = ""
        site = ""
        if isinstance(statuses, list) and statuses:
            latest = statuses[0] if isinstance(statuses[0], dict) else {}
            state = str(latest.get("state") or "")
            target = str(latest.get("target_url") or latest.get("log_url") or "")
            site = str(latest.get("environment_url") or "")
        found.append(
            {
                "sha": item.get("sha") or sha,
                "environment": item.get("environment") or "",
                "state": state,
                "target_url": target,
                "site_url": site,
                "id": item.get("id") or 0,
            }
        )
    return found


def _find_pr_number(token: str, repo: str, sha: str) -> int | None:
    if not sha:
        return None
    payload = _request(
        f"https://api.github.com/repos/{repo}/commits/{sha}/pulls",
        "GET",
        token,
    )
    if not isinstance(payload, list):
        return None
    numbers = []
    for item in payload:
        if isinstance(item, dict) and isinstance(item.get("number"), int):
            numbers.append(item["number"])
    unique = list(dict.fromkeys(numbers))
    if len(unique) != 1:
        return None
    return unique[0]


def _smoke(targets: list[dict], cache_key: str = "") -> list[dict]:
    results = []
    for target in targets:
        code, body, error = _pages(str(target["url"]), cache_key)
        results.append(
            {
                "url": target["url"],
                "path": target["path"],
                "expect": target["expect"],
                "snippet": target.get("snippet") or "",
                "snippet_required": bool(target.get("snippet_required")),
                "status_code": code,
                "body": body,
                "error": error,
            }
        )
    return results


def _fill_content(token: str, repo: str, files: list[dict], ref: str) -> None:
    for item in files:
        path = item.get("filename") or ""
        if item.get("status") == "removed" or not path.startswith("docs/"):
            continue
        if _ext(path) not in TEXT_EXT:
            continue
        try:
            item["content"] = _file_text(token, repo, path, ref)
        except urllib.error.HTTPError as exc:
            item["content"] = ""
            item["content_error"] = f"HTTP {exc.code}"


def _notify(token: str, repo: str, number: int, owner: str) -> str:
    try:
        _request(
            f"https://api.github.com/repos/{repo}/pulls/{number}/requested_reviewers",
            "POST",
            token,
            {"reviewers": [owner]},
        )
        return ""
    except urllib.error.HTTPError as exc:
        if exc.code in {422, 409}:
            return ""
        return f"HTTP {exc.code}: {getattr(exc, 'detail', '')}"


def _update_branch(token: str, repo: str, number: int, sha: str) -> str:
    try:
        _request(
            f"https://api.github.com/repos/{repo}/pulls/{number}/update-branch",
            "PUT",
            token,
            {"expected_head_sha": sha},
        )
        return ""
    except urllib.error.HTTPError as exc:
        return f"HTTP {exc.code}: {getattr(exc, 'detail', '')}"


def _merge(token: str, repo: str, number: int, sha: str, title: str, methods: list[str]) -> tuple[str, str]:
    last = "merge was not attempted"
    for method in methods:
        try:
            payload = _request(
                f"https://api.github.com/repos/{repo}/pulls/{number}/merge",
                "PUT",
                token,
                {"sha": sha, "merge_method": method, "commit_title": title},
            )
        except urllib.error.HTTPError as exc:
            last = f"HTTP {exc.code}: {getattr(exc, 'detail', '')}"
            if exc.code in {405, 409}:
                continue
            return "", last
        if isinstance(payload, dict) and payload.get("merged") and payload.get("sha"):
            return str(payload["sha"]), ""
        last = "merge response did not include a sha"
    return "", last


def _record(token: str, repo: str, decision: dict, body: str) -> None:
    if decision.get("comment_on_issue") and decision.get("issue_number"):
        _post(token, repo, int(decision["issue_number"]), body)
    if decision.get("comment_on_pr"):
        _post(token, repo, int(decision["pr"]), body)


def _detail_for(decision: dict, extra: str = "") -> str:
    reasons = decision.get("detail_reasons") or []
    parts = []
    if reasons:
        parts.append("Risk: " + ", ".join(str(item) for item in reasons[:20]))
    if decision.get("reason") == "pages_not_triggered":
        parts.append(
            "GitHub does not start a Pages build for a commit pushed by GITHUB_TOKEN. "
            "The site source is master:/docs at https://nathanaherne.github.io/cruisin/. "
            "There is no deploy credential in this repository. Add Actions secret "
            "CRUISIN_PAGES_PUSH_TOKEN: a fine-grained personal access token for nathanaherne "
            "with contents read and write on nathanaherne/cruisin only, from an account with "
            "a verified email. No other permission. Then rerun Cursor safe auto-merge for this "
            "pull request. Do not send the token in a comment or issue."
        )
    if extra:
        parts.append(extra)
    return " ".join(parts)


def main() -> int:
    repo = os.environ.get("REPO") or os.environ.get("GITHUB_REPOSITORY") or ""
    token = os.environ.get("GITHUB_TOKEN", "").strip()
    pages_token = os.environ.get("PAGES_PUSH_TOKEN", "").strip()
    run_id = os.environ.get("GITHUB_RUN_ID", "")
    event = os.environ.get("EVENT_NAME", "")
    pr_raw = os.environ.get("PR_NUMBER", "").strip()
    head_hint = os.environ.get("HEAD_SHA", "").strip()
    pages_base = os.environ.get("PAGES_BASE", PAGES_BASE).strip() or PAGES_BASE
    if not repo or not token:
        print("REPO and GITHUB_TOKEN are required", file=sys.stderr)
        return 1
    if event == "workflow_run" and os.environ.get("CI_EVENT", "") not in {"", "pull_request"}:
        print(json.dumps({"status": "skip", "reason": "not_pull_request"}))
        return 0
    try:
        if pr_raw.isdigit():
            number = int(pr_raw)
        else:
            found = _find_pr_number(token, repo, head_hint)
            if found is None:
                print(json.dumps({"status": "skip", "reason": "pull_request_not_found"}))
                return 0
            number = found
        pr = _pull(token, repo, number)
        if head_hint and not pr["merged"] and pr["head_sha"] != head_hint:
            print(json.dumps({"status": "skip", "reason": "head_moved", "pr": number}))
            return 0
        branch_issue = BRANCH_RE.match(str(pr["head_ref"] or ""))
        issue_comments: list[dict] = []
        if branch_issue:
            issue_number = int(branch_issue.group(1))
            pr["issue_labels"], issue_comments = _load_issue(token, repo, issue_number)
        pr_comments = _comments(token, repo, number)
        files = _files(token, repo, number)
        if files is None:
            raise RuntimeError("file list lookup failed")
        _fill_content(token, repo, files, str(pr["head_sha"]))
        protection = _protection_contexts(
            token,
            repo,
            str(pr["base_ref"] or pr["default_branch"]),
            str(pr["default_branch"]),
        )
        reviews = _reviews(token, repo, number)
        comparison = _compare(token, repo, str(pr["base_ref"]), str(pr["head_sha"]))
        for _ in range(4):
            if pr["mergeable_state"] != "unknown" or pr["merged"]:
                break
            time.sleep(_int_env("MERGEABLE_POLL_SECONDS", 5))
            pr = _pull(token, repo, number)
            if branch_issue:
                pr["issue_labels"], issue_comments = _load_issue(token, repo, int(branch_issue.group(1)))
        markers = markers_from(issue_comments + pr_comments)
        decision = decide(
            {
                "pr": pr,
                "comments": issue_comments,
                "files": files,
                "check_runs": _check_runs(token, repo, str(pr["head_sha"])),
                "protection_contexts": protection,
                "reviews": reviews,
                "comparison_status": comparison,
                "mergeable_state": pr["mergeable_state"],
                "markers": markers,
                "run_id": run_id,
                "now": datetime.now(timezone.utc),
            }
        )
        targets = publication_targets(files, pages_base)
        decision["docs_changed"] = bool(targets)
        status = decision["status"]
        reason = decision["reason"]
        if status == "skip":
            print(json.dumps({"status": status, "reason": reason, "pr": number}))
            return 0
        if status != "verify-deploy" and already_posted(markers, number, str(pr["head_sha"]), status, reason):
            print(json.dumps({"status": "skip", "reason": "duplicate", "pr": number}))
            return 0
        if status == "ready" and decision["merge"]:
            lock = render_comment(
                pr=number,
                sha=str(pr["head_sha"]),
                status="evaluating",
                reason="gates_passed",
                repo=repo,
                run_id=run_id,
                pr_url=str(pr["html_url"]),
            )
            _record(token, repo, {**decision, "status": "evaluating"}, lock)
            time.sleep(_int_env("EVALUATING_WAIT_SECONDS", 3))
            issue_number = decision.get("issue_number")
            fresh_comments = _comments(token, repo, number)
            if issue_number:
                fresh_comments = _comments(token, repo, int(issue_number)) + fresh_comments
            if in_flight(markers_from(fresh_comments), number, str(pr["head_sha"]), run_id, datetime.now(timezone.utc)):
                print(json.dumps({"status": "skip", "reason": "in_flight", "pr": number}))
                return 0
            repo_info = _request(f"https://api.github.com/repos/{repo}", "GET", token)
            flags = repo_info if isinstance(repo_info, dict) else {}
            methods = merge_methods(
                bool(flags.get("allow_squash_merge", True)),
                bool(flags.get("allow_merge_commit", True)),
                bool(flags.get("allow_rebase_merge", True)),
            )
            merge_token = pages_token or token
            merge_sha, merge_error = _merge(
                merge_token, repo, number, str(pr["head_sha"]), str(pr.get("title") or f"PR #{number}"), methods
            )
            if not merge_sha:
                recovered = _pull(token, repo, number)
                recovered_sha = str(recovered.get("merge_commit_sha") or "")
                if recovered.get("merged") and recovered_sha:
                    return _finish_deploy(
                        token,
                        repo,
                        run_id,
                        recovered,
                        decision,
                        targets,
                        recovered_sha,
                        "pages_push_token" if pages_token else "github_token",
                        pages_base,
                    )
                body = render_comment(
                    pr=number,
                    sha=str(pr["head_sha"]),
                    status="failed",
                    reason="merge_failed",
                    repo=repo,
                    run_id=run_id,
                    pr_url=str(pr["html_url"]),
                    detail=_detail_for(decision, merge_error),
                )
                decision["status"] = "failed"
                decision["reason"] = "merge_failed"
                _record(token, repo, decision, body)
                print(json.dumps({"status": "failed", "reason": "merge_failed", "pr": number}))
                return 1
            token_kind = "pages_push_token" if pages_token else "github_token"
            return _finish_deploy(
                token,
                repo,
                run_id,
                pr,
                decision,
                targets,
                merge_sha,
                token_kind,
                pages_base,
            )
        if decision.get("update_branch"):
            error = _update_branch(token, repo, number, str(pr["head_sha"]))
            detail = "Updated the branch onto the base." if not error else f"Branch update failed. {error}"
            body = render_comment(
                pr=number,
                sha=str(pr["head_sha"]),
                status="blocked-not-current",
                reason="update_requested" if not error else "update_failed",
                repo=repo,
                run_id=run_id,
                pr_url=str(pr["html_url"]),
                detail=detail,
            )
            decision["reason"] = "update_requested" if not error else "update_failed"
            _record(token, repo, decision, body)
            print(json.dumps({"status": "blocked-not-current", "reason": decision["reason"], "pr": number}))
            return 0
        if status == "verify-deploy":
            merge_sha = str(pr.get("merge_commit_sha") or "")
            if not merge_sha:
                print(json.dumps({"status": "skip", "reason": "merged_sha_missing", "pr": number}))
                return 0
            token_kind = "pages_push_token" if pages_token else "github_token"
            decision["comment_on_issue"] = bool(decision.get("issue_number"))
            decision["comment_on_pr"] = True
            return _finish_deploy(
                token,
                repo,
                run_id,
                pr,
                decision,
                targets,
                merge_sha,
                token_kind,
                pages_base,
            )
        if decision.get("notify_human"):
            owner = repo.split("/")[0]
            note = _notify(token, repo, number, owner)
            if note:
                decision["detail_reasons"] = list(decision.get("detail_reasons") or []) + [note]
        body = render_comment(
            pr=number,
            sha=str(pr["head_sha"]),
            status=status,
            reason=reason,
            repo=repo,
            run_id=run_id,
            pr_url=str(pr["html_url"]),
            detail=_detail_for(decision),
        )
        _record(token, repo, decision, body)
        print(json.dumps({"status": status, "reason": reason, "pr": number}))
        return 0
    except urllib.error.HTTPError as exc:
        print(f"GitHub API HTTP {exc.code}: {getattr(exc, 'detail', '')}", file=sys.stderr)
        return 1
    except Exception as exc:  # noqa: BLE001 - record the operational failure and stop
        print(redact(str(exc)), file=sys.stderr)
        return 1


def _finish_deploy(
    token: str,
    repo: str,
    run_id: str,
    pr: dict,
    decision: dict,
    targets: list[dict],
    merge_sha: str,
    token_kind: str,
    pages_base: str,
) -> int:
    attempts = _int_env("PAGES_POLL_ATTEMPTS", 20)
    interval = _int_env("PAGES_POLL_SECONDS", 15)
    deployments: list[dict] = []
    chosen: dict | None = None
    if targets:
        for attempt in range(attempts):
            deployments = _deployments(token, repo, merge_sha)
            chosen = select_deployment(deployments)
            state = str((chosen or {}).get("state") or "")
            if state in {"success", "failure", "error"}:
                break
            if attempt < attempts - 1:
                time.sleep(interval)
        smoke = []
        if chosen and str(chosen.get("state") or "") == "success":
            smoke_attempts = _int_env("PAGES_SMOKE_ATTEMPTS", 4)
            smoke_interval = _int_env("PAGES_SMOKE_SECONDS", 20)
            for attempt in range(smoke_attempts):
                smoke = _smoke(targets, merge_sha)
                if not any(smoke_problem(item) for item in smoke):
                    break
                if attempt < smoke_attempts - 1:
                    time.sleep(smoke_interval)
    else:
        code, _body, error = _pages(pages_base + "CURSOR_ISSUE_AUTOMATION.html", merge_sha)
        smoke = [{"url": pages_base + "CURSOR_ISSUE_AUTOMATION.html", "status_code": code, "expect": 200, "error": error}]
        deployments = []
    result = deployment_result(bool(targets), deployments, smoke if targets else [], token_kind)
    if not targets:
        health = smoke[0]["status_code"] if smoke else 0
        result = {"outcome": "merged", "reason": "pages_source_unchanged"}
        extra = f"Published docs page health check returned HTTP {health}."
    else:
        extra = ""
    chosen = select_deployment(deployments)
    deployment_url = str((chosen or {}).get("target_url") or "")
    site_url = str((chosen or {}).get("site_url") or "")
    terminal = str(result["outcome"])
    if already_posted(markers_from(_comments(token, repo, int(pr["number"]))), int(pr["number"]), str(pr["head_sha"]), terminal, str(result["reason"])):
        print(json.dumps({"status": "skip", "reason": "duplicate", "pr": pr["number"]}))
        return 0
    body = render_comment(
        pr=int(pr["number"]),
        sha=str(pr["head_sha"]),
        status=terminal,
        reason=str(result["reason"]),
        repo=repo,
        run_id=run_id,
        pr_url=str(pr["html_url"]),
        merge_sha=merge_sha,
        deployment_url=deployment_url,
        site_url=site_url,
        smoke=smoke if targets else smoke,
        detail=_detail_for({"reason": result["reason"]}, extra),
    )
    decision = dict(decision)
    decision["status"] = terminal
    decision["comment_on_issue"] = bool(decision.get("issue_number"))
    decision["comment_on_pr"] = True
    _record(token, repo, decision, body)
    print(json.dumps({"status": terminal, "reason": result["reason"], "pr": pr["number"], "merge_sha": merge_sha}))
    return 0 if terminal in {"deployed", "merged"} else 1


if __name__ == "__main__":
    raise SystemExit(main())
