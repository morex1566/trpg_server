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
NAMESPACE = "net.protocol"
FLATBUF_DIR = "flatbuf"
SCHEMA_INPUT_DIR = "in"
GENERATED_OUTPUT_DIR = "out"

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
TS_EXPORT_RE = re.compile(r"export\s+\{\s*([^}]+?)\s*\}\s+from\s+'\.\/protocol\/([^']+)\.js';")
TS_IMPORT_RE = re.compile(r"^\s*import\s+\{\s*([^}]+?)\s*\}\s+from\s+'([^']+)';\s*$")
TS_FLATBUFFERS_IMPORT_RE = re.compile(r"^\s*import\s+\*\s+as\s+flatbuffers\s+from\s+'flatbuffers';\s*$")


@dataclass(frozen=True)
class SchemaInfo:
    path: Path
    tables: tuple[str, ...]
    types: tuple[str, ...]


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
    LanguageProfile(
        name="typescript",
        flag="--ts",
        output_dir="typescript_modules",
        extension=".ts",
        extra_flags=("--gen-all",),
    ),
)


class FlatbufferGenerator:
    def __init__(self, root_dir: Path) -> None:
        self.root = root_dir
        self.workspace = root_dir.parent
        self.flatbuf_dir = root_dir / FLATBUF_DIR
        self.input_dir = self.flatbuf_dir / SCHEMA_INPUT_DIR
        self.output_dir = self.flatbuf_dir / GENERATED_OUTPUT_DIR
        self.schema_work_dir = self.output_dir / "schema_work"
        self.flatc = root_dir / "flatc.exe"

    def run(self) -> int:
        self.validate()

        schemas = self.read_schemas()
        self.write_packet_schema(schemas)

        total = 0
        for profile in PROFILES:
            if profile.name == "typescript":
                continue
            total += len(self.generate(profile, schemas))

        total += len(self.generate_typescript(schemas))
        total += len(self.compile_javascript())
        print(f"총 생성 파일 수: {total}")
        return 0

    def validate(self) -> None:
        missing = [
            str(path)
            for path in (self.flatc, self.input_dir)
            if not path.exists()
        ]

        if missing:
            raise FileNotFoundError("다음 경로를 찾을 수 없습니다.\n" + "\n".join(missing))

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
                types=tuple(parse_type_names(schema_path)),
            )
            for schema_path in self.find_schemas()
        ]

        empty_schemas = [schema.path.name for schema in schemas if not schema.tables]
        if empty_schemas:
            raise RuntimeError("table 선언이 없는 스키마가 있습니다: " + ", ".join(empty_schemas))

        return schemas

    def write_packet_schema(self, schemas: list[SchemaInfo]) -> None:
        packet_path = self.input_dir / PACKET_SCHEMA
        packet_path.write_text(build_packet_schema(schemas), encoding="utf-8")

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

        if profile.name == "typescript":
            command.append(str(schema_dir / profile.filename(PACKET_SCHEMA)))
        else:
            command.extend(str(path) for path in sorted(schema_dir.glob("*.fbs")))

        self.run_command(profile.name, command, self.root)
        files = sorted(out_dir.rglob(f"*{profile.extension}"))
        self.print_files(profile.name, files)
        return files

    def generate_typescript(self, schemas: list[SchemaInfo]) -> list[Path]:
        profile = next(profile for profile in PROFILES if profile.name == "typescript")
        schema_dir = self.schema_work_dir / profile.name
        module_dir = self.schema_work_dir / "typescript_modules"
        out_dir = self.output_dir / "typescript"
        self.reset_dir(schema_dir, self.schema_work_dir)
        self.reset_dir(module_dir, self.schema_work_dir)
        self.reset_dir(out_dir, self.output_dir)

        self.write_language_schemas(profile, schemas, schema_dir)
        self.run_command(
            profile.name,
            [
                str(self.flatc),
                profile.flag,
                *profile.extra_flags,
                "-I",
                str(schema_dir),
                "-o",
                str(module_dir),
                str(schema_dir / PACKET_SCHEMA),
            ],
            self.root,
        )

        files = self.bundle_typescript_modules(schemas, module_dir, out_dir)
        self.reset_dir(module_dir, self.schema_work_dir, recreate=False)
        self.print_files(profile.name, files)
        return files

    def write_language_schemas(
        self,
        profile: LanguageProfile,
        schemas: list[SchemaInfo],
        schema_dir: Path,
    ) -> None:
        input_files = [schema.path for schema in schemas]
        input_files.append(self.input_dir / PACKET_SCHEMA)

        type_names = collect_type_names(input_files)
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
            (schema_dir / profile.filename(source_path.name)).write_text(
                converted,
                encoding="utf-8",
            )

    def compile_javascript(self) -> list[Path]:
        ts_dir = self.output_dir / "typescript"
        js_dir = self.output_dir / "javascript"
        self.reset_dir(js_dir, self.output_dir)

        compile_dir = self.prepare_typescript_compile_dir(ts_dir)
        try:
            self.run_command(
                "javascript",
                self.build_tsc_command(compile_dir, js_dir),
                self.node_project_dir(),
                "TypeScript -> JavaScript 변환",
            )
        finally:
            if compile_dir != ts_dir:
                self.reset_dir(compile_dir, self.node_project_dir(), recreate=False)

        files = sorted(js_dir.rglob("*.js"))
        self.print_files("javascript", files)
        return files

    def bundle_typescript_modules(
        self,
        schemas: list[SchemaInfo],
        module_dir: Path,
        out_dir: Path,
    ) -> list[Path]:
        export_map = read_typescript_export_map(module_dir / "net" / "protocol.ts")
        bundle_map = self.build_typescript_bundle_map(schemas)
        generated_files = []

        for bundle_name, symbols in bundle_map.items():
            output_path = out_dir / f"{bundle_name}.generated.ts"
            output_path.write_text(
                build_typescript_bundle(
                    bundle_name=bundle_name,
                    symbols=symbols,
                    export_map=export_map,
                    module_dir=module_dir,
                    bundle_map=bundle_map,
                ),
                encoding="utf-8",
            )
            generated_files.append(output_path)

        return generated_files

    def build_typescript_bundle_map(self, schemas: list[SchemaInfo]) -> dict[str, list[str]]:
        bundle_map = {
            schema.path.stem: list(schema.types)
            for schema in schemas
        }
        bundle_map[Path(PACKET_SCHEMA).stem] = parse_type_names(self.input_dir / PACKET_SCHEMA)
        return bundle_map

    def prepare_typescript_compile_dir(self, ts_dir: Path) -> Path:
        node_project = self.node_project_dir()
        if node_project == self.root:
            return ts_dir

        compile_dir = node_project / ".flatbuffer_generated_ts"
        self.reset_dir(compile_dir, node_project, recreate=False)
        shutil.copytree(ts_dir, compile_dir)
        return compile_dir

    def node_project_dir(self) -> Path:
        root_node_modules = self.root / "node_modules" / "flatbuffers"
        if root_node_modules.exists():
            return self.root

        for package_json in sorted(self.workspace.glob("**/node_modules/flatbuffers/package.json")):
            return package_json.parents[2]

        return self.root

    def build_tsc_command(self, ts_dir: Path, js_dir: Path) -> list[str]:
        local_tsc = self.node_project_dir() / "node_modules" / ".bin" / "tsc.cmd"
        tsc = str(local_tsc) if local_tsc.exists() else shutil.which("tsc")
        command = [tsc] if tsc else self.npx_tsc_command()
        command.extend(
            [
                "--target",
                "ES2020",
                "--module",
                "node16",
                "--moduleResolution",
                "node16",
                "--esModuleInterop",
                "--skipLibCheck",
                "--rootDir",
                str(ts_dir),
                "--outDir",
                str(js_dir),
                "--declaration",
                "false",
                "--sourceMap",
                "false",
                *(str(path) for path in sorted(ts_dir.rglob("*.ts"))),
            ]
        )
        return command

    def npx_tsc_command(self) -> list[str]:
        npx = shutil.which("npx")
        if npx is None:
            raise FileNotFoundError("TypeScript 컴파일러를 찾을 수 없습니다. tsc 또는 npx가 필요합니다.")
        return [npx, "tsc"]

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


def parse_tables(schema_path: Path) -> list[str]:
    return TABLE_RE.findall(schema_path.read_text(encoding="utf-8"))


def parse_type_names(schema_path: Path) -> list[str]:
    text = schema_path.read_text(encoding="utf-8")
    return [match.group(2) for match in TYPE_RE.finditer(text)]


def collect_type_names(paths: list[Path]) -> set[str]:
    type_names: set[str] = set()
    for path in paths:
        text = path.read_text(encoding="utf-8")
        type_names.update(match.group(2) for match in TYPE_RE.finditer(text))
    return type_names


def build_packet_schema(schemas: list[SchemaInfo]) -> str:
    lines: list[str] = []

    for schema in schemas:
        lines.append(f'include "{schema.path.name}";')

    lines.extend(
        [
            "",
            f"namespace {NAMESPACE};",
            "",
            "enum packet_type : ushort",
            "{",
            "    none = 0,",
        ]
    )

    for schema_index, schema in enumerate(schemas, start=1):
        base_id = schema_index * 10000
        lines.extend(["", f"    // {schema.path.name} ({base_id}~{base_id + 9999})"])

        for table_index, table_name in enumerate(schema.tables):
            lines.append(f"    {table_name} = {base_id + table_index},")

    lines.extend(
        [
            "}",
            "",
            "struct packet_header",
            "{",
            "    connection_id:ulong;",
            "    type:packet_type;",
            "    payload_size:ushort;",
            "}",
            "",
            "union packet_payload",
            "{",
        ]
    )

    payload_tables = [table for schema in schemas for table in schema.tables]
    for index, table_name in enumerate(payload_tables):
        suffix = "," if index < len(payload_tables) - 1 else ""
        lines.append(f"    {table_name}{suffix}")

    lines.extend(
        [
            "}",
            "",
            "table packet",
            "{",
            "    header:packet_header;",
            "    payload:packet_payload;",
            "}",
            "",
            "root_type packet;",
            "",
        ]
    )
    return "\n".join(lines)


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


def read_typescript_export_map(index_path: Path) -> dict[str, Path]:
    export_map = {}
    base_dir = index_path.parent / "protocol"

    for match in TS_EXPORT_RE.finditer(index_path.read_text(encoding="utf-8")):
        symbol = match.group(1).strip()
        module_name = match.group(2)
        export_map[symbol] = base_dir / f"{module_name}.ts"

    return export_map


def build_typescript_bundle(
    bundle_name: str,
    symbols: list[str],
    export_map: dict[str, Path],
    module_dir: Path,
    bundle_map: dict[str, list[str]],
) -> str:
    symbol_to_bundle = {
        symbol: current_bundle
        for current_bundle, current_symbols in bundle_map.items()
        for symbol in current_symbols
    }
    module_to_bundle = {
        export_map[symbol].stem: symbol_to_bundle[symbol]
        for symbol in export_map
        if symbol in symbol_to_bundle
    }
    imports: dict[str, set[str]] = {}
    chunks = []
    needs_flatbuffers = False

    for symbol in symbols:
        source_path = export_map.get(symbol)
        if source_path is None:
            raise RuntimeError(f"TypeScript 생성 파일을 찾을 수 없습니다: {symbol}")

        chunk, chunk_imports, uses_flatbuffers = transform_typescript_module(
            source_path=source_path,
            current_bundle=bundle_name,
            module_to_bundle=module_to_bundle,
            symbol_to_bundle=symbol_to_bundle,
        )
        chunks.append(chunk)
        needs_flatbuffers = needs_flatbuffers or uses_flatbuffers

        for import_bundle, import_symbols in chunk_imports.items():
            imports.setdefault(import_bundle, set()).update(import_symbols)

    lines = [
        "// automatically generated by generate.py, do not modify",
        "",
        "/* eslint-disable @typescript-eslint/no-unused-vars, @typescript-eslint/no-explicit-any, @typescript-eslint/no-non-null-assertion */",
        "",
    ]

    if needs_flatbuffers:
        lines.extend(["import * as flatbuffers from 'flatbuffers';", ""])

    for import_bundle in sorted(imports):
        import_symbols = ", ".join(sorted(imports[import_bundle]))
        lines.append(f"import {{ {import_symbols} }} from './{import_bundle}.generated.js';")

    if imports:
        lines.append("")

    lines.extend(chunks)
    return "\n\n".join(section for section in lines if section != "") + "\n"


def transform_typescript_module(
    source_path: Path,
    current_bundle: str,
    module_to_bundle: dict[str, str],
    symbol_to_bundle: dict[str, str],
) -> tuple[str, dict[str, set[str]], bool]:
    external_imports: dict[str, set[str]] = {}
    kept_lines = []
    uses_flatbuffers = False

    for line in source_path.read_text(encoding="utf-8").splitlines():
        stripped = line.strip()

        if not stripped:
            kept_lines.append(line)
            continue

        if stripped.startswith("// automatically generated by"):
            continue

        if stripped.startswith("/* eslint-disable"):
            continue

        if TS_FLATBUFFERS_IMPORT_RE.match(line):
            uses_flatbuffers = True
            continue

        import_match = TS_IMPORT_RE.match(line)
        if import_match:
            import_path = import_match.group(2)
            import_module = Path(import_path).stem
            import_bundle = module_to_bundle.get(import_module)

            if import_bundle is None or import_bundle == current_bundle:
                continue

            imported_symbols = [
                symbol.strip()
                for symbol in import_match.group(1).split(",")
                if symbol.strip() in symbol_to_bundle
            ]
            external_imports.setdefault(import_bundle, set()).update(imported_symbols)
            continue

        kept_lines.append(line)

    while kept_lines and not kept_lines[0].strip():
        kept_lines.pop(0)

    while kept_lines and not kept_lines[-1].strip():
        kept_lines.pop()

    return "\n".join(kept_lines), external_imports, uses_flatbuffers


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
