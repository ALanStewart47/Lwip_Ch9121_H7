#!/usr/bin/env python3
"""Create a verified, comment-reduced release copy of this product."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterable


PRODUCT_ROOT = Path(__file__).resolve().parents[2]
SOURCE_DIRS = {"App", "Bsp", "Protocol"}
EXCLUDED_ROOT_DIRS = {
    "_Doc",
    "release-publish-skill",
    "协议代码文档 -2025-11-11（模块化新增）",
}
EXCLUDED_ROOT_FILES = {"README.md", "STATUS.md", "CHANGELOG.md"}
FIRMWARE_SUFFIXES = {".hex", ".bin"}
MULTI_CHAR_TOKENS = tuple(
    sorted(
        (
            "<<=", ">>=", "...", "->", "++", "--", "<<", ">>", "<=", ">=",
            "==", "!=", "&&", "||", "+=", "-=", "*=", "/=", "%=", "&=",
            "|=", "^=", "##",
        ),
        key=len,
        reverse=True,
    )
)
RISK_PATTERNS = {
    "high": re.compile(r"\b(strcpy|strcat|sprintf|vsprintf|gets)\s*\("),
    "review": re.compile(r"\b(memcpy|memmove|strncpy|snprintf)\s*\("),
}


class PublishError(RuntimeError):
    pass


def reject_unsafe_comment_splices(text: str) -> None:
    if re.search(r"/\\\r?\n\*|\*\\\r?\n/", text):
        raise PublishError("comment delimiter crosses a line splice; manual review is required")


@dataclass
class Report:
    source: Path
    output: Path
    status: str = "FAILED"
    messages: list[str] = field(default_factory=list)
    processed_files: list[str] = field(default_factory=list)
    firmware_hashes: dict[str, str] = field(default_factory=dict)
    risks: list[str] = field(default_factory=list)

    def add(self, message: str) -> None:
        self.messages.append(message)


def is_excluded_path(relative_path: Path) -> bool:
    if not relative_path.parts:
        return False
    if relative_path.parts[0] in EXCLUDED_ROOT_DIRS:
        return True
    return len(relative_path.parts) == 1 and relative_path.name in EXCLUDED_ROOT_FILES


def is_processed_source(relative_path: Path) -> bool:
    return (
        len(relative_path.parts) >= 2
        and relative_path.parts[0] in SOURCE_DIRS
        and relative_path.suffix.lower() in {".c", ".h"}
    )


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def read_c_source(path: Path) -> tuple[str, str]:
    raw = path.read_bytes()
    if raw.startswith(b"\xef\xbb\xbf"):
        return raw.decode("utf-8-sig"), "utf-8-sig"
    try:
        return raw.decode("utf-8"), "utf-8"
    except UnicodeDecodeError:
        return raw.decode("gbk"), "gbk"


def write_c_source(path: Path, text: str, encoding: str) -> None:
    path.write_bytes(text.encode(encoding))

def _newline_end(text: str, index: int) -> int:
    if index < len(text) and text[index] == "\r":
        return index + 2 if index + 1 < len(text) and text[index + 1] == "\n" else index + 1
    return index + 1


def _consume_line_comment(text: str, index: int) -> tuple[int, str]:
    index += 2
    newlines: list[str] = []
    while index < len(text):
        if text[index] in "\r\n":
            end = _newline_end(text, index)
            newlines.append(text[index:end])
            return end, "".join(newlines)
        if text[index] == "\\" and index + 1 < len(text) and text[index + 1] in "\r\n":
            end = _newline_end(text, index + 1)
            newlines.append(text[index + 1:end])
            index = end
            continue
        index += 1
    return index, "".join(newlines)


def _consume_block_comment(text: str, index: int) -> tuple[int, str]:
    index += 2
    newlines: list[str] = []
    while index < len(text):
        if text[index] == "\\" and index + 1 < len(text) and text[index + 1] in "\r\n":
            end = _newline_end(text, index + 1)
            newlines.append(text[index + 1:end])
            index = end
            continue
        if text.startswith("*/", index):
            return index + 2, "".join(newlines)
        if text[index] in "\r\n":
            end = _newline_end(text, index)
            newlines.append(text[index:end])
            index = end
            continue
        index += 1
    raise PublishError("unterminated block comment")


def _consume_quoted(text: str, index: int, quote: str) -> int:
    index += 1
    while index < len(text):
        current = text[index]
        if current == "\\":
            if index + 1 >= len(text):
                raise PublishError("unfinished escape sequence in literal")
            index += 2
            continue
        if current == quote:
            return index + 1
        if current in "\r\n":
            raise PublishError("newline in an unterminated literal")
        index += 1
    raise PublishError("unterminated literal")


def _header_end(text: str) -> int:
    index = 1 if text.startswith("\ufeff") else 0
    preserved_end = index
    saw_comment = False
    while True:
        whitespace_start = index
        while index < len(text) and text[index].isspace():
            index += 1
        if text.startswith("//", index):
            index, _ = _consume_line_comment(text, index)
            preserved_end = index
            saw_comment = True
            continue
        if text.startswith("/*", index):
            index, _ = _consume_block_comment(text, index)
            preserved_end = index
            saw_comment = True
            continue
        return preserved_end if saw_comment else whitespace_start


def strip_comments_preserving_header(text: str) -> str:
    header_end = _header_end(text)
    output = [text[:header_end]]
    index = header_end
    while index < len(text):
        current = text[index]
        if current in {'"', "'"}:
            end = _consume_quoted(text, index, current)
            output.append(text[index:end])
            index = end
            continue
        if text.startswith("//", index):
            index, newlines = _consume_line_comment(text, index)
            output.append(" " + newlines)
            continue
        if text.startswith("/*", index):
            index, newlines = _consume_block_comment(text, index)
            output.append(" " + newlines)
            continue
        output.append(current)
        index += 1
    return "".join(output)


def _skip_comment(text: str, index: int) -> int:
    if text.startswith("//", index):
        return _consume_line_comment(text, index)[0]
    if text.startswith("/*", index):
        return _consume_block_comment(text, index)[0]
    return index


def token_sequence(text: str) -> tuple[str, ...]:
    tokens: list[str] = []
    index = 0
    while index < len(text):
        current = text[index]
        if current.isspace():
            index += 1
            continue
        if text.startswith("//", index) or text.startswith("/*", index):
            index = _skip_comment(text, index)
            continue
        if current in {'"', "'"}:
            end = _consume_quoted(text, index, current)
            tokens.append(text[index:end])
            index = end
            continue
        if current.isalpha() or current == "_":
            end = index + 1
            while end < len(text) and (text[end].isalnum() or text[end] == "_"):
                end += 1
            tokens.append(text[index:end])
            index = end
            continue
        if current.isdigit() or (current == "." and index + 1 < len(text) and text[index + 1].isdigit()):
            end = index + 1
            while end < len(text):
                candidate = text[end]
                if candidate.isalnum() or candidate in "._":
                    end += 1
                    continue
                if candidate in "+-" and end > index and text[end - 1] in "eEpP":
                    end += 1
                    continue
                break
            tokens.append(text[index:end])
            index = end
            continue
        operator = next((item for item in MULTI_CHAR_TOKENS if text.startswith(item, index)), None)
        if operator:
            tokens.append(operator)
            index += len(operator)
            continue
        tokens.append(current)
        index += 1
    return tuple(tokens)


def has_disallowed_comments(text: str) -> bool:
    index = _header_end(text)
    while index < len(text):
        if text[index] in {'"', "'"}:
            index = _consume_quoted(text, index, text[index])
            continue
        if text.startswith("//", index) or text.startswith("/*", index):
            return True
        index += 1
    return False


def iter_source_files(root: Path) -> Iterable[Path]:
    for path in root.rglob("*"):
        if path.is_file():
            yield path


def copy_project(source: Path, output: Path) -> None:
    for source_file in iter_source_files(source):
        relative = source_file.relative_to(source)
        if is_excluded_path(relative):
            continue
        target = output / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source_file, target)


def verify_unprocessed_files(source: Path, output: Path) -> None:
    for source_file in iter_source_files(source):
        relative = source_file.relative_to(source)
        if is_excluded_path(relative) or is_processed_source(relative):
            continue
        target = output / relative
        if not target.is_file() or sha256_file(source_file) != sha256_file(target):
            raise PublishError(f"unprocessed file differs: {relative}")


def process_source_files(source: Path, output: Path, report: Report) -> None:
    for source_file in iter_source_files(source):
        relative = source_file.relative_to(source)
        if not is_processed_source(relative):
            continue
        target = output / relative
        original, encoding = read_c_source(source_file)
        reject_unsafe_comment_splices(original)
        transformed = strip_comments_preserving_header(original)
        if token_sequence(original) != token_sequence(transformed):
            raise PublishError(f"code token mismatch after comment removal: {relative}")
        if has_disallowed_comments(transformed):
            raise PublishError(f"disallowed comment remains after processing: {relative}")
        write_c_source(target, transformed, encoding)
        report.processed_files.append(str(relative).replace("\\", "/"))


def firmware_files(root: Path) -> list[Path]:
    return sorted(
        path for path in iter_source_files(root)
        if path.suffix.lower() in FIRMWARE_SUFFIXES and not is_excluded_path(path.relative_to(root))
    )


_KEIL_SUMMARY = re.compile(
    r"(\d+)\s+Error\(s\)\s*,\s*(\d+)\s+Warning\(s\)",
    re.IGNORECASE,
)


def resolve_keil(executable: str | None) -> str:
    candidates = [executable] if executable else []
    candidates.extend([shutil.which("UV4.exe"), r"C:\Keil_v5\UV4\UV4.exe"])
    for candidate in candidates:
        if candidate and Path(candidate).is_file():
            return str(candidate)
    raise PublishError("Keil UV4.exe was not found; pass --keil with its full path")


def uv4_build_command(keil: str, project_path: Path, target: str, log_path: Path) -> list[str]:
    return [
        keil,
        "-j0",
        "-b",
        str(project_path),
        "-t",
        target,
        "-o",
        str(log_path),
    ]


def require_zero_error_keil_build(exit_code: int, log_text: str) -> tuple[int, int]:
    matches = list(_KEIL_SUMMARY.finditer(log_text or ""))
    if not matches:
        raise PublishError("original Keil build did not complete with zero errors")
    errors = int(matches[-1].group(1))
    warnings = int(matches[-1].group(2))
    if exit_code not in {0, 1} or errors != 0:
        raise PublishError("original Keil build did not complete with zero errors")
    return errors, warnings


def build_original(source: Path, keil: str | None, project: Path, target: str, report: Report) -> list[Path]:
    project_path = source / project
    if not project_path.is_file():
        raise PublishError(f"Keil project does not exist: {project}")
    log_path = (source / "MDK-ARM" / "release_publish_uv4.log").resolve()
    log_path.parent.mkdir(parents=True, exist_ok=True)
    command = uv4_build_command(resolve_keil(keil), project_path, target, log_path)
    result = subprocess.run(command, cwd=source, text=True, capture_output=True, check=False)
    log_text = log_path.read_text(encoding="utf-8", errors="replace") if log_path.is_file() else ""
    report.add("Build command: " + " ".join(command))
    report.add("Build exit code: " + str(result.returncode))
    report.add("Build log: " + str(log_path))
    errors, warnings = require_zero_error_keil_build(result.returncode, log_text)
    report.add(f"Build summary: {errors} Error(s), {warnings} Warning(s)")
    artifacts = firmware_files(source)
    if not artifacts:
        raise PublishError("original build produced no .hex or .bin firmware artifact")
    for artifact in artifacts:
        relative = artifact.relative_to(source)
        report.firmware_hashes[str(relative).replace("\\", "/")] = sha256_file(artifact)
    return artifacts


def verify_firmware_copy(output: Path, report: Report) -> None:
    for relative_name, original_hash in report.firmware_hashes.items():
        copied = output / Path(relative_name)
        if not copied.is_file() or sha256_file(copied) != original_hash:
            raise PublishError(f"firmware artifact differs: {relative_name}")


def scan_risks(root: Path) -> list[str]:
    findings: list[str] = []
    for path in iter_source_files(root):
        relative = path.relative_to(root)
        if not is_processed_source(relative):
            continue
        try:
            content = path.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            continue
        for severity, pattern in RISK_PATTERNS.items():
            for match in pattern.finditer(content):
                line = content.count("\n", 0, match.start()) + 1
                findings.append(f"{severity}: {relative}:{line}: {match.group(1)} requires review")
    return findings


def write_report(report: Report) -> Path:
    report_path = report.output.parent / f"{report.output.name}_REPORT.md"
    lines = [
        "# 发布副本报告",
        "",
        f"- 状态：{report.status}",
        f"- 正本：`{report.source}`",
        f"- 副本：`{report.output}`",
        "",
        "## 执行记录",
    ]
    lines.extend(f"- {message}" for message in report.messages)
    lines.extend(["", "## 处理的源码文件"])
    lines.extend(f"- `{item}`" for item in report.processed_files)
    lines.extend(["", "## 正本固件 SHA-256"])
    lines.extend(f"- `{name}`: `{digest}`" for name, digest in report.firmware_hashes.items())
    lines.extend(["", "## 风险提示"])
    lines.extend(f"- {item}" for item in report.risks) if report.risks else lines.append("- 未发现本规则覆盖的风险信号。")
    report_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return report_path


def mark_failed_output(output: Path, error: str) -> None:
    if output.is_dir():
        (output / ".RELEASE_FAILED").write_text(error + "\n", encoding="utf-8")


def publish(args: argparse.Namespace) -> int:
    source = Path(args.source).resolve()
    output = Path(args.output).resolve()
    report = Report(source=source, output=output)
    try:
        if not source.is_dir():
            raise PublishError(f"source directory does not exist: {source}")
        if output.exists():
            raise PublishError(f"output directory already exists: {output}")
        build_original(source, args.keil, Path(args.project), args.target, report)
        copy_project(source, output)
        process_source_files(source, output, report)
        verify_unprocessed_files(source, output)
        verify_firmware_copy(output, report)
        report.risks = scan_risks(output)
        report.status = "SUCCESS"
        report.add("All copy, token, comment and firmware-hash checks passed.")
    except (OSError, PublishError) as error:
        report.add(f"Failure: {error}")
        mark_failed_output(output, str(error))
        write_report(report)
        print(f"Release copy failed: {error}", file=sys.stderr)
        return 1
    report_path = write_report(report)
    print(f"Release copy created: {output}")
    print(f"Report: {report_path}")
    return 0


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", default=str(PRODUCT_ROOT), help="original product directory")
    parser.add_argument(
        "--output",
        default=str(PRODUCT_ROOT.parent / f"{PRODUCT_ROOT.name}_RELEASE"),
        help="new release-copy directory; it must not already exist",
    )
    parser.add_argument("--keil", help="full path to UV4.exe; searched automatically when omitted")
    parser.add_argument("--project", default="MDK-ARM/STM32G431.uvprojx")
    parser.add_argument("--target", default="STM32G431")
    return parser.parse_args()


if __name__ == "__main__":
    sys.exit(publish(parse_args()))
