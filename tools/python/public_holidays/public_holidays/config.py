import os
import re
from dataclasses import dataclass
from pathlib import Path

import yaml

from .holiday import HolidayType


@dataclass
class OutputSource:
    name: str
    template: Path
    output: Path


@dataclass
class Input:
    metadata: Path
    generation_years: list[int]
    holiday_types: list[HolidayType]


@dataclass
class Twine:
    path: Path
    supported_languages: list[str]


@dataclass
class Output:
    data: Path
    sources: list[OutputSource]


@dataclass
class DataSource:
    name: str
    enabled: bool
    link: str
    supported_countries: list[str]


@dataclass
class Config:
    env: dict[str, str]
    input: Input
    twine: Twine
    output: Output
    data_sources: list[DataSource]
    duplicates_map: dict[str, list[str]]


def _resolve_vars(value: str, env: dict[str, str]) -> str:
    """Resolve ${VAR} references in a string using the provided env dict."""
    pattern = re.compile(r"\$\{(\w+)}")

    def replacer(match: re.Match) -> str:
        var = match.group(1)
        if var not in env:
            raise KeyError(f"Undefined variable: {var}")
        return env[var]

    # Resolve iteratively to handle chained references (${A} where A contains ${B})
    prev = None
    result = value
    while prev != result:
        prev = result
        result = pattern.sub(replacer, result)
    return result


def _resolve_env(raw_env: dict[str, str]) -> dict[str, str]:
    """Resolve all env variables, allowing forward and self references."""
    env: dict[str, str] = {}
    # Merge OS environment so that external vars can be used too
    combined = {**os.environ, **raw_env}
    for key, value in raw_env.items():
        env[key] = _resolve_vars(str(value), combined | env)
    return env


def _resolve_path(value: str, env: dict[str, str], config_dir: Path) -> Path:
    resolved = _resolve_vars(value, env)
    path = Path(resolved)
    if not path.is_absolute():
        path = (config_dir / path).resolve()
    return path


def _parse_output(raw: dict, env: dict[str, str], config_dir: Path) -> Output:
    sources: list[OutputSource] = []
    for entry in raw.get("sources", []):
        for name, props in entry.items():
            sources.append(
                OutputSource(
                    name=name,
                    template=_resolve_path(props["template"], env, config_dir),
                    output=_resolve_path(props["output"], env, config_dir),
                )
            )
    return Output(
        data=_resolve_path(raw["data"], env, config_dir),
        sources=sources,
    )


def _parse_input(raw: dict, env: dict[str, str], config_dir: Path) -> Input:
    return Input(
        metadata=_resolve_path(raw["metadata"], env, config_dir),
        generation_years=raw["generation-years"],
        holiday_types=[HolidayType[ht] for ht in raw["holiday-types"]],
    )


def _parse_twine(raw: dict, env: dict[str, str], config_dir: Path) -> Twine:
    return Twine(
        path=_resolve_path(raw["path"], env, config_dir),
        supported_languages=raw["supported-languages"],
    )


def _parse_data_sources(raw: list) -> list[DataSource]:
    result: list[DataSource] = []
    for entry in raw:
        for name, props in entry.items():
            result.append(
                DataSource(
                    name=name,
                    enabled=props.get("enabled", True),
                    link=props["link"],
                    supported_countries=list(props.get("supported-countries", [])),
                )
            )
    return result


def parse_config(config_path: str | Path) -> Config:
    config_path = Path(config_path).resolve()
    config_dir = config_path.parent

    with config_path.open() as f:
        raw = yaml.safe_load(f)

    env = _resolve_env(raw.get("env", {}))
    input_ = _parse_input(raw["input"], env, config_dir)
    twine = _parse_twine(raw["twine"], env, config_dir)
    output = _parse_output(raw["output"], env, config_dir)
    data_sources = _parse_data_sources(raw.get("data-sources", []))

    duplicates_dict = raw.get("duplicates-map", {})
    duplicates_map = {}
    for key, value in duplicates_dict.items():
        for item in value:
            duplicates_map[item] = key

    return Config(env=env, input=input_, twine=twine, output=output, data_sources=data_sources,
                  duplicates_map=duplicates_map)
