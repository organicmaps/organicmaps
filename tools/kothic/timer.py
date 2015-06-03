from timeit import default_timer

class Timer(object):
    def __init__(self):
        self.timer = default_timer
        self.start = self.timer()

    def Reset(self):
        self.start = self.timer()

    def ElapsedMsec(self):
        elapsed_secs = self.timer() - self.start
        return elapsed_secs * 1000

    def ElapsedSec(self):
        elapsed_secs = self.timer() - self.start
        return elapsed_secs

class AccumulativeTimer(object):
    def __init__(self):
        self.timer = default_timer
        self.elapsed_secs = 0
        self.start = 0
        self.count = 0

    def Start(self):
        self.start = self.timer()

    def Stop(self):
        self.elapsed_secs += self.timer() - self.start
        self.start = 0
        self.count += 1

    def ElapsedMsec(self):
        elapsed_msec = self.elapsed_secs * 1000
        return elapsed_msec

    def ElapsedSec(self):
        return self.elapsed_secs

    def Count(self):
        return self.count
