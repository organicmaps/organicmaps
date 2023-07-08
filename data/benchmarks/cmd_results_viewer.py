import math
import sys


f1 = open(sys.argv[2], "r")
lns1 = f1.readlines()
lns1 = [l.split(" ") for l in lns1]

bench_cfg = {}

for l in lns1:
    c_name = l[1]
    is_country = (len(l) == 3)
    bench_cfg[l[1]] = []
    bench_cfg[l[1]].append(is_country) 
    if len(l) > 0:
        if not is_country:
            bench_cfg[c_name].append(float(l[2]))
            bench_cfg[c_name].append(float(l[3]))
            bench_cfg[c_name].append(float(l[4]))
            bench_cfg[c_name].append(float(l[5]))
            bench_cfg[c_name].append(int(l[6]))
        else:
            bench_cfg[c_name].append(int(l[2]))

f = open(sys.argv[1], "r")
lns = f.readlines()

def scale_level(r):
    dx = 360.0 / (r[2] - r[0])
    dy = 360.0 / (r[3] - r[1])

    v = (dx + dy) / 2.0

    l = math.log(v) / math.log(2.0) + 1
    if l > 17:
        l = 17
    if l < 0:
        return 0
    else:
        return math.floor(l + 0.5)

lns = [l.split(" ") for l in lns]

rev = {}

for l in lns:
    rev_name = l[1]
    start_time = l[2]
    bench_name = l[3]

    rect = [float(l[4]), float(l[5]), float(l[6]), float(l[7])]
    dur = float(l[8])
    if not rev.has_key(rev_name):
        rev[rev_name] = {}
    if not rev[rev_name].has_key(start_time):
        rev[rev_name][start_time] = {}
    if not rev[rev_name][start_time].has_key(bench_name):
        rev[rev_name][start_time][bench_name] = {}

    scale = scale_level(rect)

    if not rev[rev_name][start_time][bench_name].has_key(scale):
        rev[rev_name][start_time][bench_name][scale] = 0

    rev[rev_name][start_time][bench_name][scale] += dur

for rev_name in rev.keys():
    print rev_name
    for start_time in rev[rev_name].keys():
        print "\t", start_time
        for bench_name in rev[rev_name][start_time].keys():

            cfg_info = bench_cfg[bench_name]
            if not cfg_info[0]:
                print "\t\t", bench_name, "[", cfg_info[1], cfg_info[2], cfg_info[3], cfg_info[4], "]", "endScale=", cfg_info[5]
            else:
                print "\t\t", bench_name, "endScale=", cfg_info[1]
            k = rev[rev_name][start_time][bench_name].keys()
            k.sort()
            for scale_level in k:
                print "\t\t\t scale: ", scale_level, ", duration: " , rev[rev_name][start_time][bench_name][scale_level]
