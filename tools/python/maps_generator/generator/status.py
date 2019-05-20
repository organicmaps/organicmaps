import os


class Status:
    def __init__(self):
        self.stat_path = None
        self.stat_next = None
        self.stat_saved = None
        self.find = False

    def init(self, stat_path, stat_next):
        self.stat_path = stat_path
        self.stat_next = stat_next
        if os.path.exists(self.stat_path) and os.path.isfile(self.stat_path):
            with open(self.stat_path) as status:
                self.stat_saved = status.read()
        if not self.find:
            self.find = self.stat_saved is None or not self.need_skip()

    def need_skip(self):
        if self.find:
            return False
        return self.stat_saved is not None and self.stat_next != self.stat_saved

    def update_status(self):
        with open(self.stat_path, "w") as status:
            status.write(self.stat_next)

    def finish(self):
        with open(self.stat_path, "w") as status:
            status.write("finish")
