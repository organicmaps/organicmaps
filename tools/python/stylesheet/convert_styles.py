import sys

lns = open(sys.argv[1]).readlines()
lns = [l.strip("\n") for l in lns]

newlns = []
isCaption = False

captionLns = []
leadSpaces = ""

i = 0

for l in lns:
    if not isCaption:
        i = l.find(" caption ")
        if i != -1:
            isCaption = True
            captionLns = []
            leadSpaces = l[0:i + 1]
            newlns.append(l)
            newlns.append(leadSpaces + "  primary {")
        else:
            i = l.find(" path_text ")
            if i != -1:
                isCaption = True
                captionLns = []
                leadSpaces = l[0:i + 1]
                newlns.append(l)
                newlns.append(leadSpaces + "  primary {")
            else:
                newlns.append(l)
    else:
        if l[i + 1] == "}":
            isCaption = False
            newlns.append(l)
        else:
            if l.find("priority") == -1:
                newlns.append("  " + l)
                captionLns.append("  " + l)
            else:
                newlns.append(leadSpaces + "  }")
#                newlns.append(leadSpaces + "  secondary {")
#                for l1 in captionLns:
#                    newlns.append(l1)
#                newlns.append(leadSpaces + "  }")
                newlns.append(l)


for i in newlns:
    print i
