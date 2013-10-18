import pprint
from xml.dom import minidom

dom = minidom.parse("_nori.svg")

groups = [(a.getAttribute("inkscape:label"), 
           (float(a.getElementsByTagName("rect")[0].getAttribute("width")), float(a.getElementsByTagName("rect")[0].getAttribute("height")), float(a.getElementsByTagName("rect")[0].getAttribute("x")), float(a.getElementsByTagName("rect")[0].getAttribute("y"))),
           a) for a in dom.getElementsByTagName("g") if a.getAttribute("inkscape:label") and a.getElementsByTagName("rect")]

pprint.pprint([groups, len(groups)])
for name, bbox, v in groups:
    print name
    dx, dy = 0, 0
    rt = v.getElementsByTagName("rect")[0].getAttribute("transform")
    if rt and 'translate' in rt:
        dy =  1028.3622
    rt = v.getAttribute("transform")
    if rt and "translate" in rt:
        dx += float(rt.split("(")[1].split(",")[0])
        dy += float(rt.split("(")[1].split(",")[1].strip(")"))
    open(name+".svg", 'w').write(
        '<svg width="%s" height="%s" viewBox="%s %s %s %s">'%(bbox[0], bbox[1], bbox[2]+dx, bbox[3]+dy, bbox[0], bbox[1])+
        v.toxml() +
        '</svg>')
