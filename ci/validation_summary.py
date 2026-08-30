"""Write and aggregate truthful release-lane validation results."""

from __future__ import annotations

import argparse
import json
from pathlib import Path


def _state(value: str) -> str:
    return {
        "success": "✅ Passed",
        "failure": "❌ Failed",
        "cancelled": "❌ Cancelled",
        "skipped": "— Not reached",
        "not-run": "⚠️ Not run",
    }.get(value or "skipped", f"— {value or 'Not reached'}")


def write_lane(args: argparse.Namespace) -> None:
    result = {
        "platform": args.platform,
        "python": args.python,
        "wheel": args.wheel,
        "installed": args.installed,
        "installed_contracts": args.installed_contracts,
        "native_contracts": args.native_contracts,
        "runtime": args.runtime,
        "runtime_result": args.runtime_result,
    }
    Path(args.output).write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")


def summarize(args: argparse.Namespace) -> None:
    results_by_lane = {}
    for path in Path(args.input).rglob("*.json"):
        item = json.loads(path.read_text(encoding="utf-8"))
        results_by_lane[(item["platform"], item["python"])] = item
    metadata = json.loads(Path(args.metadata).read_text(encoding="utf-8"))
    for platform in metadata["platforms"]:
        for python in metadata["python"]:
            key = (platform["label"], python)
            results_by_lane.setdefault(
                key,
                {
                    "platform": platform["label"],
                    "python": python,
                    "wheel": "skipped",
                    "installed": "skipped",
                    "installed_contracts": "skipped",
                    "native_contracts": "not-run" if python != "3.11" else "skipped",
                    "runtime": "Platform runtime covered by Py3.11"
                    if python != "3.11"
                    else " + ".join(platform["runtime_backends"]),
                    "runtime_result": "not-run" if python != "3.11" else "skipped",
                },
            )
    results = list(results_by_lane.values())
    results.sort(key=lambda item: (item["platform"], item["python"]))
    lines = [
        "## Workbench Validation",
        "",
        "Each cell reports that exact layer; a built wheel never implies a runtime pass.",
        "",
        "| Platform | Py | Wheel | Installed | Installed contracts | Native contracts | Runtime |",
        "|---|---:|---|---|---|---|---|",
    ]
    for item in results:
        native = _state(item["native_contracts"])
        runtime = _state(item["runtime_result"])
        if item["runtime"]:
            runtime = f'{runtime} — {item["runtime"]}'
        lines.append(
            f'| {item["platform"]} | {item["python"]} | {_state(item["wheel"])} | '
            f'{_state(item["installed"])} | {_state(item["installed_contracts"])} | '
            f"{native} | {runtime} |"
        )
    Path(args.output).write_text("\n".join(lines) + "\n", encoding="utf-8")


def release_support(args: argparse.Namespace) -> None:
    data = json.loads(Path(args.metadata).read_text(encoding="utf-8"))
    platforms = ", ".join(item["label"] for item in data["platforms"])
    backends = "; ".join(
        f'{item["label"]}: {" + ".join(item["runtime_backends"])}' for item in data["platforms"]
    )
    print("**Install:** `uvx miskeyed-workbench`")
    print(f'\n**Python:** {data["python"][0]}–{data["python"][-1]}')
    print(f"\n**Wheels:** {platforms}")
    print(f"\n**Runtime validated:** {backends}")


def main() -> None:
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(dest="command", required=True)
    lane = subparsers.add_parser("lane")
    for name in (
        "platform",
        "python",
        "wheel",
        "installed",
        "installed-contracts",
        "native-contracts",
        "runtime",
        "runtime-result",
        "output",
    ):
        lane.add_argument(f"--{name}", required=True)
    lane.set_defaults(handler=write_lane)
    summary = subparsers.add_parser("summary")
    summary.add_argument("--input", required=True)
    summary.add_argument("--output", required=True)
    summary.add_argument("--metadata", default="ci/support-matrix.json")
    summary.set_defaults(handler=summarize)
    support = subparsers.add_parser("release-support")
    support.add_argument("--metadata", default="ci/support-matrix.json")
    support.set_defaults(handler=release_support)
    args = parser.parse_args()
    args.handler(args)


if __name__ == "__main__":
    main()
