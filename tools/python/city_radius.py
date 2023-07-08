import sys, os, math
import matplotlib.pyplot as plt
from optparse import OptionParser


cities = []

def strip(s):
    return s.strip('\t\n ')

def load_data(path):
    
    global cities

    f = open(path, 'r')
    lines = f.readlines()
    f.close();

    for l in lines:

        if l.startswith('#'):
            continue

        data = l.split('|')

        if len(data) < 6:
            continue

        item = {}

        item['name'] = strip(data[0])
        item['population'] = int(strip(data[1]))
        item['region'] = strip(data[2])
        item['width'] = float(strip(data[3]))
        item['height'] = float(strip(data[4]))

        item['square'] = float(data[5])

        cities.append(item)

    # build plot
    print "Cities count: %d" % len(cities)

def formula(popul, base = 32, mult = 0.5):
    #return math.exp(math.log(popul, base)) * mult
    return math.pow(popul, 1 / base) * mult

def avgDistance(approx, data):
    dist = 0
    for x in xrange(len(data)):
        dist += math.fabs(approx[x] - data[x])
    return dist / float(len(data))  

def findBest(popul, data, minBase = 5, maxBase = 100, stepBase = 0.1, minMult = 0.01, maxMult = 1, stepMult = 0.01):

    # try to find best parameters
    base = minBase

    minDist = -1
    bestMult = minMult
    bestBase = base

    while base <= maxBase:
        print "%.02f%% best mult: %f, best base: %f, best dist: %f" % (100 * (base - minBase) / (maxBase - minBase), bestMult, bestBase, minDist)
        mult = minMult
        
        while mult <= maxMult:
            approx = []

            for p in popul:
                approx.append(formula(p, base, mult))

            dist = avgDistance(approx, data)

            if minDist < 0 or minDist > dist:
                minDist = dist
                bestBase = base
                bestMult = mult

            mult += stepMult

        base += stepBase

    return (bestBase, bestMult)

def process_data(steps_count, base, mult, bestFind = False, dataFlag = 0):
    avgData = []
    maxData = []
    sqrData = []
    population = []
    maxPopulation = 0
    minPopulation = -1
    for city in cities:
        p = city['population']
        w = city['width']
        h = city['height']
        s = city['square']
        population.append(p)
        if p > maxPopulation:
            maxPopulation = p
        if minPopulation < 0 or p < minPopulation:
            minPopulation = p

        maxData.append(max([w, h]))
        avgData.append((w + h) * 0.5)
        sqrData.append(math.sqrt(s))


    bestBase = base
    bestMult = mult
    if bestFind:
        d = maxData
        if dataFlag == 1:
            d = avgData
        elif dataFlag == 2:
            d = sqrData
        bestBase, bestMult = findBest(population, d)

    print "Finished\n\nBest mult: %f, Best base: %f" % (bestMult, bestBase)

    approx = []
    population2 = []
    v = minPopulation
    step = (maxPopulation - minPopulation) / float(steps_count)
    for i in xrange(0, steps_count):
        approx.append(formula(v, bestBase, bestMult))
        population2.append(v)
        v += step

    plt.plot(population, avgData, 'bo', population, maxData, 'ro', population, sqrData, 'go', population2, approx, 'y')
    plt.axis([minPopulation, maxPopulation, 0, 100])
    plt.xscale('log')
    plt.show()

if __name__ == "__main__":

    if len(sys.argv) < 3:
        print 'city_radius.py <data_file> <steps>'
    
    parser = OptionParser()
    parser.add_option("-f", "--file", dest="filename", default="city_popul_sqr.data",
                    help="source data file", metavar="path")
    parser.add_option("-s", "--scan",
                    dest="best", default=False, action="store_true",
                    help="scan best values of mult and base")
    parser.add_option('-m', "--mult",
                    dest='mult', default=1,
                    help='multiplier value')
    parser.add_option('-b', '--base',
                    dest='base', default=3.6,
                    help="base value")
    parser.add_option('-d', '--data', 
                    default=0, dest='data',
                    help="Dataset to use on best values scan: 0 - max, 1 - avg, 2 - sqr")

    (options, args) = parser.parse_args()
    load_data(options.filename)
    process_data(1000, float(options.base), float(options.mult), options.best, int(options.data))
