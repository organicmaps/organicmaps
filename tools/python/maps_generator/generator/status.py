import os
from typing import AnyStr
from typing import Optional


def with_stat_ext(country: AnyStr):
    return f"{country}.status"


def without_stat_ext(status: AnyStr):
    return status.replace(".status", "")


class Status:
    """Status is used for recovering and continuation maps generation."""

    def __init__(
        self, stat_path: Optional[AnyStr] = None, stat_next: Optional[AnyStr] = None
    ):
        self.stat_path = stat_path
        self.stat_next = stat_next
        self.stat_saved = None
        self.find = False

    def init(self, stat_path: AnyStr, stat_next: AnyStr):
        self.stat_path = stat_path
        self.stat_next = stat_next
        self.stat_saved = self.status()
        if not self.find:
            self.find = not self.stat_saved or not self.need_skip()

    def need_skip(self) -> bool:
        if self.find:
            return False
        return self.stat_saved and self.stat_next != self.stat_saved

    def update_status(self):
        with open(self.stat_path, "w") as status:
            status.write(self.stat_next)

    def finish(self):
        with open(self.stat_path, "w") as status:
            status.write("finish")

    def is_finished(self):
        return self.status() == "finish"

    def status(self):
        try:
            with open(self.stat_path) as status:
                return status.read()
        except IOError:
            return None
