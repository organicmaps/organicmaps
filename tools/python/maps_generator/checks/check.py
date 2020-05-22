import os
from abc import ABC
from collections import namedtuple
from enum import Enum
from typing import Any
from typing import Callable
from typing import List


ResLine = namedtuple("ResLine", ["previous", "current", "diff", "arrow"])


class Arrow(Enum):
    zero = 0
    down = 1
    up = 2


ROW_TO_STR = {
    Arrow.zero: "◄►",
    Arrow.down: "▼",
    Arrow.up: "▲",
}


class Check(ABC):
    def __init__(self, name: str):
        self.name = name
        self.op: Callable[
            [Any, Any], Any
        ] = lambda previous, current: current - previous
        self.do: Callable[[Any], Any] = lambda x: x
        self.zero: Any = 0
        self.diff_format: Callable[[Any], str] = lambda x: str(x)
        self.format: Callable[[Any], str] = lambda x: str(x)

    def set_op(self, op: Callable[[Any, Any], Any]):
        self.op = op

    def set_do(self, do: Callable[[Any], Any]):
        self.do = do

    def set_zero(self, zero: Any):
        self.zero = zero

    def set_diff_format(self, diff_format: Callable[[Any], str]):
        self.diff_format = diff_format

    def set_format(self, format: Callable[[Any], str]):
        self.format = format

    def check(self):
        pass

    def get_result(self) -> Any:
        pass

    def print(self, _print=print):
        _print(self.formatted_string())

    def formatted_string(self) -> str:
        pass


class CompareCheck(Check):
    def __init__(
        self, name: str, old: Any, new: Any,
    ):
        super().__init__(name)

        self.old = old
        self.new = new
        self.result = None

    def set_op(self, op: Callable[[Any, Any], Any]):
        self.op = op

    def set_do(self, do: Callable[[Any], Any]):
        self.do = do

    def set_zero(self, zero: Any):
        self.zero = zero

    def get_result(self) -> ResLine:
        return self.result

    def check(self):
        previous = self.do(self.old)
        if previous is None:
            return False

        current = self.do(self.new)
        if current is None:
            return False

        diff = self.op(previous, current)
        if diff is None:
            return False

        arrow = Arrow.zero
        if diff > self.zero:
            arrow = Arrow.up
        elif diff < self.zero:
            arrow = Arrow.down

        self.result = ResLine(
            previous=previous, current=current, diff=diff, arrow=arrow
        )
        return True

    def formatted_string(self) -> str:
        assert self.result
        return (
            f"{self.name}: {ROW_TO_STR[self.result.arrow]} "
            f"{self.diff_format(self.result.diff)} "
            f"[previous: {self.format(self.result.previous)}, "
            f"current: {self.format(self.result.current)}]"
        )


class CompareCheckSet(Check):
    def __init__(self, name: str):
        super().__init__(name)

        self.checks = []

    def add_check(self, check: Check):
        self.checks.append(check)

    def set_op(self, op: Callable[[Any, Any], Any]):
        for c in self.checks:
            c.set_op(op)

    def set_do(self, do: Callable[[Any], Any]):
        for c in self.checks:
            c.set_do(do)

    def set_zero(self, zero: Any):
        for c in self.checks:
            c.set_zero(zero)

    def set_diff_format(self, diff_format: Callable[[Any], str]):
        for c in self.checks:
            c.set_diff_format(diff_format)

    def set_format(self, format: Callable[[Any], str]):
        for c in self.checks:
            c.set_format(format)

    def check(self):
        for c in self.checks:
            c.check()

    def get_result(self,) -> List[ResLine]:
        return [c.get_result() for c in self._with_result()]

    def formatted_string(self, offset=0) -> str:
        sets = filter(lambda c: isinstance(c, CompareCheckSet), self._with_result())
        checks = filter(lambda c: isinstance(c, CompareCheck), self._with_result())
        checks = sorted(checks, key=lambda c: c.get_result().diff, reverse=True,)

        lines = [
            f"{' ' * offset}Check set[{self.name}]:",
        ]

        sets = list(sets)
        checks = list(checks)
        if not checks and not sets:
            lines.append(f"{' ' * offset}No results.")

        for c in checks:
            lines.append(f"{' ' * offset + '  '}{c.formatted_string()}")

        for s in sets:
            lines += [
                s.formatted_string(offset + 1),
            ]

        return "\n".join(lines)

    def _with_result(self):
        return (c for c in self.checks if c.get_result() is not None)


def build_check_set_for_files(
    name: str,
    old_path: str,
    new_path: str,
    *,
    ext: str = "",
    recursive: bool = False,
    op: Callable[[Any, Any], Any] = lambda previous, current: current - previous,
    do: Callable[[Any], Any] = lambda x: x,
    zero: Any = 0,
    diff_format: Callable[[Any], str] = lambda x: str(x),
    format: Callable[[Any], str] = lambda x: str(x),
):
    if recursive:
        raise NotImplementedError(
            f"CheckSetBuilderForFiles is not implemented for recursive."
        )

    files = list(filter(lambda f: f.endswith(ext), os.listdir(old_path)))
    s = set(files) ^ set(filter(lambda f: f.endswith(ext), os.listdir(new_path)))
    assert len(s) == 0, s

    cs = CompareCheckSet(name)
    for file in files:
        cs.add_check(
            CompareCheck(
                file, os.path.join(old_path, file), os.path.join(new_path, file)
            )
        )

    cs.set_do(do)
    cs.set_op(op)
    cs.set_zero(zero)
    cs.set_diff_format(diff_format)
    cs.set_format(format)
    return cs
