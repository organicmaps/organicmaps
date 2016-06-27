#!/usr/bin/env python2.7

from __future__ import print_function
import struct
import sys
import numpy


class Analyzer:
    """
    The binary format is
        time since the beginning of the measurement : double
        unknown and irrelevant field : double
        momentary consumption calculated for the current time segment : double
    """
    def __init__(self):
        self.duration = 0.0
        self.consumption = []
        self.mean = 0.0
        self.std = 0.0
        self.avg = 0.0
        self.averages = []


    def read_file(self, file_path):
        binary = bytearray()
        with open(file_path, "r") as f:
            binary = bytearray(f.read())

        for i in range(0, len(binary) - 24, 24):
            res = struct.unpack(">ddd", binary[i:i+24])

            current_duration = res[0]
            if not current_duration > self.duration:
                print("Unexpected elapsed time value, lower than the previous one.")
                exit(2) # this should never happen because the file is written sequentially

            current_consumption = res[2]
            self.averages.append(current_consumption / (current_duration - self.duration))
            self.duration = current_duration

            self.consumption.append(current_consumption)

        self.calculate_stats()


    def calculate_stats(self):
        self.mean = numpy.mean(self.averages)
        self.std = numpy.std(self.averages)
        self.avg = sum(self.consumption) / self.duration


if __name__ == "__main__":
    for file_path in sys.argv[1:]:
        analyzer = Analyzer()
        analyzer.read_file(file_path)
        print("{}\n\tavg:  {}\n\tmean: {}\n\tstd:  {}".format(file_path, analyzer.avg, analyzer.mean, analyzer.std))
