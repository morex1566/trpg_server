from __future__ import annotations

import argparse
import re
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Callable


PACKET_SCHEMA = "packet.fbs"
PACKET_TABLE = "packet"
PAYLOAD_UNION = "payload_type"
PAYLOAD_FIELD = "payload"
NAMESPACE = "net.protocol"
FLATBUF_DIR = "flatbuf"
SCHEMA_INPUT_DIR = "in"
GENERATED_OUTPUT_DIR = "out"
SCHEMA_WORK_DIR = "schema_work"
RUNTIME_CPP_GENERATED_DIR = Path("runtime") / "src" / "net.core" / "gen"

TABLE_RE = re.compile(r"^\s*table\s+([A-Za-z_][A-Za-z0-9_]*)\b", re.MULTILINE)
TYPE_RE = re.compile(r"^\s*(table|struct|enum|union)\s+([A-Za-z_][A-Za-z0-9_]*)\b", re.MULTILINE)
FIELD_RE = re.compile(r"^(\s*)([A-Za-z_][A-Za-z0-9_]*)(\s*:)", re.MULTILINE)
ENUM_VALUE_RE = re.compile(r"^(\s*)([A-Za-z_][A-Za-z0-9_]*)(\s*(?:=|,|$))", re.MULTILINE)
INCLUDE_RE = re.compile(r'include\s+"([^"]+)";')
NAMESPACE_RE = re.compile(r"\bnamespace\s+([A-Za-z_][A-Za-z0-9_.]*)\s*;")
ENUM_RE = re.compile(r"(enum\s+[A-Za-z_][A-Za-z0-9_]*[^{]*\{)(.*?)(\n\})", re.DOTALL)
UNION_RE = re.compile(r"(union\s+[A-Za-z_][A-Za-z0-9_]*\s*\{)(.*?)(\n\})", re.DOTALL)
TYPE_REF_RE = re.compile(r"(:\s*)([A-Za-z_][A-Za-z0-9_]*)(\s*(?:[;=]|$))", re.MULTILINE)
ROOT_TYPE_RE = re.compile(r"(\broot_type\s+)([A-Za-z_][A-Za-z0-9_]*)(\s*;)")


@dataclass(frozen=True)
class SchemaInfo:
    path: Path
    tables: tuple[str, ...]


@dataclass(frozen=True)
class LanguageProfile:
    name: str
    flag: str
    output_dir: str
    extension: str
    extra_flags: tuple[str, ...] = ()
    filename: Callable[[str], str] = lambda value: value
    type_name: Callable[[str], str] = lambda value: value
    field_name: Callable[[str], str] = lambda value: value
    enum_value: Callable[[str], str] = lambda value: value
    namespace: str = NAMESPACE


def snake_to_pascal(value: str) -> str:
    return "".join(part[:1].upper() + part[1:] for part in value.split("_") if part)


def snake_to_camel(value: str) -> str:
    pascal = snake_to_pascal(value)
    return pascal[:1].lower() + pascal[1:]


def pascal_file_name(file_name: str) -> str:
    return f"{snake_to_pascal(Path(file_name).stem)}.fbs"


PROFILES = (
    LanguageProfile(
        name="cpp",
        flag="--cpp",
        output_dir="cpp",
        extension=".h",
    ),
    LanguageProfile(
        name="csharp",
        flag="--csharp",
        output_dir="csharp",
        extension=".cs",
        extra_flags=("--gen-onefile",),
        filename=pascal_file_name,
        type_name=snake_to_pascal,
        field_name=snake_to_camel,
        enum_value=snake_to_pascal,
        namespace="Net.Protocol",
    ),
)


class FlatbufferGenerator:
    def __init__(self, root_dir: Path) -> None:
        self.root = root_dir
        self.flatbuf_dir = root_dir / FLATBUF_DIR
        self.input_dir = self.flatbuf_dir / SCHEMA_INPUT_DIR
        self.packet_schema_path = self.input_dir / PACKET_SCHEMA
        self.output_dir = self.flatbuf_dir / GENERATED_OUTPUT_DIR
        self.schema_work_dir = self.output_dir / SCHEMA_WORK_DIR
        self.runtime_cpp_generated_dir = root_dir / RUNTIME_CPP_GENERATED_DIR
        self.flatc = root_dir / "flatc.exe"

    def run(self) -> int:
        self.validate()
        self.remove_unneeded_outputs()

        schemas = self.read_schemas()
        self.write_packet_input_schema(schemas)

        generated_files: dict[str, list[Path]] = {}
        for profile in PROFILES:
            generated_files[profile.name] = self.generate(profile, schemas)

        self.sync_runtime_cpp_headers(generated_files["cpp"])
        self.validate_generated_outputs(generated_files)

        total = sum(len(files) for files in generated_files.values())
        print(f"총 생성 파일 수: {total}")
        print("검증 통과")
        return 0

    def validate(self) -> None:
        missing = [
            str(path)
            for path in (self.flatc, self.input_dir)
            if not path.exists()
        ]

        if missing:
            raise FileNotFoundError("다음 경로를 찾을 수 없습니다.\n" + "\n".join(missing))

    def remove_unneeded_outputs(self) -> None:
        allowed_output_dirs = {profile.output_dir for profile in PROFILES}
        allowed_output_dirs.add(SCHEMA_WORK_DIR)
        self.remove_unexpected_children(self.output_dir, allowed_output_dirs)

        allowed_schema_dirs = {profile.name for profile in PROFILES}
        self.remove_unexpected_children(self.schema_work_dir, allowed_schema_dirs)

    def remove_unexpected_children(self, parent: Path, allowed_names: set[str]) -> None:
        if not parent.exists():
            return

        for path in parent.iterdir():
            if path.name in allowed_names:
                continue

            self.remove_generated_path(path, parent)

    def remove_generated_path(self, path: Path, expected_parent: Path) -> None:
        if path.parent != expected_parent:
            raise RuntimeError(f"삭제 경로가 올바르지 않습니다: {path}")

        if path.is_dir():
            shutil.rmtree(path)
            return

        path.unlink()

    def find_schemas(self) -> list[Path]:
        return sorted(
            path
            for path in self.input_dir.glob("*.fbs")
            if path.name.lower() != PACKET_SCHEMA
        )

    def read_schemas(self) -> list[SchemaInfo]:
        schemas = [
            SchemaInfo(
                path=schema_path,
                tables=tuple(parse_tables(schema_path)),
            )
            for schema_path in self.find_schemas()
        ]

        if not schemas:
            raise RuntimeError(f"{self.input_dir}에 {PACKET_SCHEMA} 외의 .fbs 스키마가 없습니다.")

        empty_schemas = [schema.path.name for schema in schemas if not schema.tables]
        if empty_schemas:
            raise RuntimeError("table 선언이 없는 스키마가 있습니다: " + ", ".join(empty_schemas))

        return schemas

    def write_packet_input_schema(self, schemas: list[SchemaInfo]) -> None:
        write_text_crlf(self.packet_schema_path, build_packet_schema(schemas))

    def generate(self, profile: LanguageProfile, schemas: list[SchemaInfo]) -> list[Path]:
        schema_dir = self.schema_work_dir / profile.name
        out_dir = self.output_dir / profile.output_dir
        self.reset_dir(schema_dir, self.schema_work_dir)
        self.reset_dir(out_dir, self.output_dir)

        self.write_language_schemas(profile, schemas, schema_dir)

        command = [
            str(self.flatc),
            profile.flag,
            *profile.extra_flags,
            "--filename-suffix",
            ".generated",
            "-I",
            str(schema_dir),
            "-o",
            str(out_dir),
        ]

        command.extend(str(path) for path in sorted(schema_dir.glob("*.fbs")))

        self.run_command(profile.name, command, self.root)
        files = sorted(out_dir.rglob(f"*{profile.extension}"))
        self.print_files(profile.name, files)
        return files

    def write_language_schemas(
        self,
        profile: LanguageProfile,
        schemas: list[SchemaInfo],
        schema_dir: Path,
    ) -> None:
        input_files = [schema.path for schema in schemas]

        type_names = collect_type_names(input_files) | {PACKET_TABLE, PAYLOAD_UNION}
        include_map = {path.name: profile.filename(path.name) for path in input_files}
        type_map = {name: profile.type_name(name) for name in type_names}

        for source_path in input_files:
            converted = convert_schema(
                source_path.read_text(encoding="utf-8"),
                include_map=include_map,
                type_map=type_map,
                field_name=profile.field_name,
                enum_value=profile.enum_value,
                namespace=profile.namespace,
            )
            write_text_crlf(schema_dir / profile.filename(source_path.name), converted)

        packet_schema = build_packet_schema(schemas)
        converted_packet = convert_schema(
            packet_schema,
            include_map=include_map,
            type_map=type_map,
            field_name=profile.field_name,
            enum_value=profile.enum_value,
            namespace=profile.namespace,
        )
        write_text_crlf(schema_dir / profile.filename(PACKET_SCHEMA), converted_packet)

    def reset_dir(self, path: Path, expected_parent: Path, recreate: bool = True) -> None:
        if path.exists():
            if path.parent != expected_parent:
                raise RuntimeError(f"삭제 경로가 올바르지 않습니다: {path}")
            shutil.rmtree(path)

        if recreate:
            path.mkdir(parents=True, exist_ok=True)

    def run_command(
        self,
        language: str,
        command: list[str],
        cwd: Path,
        label: str = "생성",
    ) -> None:
        print(f"[{language}] {label} 시작")
        print("  " + " ".join(command))

        result = subprocess.run(
            command,
            cwd=cwd,
            capture_output=True,
            text=True,
            encoding="utf-8",
        )

        if result.stdout.strip():
            print(result.stdout.strip())

        if result.returncode != 0:
            if result.stderr.strip():
                print(result.stderr.strip(), file=sys.stderr)
            raise RuntimeError(f"{language} {label} 실패 (exit code: {result.returncode})")

    def print_files(self, language: str, files: list[Path]) -> None:
        print(f"[{language}] 생성 완료: {len(files)}개 파일")
        for path in files:
            print(f"  - {path.relative_to(self.root)}")

    def sync_runtime_cpp_headers(self, generated_headers: list[Path]) -> None:
        self.runtime_cpp_generated_dir.mkdir(parents=True, exist_ok=True)

        expected_names = {path.name for path in generated_headers}
        for path in self.runtime_cpp_generated_dir.glob("*.generated.h"):
            if path.name not in expected_names:
                self.remove_generated_path(path, self.runtime_cpp_generated_dir)

        for source_path in generated_headers:
            destination_path = self.runtime_cpp_generated_dir / source_path.name
            write_text_crlf(destination_path, source_path.read_text(encoding="utf-8"))

        print(f"[cpp] 런타임 헤더 동기화 완료: {len(generated_headers)}개 파일")
        for path in generated_headers:
            print(f"  - {(self.runtime_cpp_generated_dir / path.name).relative_to(self.root)}")

    def validate_generated_outputs(self, generated_files: dict[str, list[Path]]) -> None:
        self.validate_expected_children(
            self.output_dir,
            {profile.output_dir for profile in PROFILES} | {SCHEMA_WORK_DIR},
        )
        self.validate_expected_children(
            self.schema_work_dir,
            {profile.name for profile in PROFILES},
        )

        cpp_headers = generated_files["cpp"]
        self.validate_expected_children(
            self.runtime_cpp_generated_dir,
            {path.name for path in cpp_headers},
            pattern="*.generated.h",
        )

        scanned_paths = [
            self.packet_schema_path,
            self.output_dir / "schema_work" / "cpp" / PACKET_SCHEMA,
            self.output_dir / "schema_work" / "csharp" / pascal_file_name(PACKET_SCHEMA),
            self.output_dir / "cpp" / "packet.generated.h",
            self.output_dir / "csharp" / "Packet.generated.cs",
            self.runtime_cpp_generated_dir / "packet.generated.h",
        ]
        unexpected_tokens = ("connection_id", "connectionId", "ConnectionId")
        for path in scanned_paths:
            if not path.exists():
                raise RuntimeError(f"검증 대상 파일을 찾을 수 없습니다: {path}")

            text = path.read_text(encoding="utf-8")
            for token in unexpected_tokens:
                if token in text:
                    raise RuntimeError(f"불필요한 필드가 생성되었습니다: {path} ({token})")

    def validate_expected_children(
        self,
        parent: Path,
        allowed_names: set[str],
        pattern: str = "*",
    ) -> None:
        if not parent.exists():
            raise RuntimeError(f"검증 대상 디렉터리를 찾을 수 없습니다: {parent}")

        unexpected = [
            path.name
            for path in parent.glob(pattern)
            if path.name not in allowed_names
        ]
        if unexpected:
            raise RuntimeError(
                f"불필요한 생성물이 남아 있습니다: {parent} ({', '.join(sorted(unexpected))})"
            )


def parse_tables(schema_path: Path) -> list[str]:
    return TABLE_RE.findall(schema_path.read_text(encoding="utf-8"))


def collect_type_names(paths: list[Path]) -> set[str]:
    type_names: set[str] = set()
    for path in paths:
        text = path.read_text(encoding="utf-8")
        type_names.update(match.group(2) for match in TYPE_RE.finditer(text))
    return type_names


def build_packet_schema(schemas: list[SchemaInfo]) -> str:
    tables = [table for schema in schemas for table in schema.tables]
    if not tables:
        raise RuntimeError("packet union에 포함할 table 선언이 없습니다.")

    lines: list[str] = []
    lines.extend(f'include "{schema.path.name}";' for schema in schemas)
    lines.extend(
        [
            "",
            f"namespace {NAMESPACE};",
            "",
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


def convert_schema(
    text: str,
    include_map: dict[str, str],
    type_map: dict[str, str],
    field_name: Callable[[str], str],
    enum_value: Callable[[str], str],
    namespace: str,
) -> str:
    text = INCLUDE_RE.sub(lambda match: f'include "{include_map.get(match.group(1), match.group(1))}";', text)
    text = NAMESPACE_RE.sub(f"namespace {namespace};", text)
    text = convert_enum_values(text, enum_value)
    text = convert_union_members(text, type_map)
    text = FIELD_RE.sub(lambda match: f"{match.group(1)}{field_name(match.group(2))}{match.group(3)}", text)
    text = replace_type_declarations(text, type_map)
    text = TYPE_REF_RE.sub(lambda match: f"{match.group(1)}{type_map.get(match.group(2), match.group(2))}{match.group(3)}", text)
    text = ROOT_TYPE_RE.sub(lambda match: f"{match.group(1)}{type_map.get(match.group(2), match.group(2))}{match.group(3)}", text)
    return text


def replace_type_declarations(text: str, type_map: dict[str, str]) -> str:
    def replace(match: re.Match[str]) -> str:
        keyword = match.group(1)
        old_name = match.group(2)
        return f"{keyword} {type_map.get(old_name, old_name)}"

    return TYPE_RE.sub(replace, text)


def convert_enum_values(text: str, enum_value: Callable[[str], str]) -> str:
    def replace_enum(match: re.Match[str]) -> str:
        body = ENUM_VALUE_RE.sub(
            lambda value: f"{value.group(1)}{enum_value(value.group(2))}{value.group(3)}",
            match.group(2),
        )
        return f"{match.group(1)}{body}{match.group(3)}"

    return ENUM_RE.sub(replace_enum, text)


def convert_union_members(text: str, type_map: dict[str, str]) -> str:
    def replace_union(match: re.Match[str]) -> str:
        converted_lines = []

        for line in match.group(2).splitlines():
            stripped = line.strip().rstrip(",")
            if not stripped:
                converted_lines.append(line)
                continue

            indent = line[: len(line) - len(line.lstrip())]
            suffix = "," if line.strip().endswith(",") else ""
            converted_lines.append(f"{indent}{type_map.get(stripped, stripped)}{suffix}")

        return f"{match.group(1)}\n" + "\n".join(converted_lines) + match.group(3)

    return UNION_RE.sub(replace_union, text)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="flatbuf/in의 .fbs를 자동 탐색해 flatbuf/out에 언어별 FlatBuffers 코드를 생성합니다."
    )
    parser.add_argument(
        "--root-dir",
        default=Path(__file__).resolve().parent,
        type=Path,
        help="flatbuf 작업 루트 경로",
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
