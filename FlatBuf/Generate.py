from __future__ import annotations

import argparse
import re
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


PACKET_SCHEMA = "Packet.fbs"
PACKET_TABLE = "Packet"
PAYLOAD_UNION = "PayloadType"
PAYLOAD_FIELD = "payload"
SCHEMA_INPUT_DIR = "In"
GENERATED_OUTPUT_DIR = "Out"

TABLE_RE = re.compile(r"^\s*table\s+([A-Za-z_][A-Za-z0-9_]*)\b", re.MULTILINE)
NAMESPACE_RE = re.compile(r"\bnamespace\s+([A-Za-z_][A-Za-z0-9_.]*)\s*;")


@dataclass(frozen=True)
class SchemaInfo:
    path: Path
    include_path: str
    tables: tuple[str, ...]


class FlatbufferGenerator:
    def __init__(self, root_dir: Path) -> None:
        self.root = root_dir
        self.input_dir = root_dir / SCHEMA_INPUT_DIR
        self.packet_schema_path = self.input_dir / PACKET_SCHEMA
        self.output_dir = root_dir / GENERATED_OUTPUT_DIR
        self.flatc = root_dir / "Flatc.exe"

    def run(self) -> int:
        self.validate()

        schemas = self.read_schemas()
        self.write_packet_schema(schemas)
        generated_files = self.generate_csharp()
        self.validate_generated_outputs(generated_files)

        print(f"총 생성 파일 수: {len(generated_files)}")
        print("검증 통과")
        return 0

    def validate(self) -> None:
        missing = [
            str(path)
            for path in (self.flatc, self.input_dir, self.packet_schema_path)
            if not path.exists()
        ]

        if missing:
            raise FileNotFoundError("다음 경로를 찾을 수 없습니다.\n" + "\n".join(missing))

    def read_schemas(self) -> list[SchemaInfo]:
        schemas = [
            SchemaInfo(
                path=schema_path,
                include_path=schema_path.relative_to(self.input_dir).as_posix(),
                tables=tuple(parse_tables(schema_path)),
            )
            for schema_path in self.find_schemas()
        ]

        if not schemas:
            raise RuntimeError(f"{self.input_dir}에 {PACKET_SCHEMA} 외의 .fbs 스키마가 없습니다.")

        return schemas

    def find_schemas(self) -> list[Path]:
        return sorted(
            path
            for path in self.input_dir.rglob("*.fbs")
            if path.name.lower() != PACKET_SCHEMA.lower()
        )

    def write_packet_schema(self, schemas: list[SchemaInfo]) -> None:
        namespace = read_packet_namespace(self.packet_schema_path)
        write_text_crlf(self.packet_schema_path, build_packet_schema(schemas, namespace))

    def generate_csharp(self) -> list[Path]:
        self.reset_output_dir()

        command = [
            str(self.flatc),
            "--csharp",
            "--gen-onefile",
            "--filename-suffix",
            ".generated",
            "-I",
            str(self.input_dir),
            "-o",
            str(self.output_dir),
        ]
        command.extend(str(path) for path in self.find_input_files())

        self.run_command(command)
        files = sorted(self.output_dir.rglob("*.cs"))
        self.print_files(files)
        return files

    def find_input_files(self) -> list[Path]:
        return sorted(self.input_dir.rglob("*.fbs"))

    def reset_output_dir(self) -> None:
        if self.output_dir.exists():
            if self.output_dir.parent != self.root:
                raise RuntimeError(f"삭제 경로가 올바르지 않습니다: {self.output_dir}")
            shutil.rmtree(self.output_dir)

        self.output_dir.mkdir(parents=True, exist_ok=True)

    def run_command(self, command: list[str]) -> None:
        print("[csharp] 생성 시작")
        print("  " + " ".join(command))

        result = subprocess.run(
            command,
            cwd=self.root,
            capture_output=True,
            text=True,
            encoding="utf-8",
        )

        if result.stdout.strip():
            print(result.stdout.strip())

        if result.returncode != 0:
            if result.stderr.strip():
                print(result.stderr.strip(), file=sys.stderr)
            raise RuntimeError(f"C# 생성 실패 (exit code: {result.returncode})")

    def print_files(self, files: list[Path]) -> None:
        print(f"[csharp] 생성 완료: {len(files)}개 파일")
        for path in files:
            print(f"  - {path.relative_to(self.root)}")

    def validate_generated_outputs(self, generated_files: list[Path]) -> None:
        if not generated_files:
            raise RuntimeError("생성된 C# 파일이 없습니다.")

        expected_names = {
            f"{path.stem}.generated.cs"
            for path in self.find_input_files()
        }
        generated_names = {path.name for path in generated_files}
        missing = expected_names - generated_names
        if missing:
            raise RuntimeError("예상한 생성 파일이 없습니다: " + ", ".join(sorted(missing)))

        unexpected = [
            path
            for path in self.output_dir.rglob("*")
            if path.is_file() and path.suffix.lower() != ".cs"
        ]
        if unexpected:
            raise RuntimeError(
                "C# 외 생성물이 남아 있습니다: "
                + ", ".join(str(path.relative_to(self.root)) for path in unexpected)
            )


def parse_tables(schema_path: Path) -> list[str]:
    return TABLE_RE.findall(schema_path.read_text(encoding="utf-8"))


def read_packet_namespace(packet_schema_path: Path) -> str | None:
    text = packet_schema_path.read_text(encoding="utf-8")
    match = NAMESPACE_RE.search(text)
    if match is None:
        return None

    return match.group(1)


def build_packet_schema(schemas: list[SchemaInfo], namespace: str | None) -> str:
    tables = [table for schema in schemas for table in schema.tables]
    if not tables:
        raise RuntimeError("Packet PayloadType에 포함할 table 선언이 없습니다.")

    lines: list[str] = []
    lines.extend(f'include "{schema.include_path}";' for schema in schemas)
    lines.append("")

    if namespace:
        lines.extend([f"namespace {namespace};", ""])

    lines.extend(
        [
            f"union {PAYLOAD_UNION}",
            "{",
        ]
    )

    for index, table in enumerate(tables):
        suffix = "," if index < len(tables) - 1 else ""
        lines.append(f"    {table}{suffix}")

    lines.extend(
        [
            "}",
            "",
            f"table {PACKET_TABLE}",
            "{",
            f"    {PAYLOAD_FIELD}:{PAYLOAD_UNION};",
            "}",
            "",
            f"root_type {PACKET_TABLE};",
            "",
        ]
    )
    return "\n".join(lines)


def write_text_crlf(path: Path, text: str) -> None:
    normalized = re.sub(r"\r+\n", "\n", text).replace("\r", "\n")
    with path.open("w", encoding="utf-8", newline="\r\n") as file:
        file.write(normalized)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="FlatBuf/In의 .fbs를 자동 탐색해 FlatBuf/Out에 C# FlatBuffers 코드를 생성합니다."
    )
    parser.add_argument(
        "--root-dir",
        default=Path(__file__).resolve().parent,
        type=Path,
        help="FlatBuf 작업 루트 경로",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    try:
        return FlatbufferGenerator(args.root_dir.resolve()).run()
    except Exception as error:
        print(f"오류: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
