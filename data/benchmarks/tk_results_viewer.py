from Tkinter import *
import tkMessageBox
import math
import sys
import copy
import os
import shutil
import time

def less_start_time(t1, t2):
    fmt = '%a_%b_%d_%H:%M:%S_%Y'
    return time.strptime(t1[0].replace('__','_'), fmt) < time.strptime(t2[0].replace('__','_'), fmt)

class BenchmarkResultsFrame(Frame):
    def __init__(self, cfg_file, results_file, master=None):
        Frame.__init__(self, master)

        self.master.title("Benchmark Results Viewer")
        self.grid(padx=10, pady=10)

        self.createWidgets()
        self.resultsDir = os.path.dirname(results_file)
        self.readResults(cfg_file, results_file)
        self.current = None
        self.poll()

    def scale_level(self, r):
        dx = 360.0 / (r[2] - r[0])
        dy = 360.0 / (r[3] - r[1])

        v = (dx + dy) / 2.0

        l = math.log(v) / math.log(2.0) + 1
        return math.floor(max(0, min(l, 17)) + 0.5)

    def poll(self):
        now = self.resultsList.curselection()
        if now != self.current:
            self.onResultsChanged(now)
            self.current = now
        self.after(100, self.poll)

    def onResultsChanged(self, now):
        if len(now) == 0:
            return

        self.resultsView.delete(1.0, END)

        for i in now:
            start_time = self.resultsList.get(int(i))

            for rev_name in self.rev.keys():
                if start_time in self.rev[rev_name]:
                    self.resultsView.insert(END, rev_name + "\n")
                    self.resultsView.insert(END, "\t%s\n" % (start_time))
                    for bench_name in self.rev[rev_name][start_time].keys():
                        if bench_name not in self.bench_cfg:
                            s = "\t\t%s [config info not found]\n" % (bench_name)
                            self.resultsView.insert(END, s)
                        else:
                            cfg_info = self.bench_cfg[bench_name]
                            if not cfg_info[0]:
                                s = "\t\t%s [%s %s %s %s], endScale=%d\n" % \
                                    (bench_name, cfg_info[1], cfg_info[2], cfg_info[3], cfg_info[4], cfg_info[5])
                                self.resultsView.insert(END, s)
                            else:
                                s = "\t\t%s endScale=%d\n" % \
                                    (bench_name, cfg_info[1])
                                self.resultsView.insert(END, s)
                        k = self.rev[rev_name][start_time][bench_name].keys()
                        k.sort()
                        for scale_level in k:
                            s = "\t\t\t scale: %d, duration: %f\n" % \
                                (scale_level, self.rev[rev_name][start_time][bench_name][scale_level])
                            self.resultsView.insert(END, s)

            self.resultsView.insert(END, "-------------------------------------------------------------------\n")
            if self.hasTraceAttachement(start_time):
                self.resultsView.insert(END, "%s attached\n" % (self.traceAttachementFile(start_time)))

    def readResults(self, cfg_file, results_file):
        self.results_file = results_file

        f1 = open(cfg_file, "r")
        lns1 = f1.readlines()
        lns1 = [l.split(" ") for l in lns1]

        # reading benchmark configuration info

        self.bench_cfg = {}

        for l in lns1:
            c_name = l[1]
            is_country = (len(l) == 3)
            self.bench_cfg[l[1]] = []
            self.bench_cfg[l[1]].append(is_country)
            if len(l) > 0:
                if not is_country:
                    self.bench_cfg[c_name].append(float(l[2]))
                    self.bench_cfg[c_name].append(float(l[3]))
                    self.bench_cfg[c_name].append(float(l[4]))
                    self.bench_cfg[c_name].append(float(l[5]))
                    self.bench_cfg[c_name].append(int(l[6]))
                else:
                    self.bench_cfg[c_name].append(int(l[2]))

        # reading results file

        f = open(results_file, "r")
        lns = f.readlines()

        lns = [l.strip().split(" ") for l in lns]

        self.rev = {}

        cur_start_time = None
        is_session = False

        self.start_time_list = []
        self.completion_status = []

        for l in lns:
            if l[0] == "START":
                if cur_start_time is not None:
                    if is_session:
                        # unfinished benchmark, mark as incomplete
                        self.completion_status.append(0)
                    else:
                        # unknown status
                        self.completion_status.append(2)
                cur_start_time = l[1].strip("\n")
                self.start_time_list.append(cur_start_time)
                is_session = True
                continue

            if l[0] == "END":
                if not is_session:
                    raise "END without matching START"
                self.completion_status.append(1)
                cur_start_time = None
                is_session = False
                continue

            if len(l) < 9:
                "android benchmarks don't write first item"
                l = [" "] + l
            rev_name = l[1]
            start_time = l[2]
            bench_name = l[3]

            if cur_start_time != start_time:
                # checking whether new start_time breaks current session
                if is_session:
                    self.completion_status.append(0)
                    is_session = False
                else:
                    if cur_start_time is not None:
                        # unknown session type
                        self.completion_status.append(2)

                cur_start_time = start_time
                self.start_time_list.append(cur_start_time)

            rect = [float(l[4]), float(l[5]), float(l[6]), float(l[7])]
            dur = float(l[8])
            if rev_name not in self.rev:
                self.rev[rev_name] = {}
            if start_time not in self.rev[rev_name]:
                self.rev[rev_name][start_time] = {}
            if bench_name not in self.rev[rev_name][start_time]:
                self.rev[rev_name][start_time][bench_name] = {}

            scale = self.scale_level(rect)

            if scale not in self.rev[rev_name][start_time][bench_name]:
                self.rev[rev_name][start_time][bench_name][scale] = 0

            self.rev[rev_name][start_time][bench_name][scale] += dur

        if cur_start_time is not None:
            if is_session:
                self.completion_status.append(0)
            else:
                self.completion_status.append(2)

        # sorting session list. latest results go first.

        if len(self.start_time_list) != len(self.completion_status):
            raise "something wrong with file parsing, list sizes don't match"

        self.start_time_pairs = [(self.start_time_list[i], self.completion_status[i]) for i in range(0, len(self.start_time_list))]

        self.start_time_pairs.sort(less_start_time)

        # updating resultList with names and completion status

        i = 0

        for e in self.start_time_pairs:
            self.resultsList.insert(END, e[0])
            if e[1] == 0:
                self.resultsList.itemconfig(i, fg="red")
            elif e[1] == 1:
                self.resultsList.itemconfig(i, fg="blue")
            elif e[1] == 2:
                self.resultsList.itemconfig(i, fg="green")
            i += 1

    def hasTraceAttachement(self, start_time):
        return self.traceAttachementFile(start_time) is not None

    def traceAttachementName(self, start_time):
        return start_time.strip("\n").replace("_", "").replace(":", "").replace("-", "")

    def traceAttachementFile(self, start_time):
        trace_files = [t for t in os.listdir(os.path.join(os.curdir, self.resultsDir)) if t.endswith(".trace")]
        sst = self.traceAttachementName(start_time)
        for tf in trace_files:
            stf = tf[0:-6].replace("_", "").replace(":", "").replace("-", "")
            if stf == sst:
                return tf

    def deleteTraceAttachement(self, start_time):
        sst = self.traceAttachementName(start_time)
        if tkMessageBox.askokcancel("Profiler results found", "Delete " + self.traceAttachementFile(start_time)):
            shutil.rmtree(self.traceAttachementFile(start_time))

    def removeRecord(self, event):
        idx = self.resultsList.nearest(event.y)
        start_time = self.resultsList.get(idx)

        if tkMessageBox.askokcancel("Are you sure?", "Delete results for " + start_time + " session?"):
            lns = open(self.results_file, "r").readlines()
            lns = [l for l in lns if l.find(start_time) == -1]
            open(self.results_file, "w").writelines(lns)
            self.resultsList.delete(idx)

            if self.hasTraceAttachement(start_time):
                self.deleteTraceAttachement(start_time)

    def createWidgets(self):
        self.resultsList = Listbox(self, width=30, height=60, selectmode=EXTENDED)
        self.resultsList.grid(row=0, column=0)
        self.resultsList.bind("<Double-Button-1>", self.removeRecord)

        self.resultsView = Text(self, width=80, height=60)
        self.resultsView.grid(row=0, column=1, columnspan=3)

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print "usage: python %s [config.info] [results.txt]" % sys.argv[0]
    root = BenchmarkResultsFrame(sys.argv[1], sys.argv[2])
    root.mainloop()
