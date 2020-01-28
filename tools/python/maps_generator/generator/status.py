import os
from typing import AnyStr


class Status:
    """Status is used for recovering and continuation maps generation."""

    def __init__(self, stat_path: AnyStr = None, stat_next: AnyStr = None):
        self.stat_path = stat_path
        self.stat_next = stat_next
        self.stat_saved = None
        self.find = False

    def init(self, stat_path: AnyStr, stat_next: AnyStr):
        self.stat_path = stat_path
        self.stat_next = stat_next
        if os.path.exists(self.stat_path) and os.path.isfile(self.stat_path):
            with open(self.stat_path) as status:
                self.stat_saved = status.read()
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
