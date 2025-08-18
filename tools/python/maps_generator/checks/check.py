import os
import sys
from abc import ABC
from abc import abstractmethod
from collections import namedtuple
from enum import Enum
from functools import lru_cache
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


def norm(value):
    if isinstance(value, (int, float)):
        return abs(value)
    elif hasattr(value, "__len__"):
        return len(value)
    elif hasattr(value, "norm"):
        return value.norm()

    assert False, type(value)


def get_rel(r: ResLine) -> bool:
    rel = 0.0
    if r.arrow != Arrow.zero:
        prev = norm(r.previous)
        if prev == 0:
            rel = 100.0
        else:
            rel = norm(r.diff) * 100.0 / prev
    return rel


class Check(ABC):
    """
    Base class for any checks.
    Usual flow:

      # Create check object.
      check = AnyCheck("ExampleCheck")
      # Do work.
      check.check()

      # Get results and process them
      raw_result = check.get_result()
      process_result(raw_result)

      # or print result
      check.print()
    """
    def __init__(self, name: str):
        self.name = name

    def print(self, silent_if_no_results=False, filt=None, file=sys.stdout):
        s = self.formatted_string(silent_if_no_results, filt)
        if s:
            print(s, file=file)

    @abstractmethod
    def check(self):
        """
        Performs a logic of the check.
        """
        pass

    @abstractmethod
    def get_result(self) -> Any:
        """
        Returns a raw result of the check.
        """
        pass

    @abstractmethod
    def formatted_string(self, silent_if_no_results=False, *args, **kwargs) -> str:
        """
        Returns a formatted string of a raw result of the check.
        """
        pass


class CompareCheckBase(Check, ABC):
    def __init__(self, name: str):
        super().__init__(name)
        self.op: Callable[
            [Any, Any], Any
        ] = lambda previous, current: current - previous
        self.do: Callable[[Any], Any] = lambda x: x
        self.zero: Any = 0
        self.diff_format: Callable[[Any], str] = lambda x: str(x)
        self.format: Callable[[Any], str] = lambda x: str(x)
        self.filt: Callable[[Any], bool] = lambda x: True

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

    def set_filt(self, filt: Callable[[Any], bool]):
        self.filt = filt


class CompareCheck(CompareCheckBase):
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

    def formatted_string(self, silent_if_no_results=False, *args, **kwargs) -> str:
        assert self.result

        if silent_if_no_results and self.result.arrow == Arrow.zero:
            return ""

        rel = get_rel(self.result)
        return (
            f"{self.name}: {ROW_TO_STR[self.result.arrow]} {rel:.2f}% "
            f"[{self.format(self.result.previous)} → "
            f"{self.format(self.result.current)}: "
            f"{self.diff_format(self.result.diff)}]"
        )


class CompareCheckSet(CompareCheckBase):
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

    def formatted_string(self, silent_if_no_results=False, filt=None, _offset=0) -> str:
        sets = filter(lambda c: isinstance(c, CompareCheckSet), self._with_result())
        checks = filter(lambda c: isinstance(c, CompareCheck), self._with_result())
        checks = sorted(checks, key=lambda c: norm(c.get_result().diff), reverse=True)

        if filt is None:
            filt = self.filt

        checks = filter(lambda c: filt(c.get_result()), checks)

        sets = list(sets)
        checks = list(checks)

        no_results = not checks and not sets
        if silent_if_no_results and no_results:
            return ""

        head = [
            f"{' ' * _offset}Check set[{self.name}]:",
        ]

        lines = []
        if no_results:
            lines.append(f"{' ' * (_offset + 2)}No results.")

        for c in checks:
            s = c.formatted_string(silent_if_no_results, filt, _offset + 2)
            if s:
                lines.append(f"{' ' * (_offset + 2)}{s}")

        for s in sets:
            s = s.formatted_string(silent_if_no_results, filt, _offset + 2)
            if s:
                lines.append(s)

        if not lines:
            return ""

        head += lines
        return "\n".join(head) + "\n"

    def _with_result(self):
        return (c for c in self.checks if c.get_result() is not None)


@lru_cache(maxsize=None)
def _get_and_check_files(old_path, new_path, ext):
    files = list(filter(lambda f: f.endswith(ext), os.listdir(old_path)))
    s = set(files) ^ set(filter(lambda f: f.endswith(ext), os.listdir(new_path)))
    assert len(s) == 0, s
    return files


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

    cs = CompareCheckSet(name)
    for file in _get_and_check_files(old_path, new_path, ext):
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
