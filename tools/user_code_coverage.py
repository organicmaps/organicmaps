import os
import json
import sys

if len(sys.argv) < 3:
    print "USAGE: " + sys.argv[0] + " [username] [htmlfile]"
    exit()

USERNAME = sys.argv[1]

HTMLFILE = sys.argv[1]

if __name__ == "__main__":
    os.system('git log --pretty="%H" --author="'+USERNAME+'" | while read commit_hash; do git show --oneline --name-only $commit_hash | tail -n+2; done | sort | uniq > /tmp/wrote.files')
    files = {}
    for f in open('/tmp/wrote.files'):
        f = f.strip()
        if os.path.exists(f):
            os.system("git blame -w "+f+" > /tmp/wrote.blame")
            stat = {'total': 0, 'unclean': 0}
            for line in open('/tmp/wrote.blame'):
                stat['total'] += 1
                if USERNAME in line:
                    stat['unclean'] += 1
            files[f] = stat
    html = open(HTMLFILE, 'w')
    print >> html, "<html><head><script src='http://www.kryogenix.org/code/browser/sorttable/sorttable.js'></script></head><body><table border=1 cellspacing=0 width=100% class='sortable'>"
    keys = files.keys()
    keys.sort(key = lambda a: 1. * files[a]['unclean'] / max(files[a]['total'],0.01))
    keys.sort(key = lambda a: files[a]['unclean'])
    keys.reverse()
    print >> html, "<tr><td><b>Filename</b></td><td><b>dirty LOC</b></td><td><b>LOC</b></td><td width=300><b>meter</b></td></tr>"
    for k in keys:
        v = files[k]
        print >> html, "<tr><td>%s</td><td>%s</td><td>%s</td><td width=300><meter style='width:300' value='%s' max='%s'> </meter></td></tr>"%(k,v['unclean'], v['total'],v['unclean'], v['total'] )
    print >> html, "</body></html>"