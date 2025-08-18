#!/usr/bin/env python3

from xml.dom.minidom import parse, parseString
import sys


def renameAttr(tag, old, new):
    if not tag.getAttribute(old):
        return
    value = tag.getAttribute(old)
    if new == 'fill-rule':
        value = value.lower()

    tag.setAttribute(new, value)
    tag.removeAttribute(old)


def colorAndAlpha(value):
    if len(value) == 9 and value[0] == '#':
        return f"#{value[3:]}", str(int(value[1:3], 16) / 255.0)
    return value, ''


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print('Converts Android XML icons into SVG and prints it')
        print(
            f"Usage: {sys.argv[0]} <Android vector drawable xml icon>", file=sys.stderr)
        exit(1)
    xml = parse(sys.argv[1])
    svg = xml.documentElement
    svg.tagName = 'svg'
    if svg.hasAttribute('xmlns:aapt'):
        svg.removeAttribute('xmlns:aapt')
    if svg.hasAttribute('xmlns:android'):
        svg.removeAttribute('xmlns:android')
    svg.setAttribute('xmlns', 'http://www.w3.org/2000/svg')

    vw = svg.getAttribute('android:viewportWidth')
    svg.removeAttribute('android:viewportWidth')
    vh = svg.getAttribute('android:viewportHeight')
    svg.removeAttribute('android:viewportHeight')
    svg.setAttribute('viewbox', f"0 0 {vw} {vh}")

    svg.removeAttribute('android:width')
    svg.removeAttribute('android:height')
    svg.setAttribute('width', vw)
    svg.setAttribute('height', vh)

    # Used for gradients if any.
    defs = xml.createElement('defs')

    for path in svg.getElementsByTagName('path'):
        renameAttr(path, 'android:pathData', 'd')
        renameAttr(path, 'android:fillType', 'fill-rule')
        renameAttr(path, 'android:strokeWidth', 'stroke-width')
        if path.hasAttribute('android:strokeColor'):
            renameAttr(path, 'android:strokeColor', 'stroke')
            stroke = path.getAttribute('stroke')
            # Extract alpha if necessary.
            color, alpha = colorAndAlpha(stroke)
            path.setAttribute('stroke', color)
            if len(alpha) > 0:
                path.setAttribute('stroke-opacity', alpha)

        # Fill color.
        if path.hasAttribute('android:fillColor'):
            renameAttr(path, 'android:fillColor', 'fill')
        else:
            fillColor = 'none'
            for child in filter(lambda c: c.nodeType == c.ELEMENT_NODE, path.childNodes):
                if child.tagName == 'aapt:attr':
                    attrName = child.getAttribute('name')
                    if attrName == 'android:fillColor':
                        for grad in filter(lambda c: c.nodeType == c.ELEMENT_NODE, child.childNodes):
                            if grad.tagName != 'gradient':
                                print(
                                    f"Unsupported fill: {grad.tagName}", file=sys.stderr)
                                continue
                            gradType = grad.getAttribute('android:type')
                            if gradType != 'linear':
                                print(
                                    f"Unsupported gradient type: {gradType}", file=sys.stderr)
                                continue
                            # Extract linear gradient.
                            x1 = grad.getAttribute('android:startX')
                            x2 = grad.getAttribute('android:endX')
                            y1 = grad.getAttribute('android:startY')
                            y2 = grad.getAttribute('android:endY')
                            linearGradient = xml.createElement(
                                'linearGradient')
                            linearGradient.setAttribute('x1', x1)
                            linearGradient.setAttribute('x2', x2)
                            linearGradient.setAttribute('y1', y1)
                            linearGradient.setAttribute('y2', y2)
                            id = f"Gradient{len(defs.childNodes) + 1}"
                            linearGradient.setAttribute('id', id)
                            for item in filter(lambda c: c.nodeType == c.ELEMENT_NODE, grad.childNodes):
                                stop = xml.createElement('stop')
                                stop.setAttribute(
                                    'offset', item.getAttribute('android:offset'))
                                color, alpha = colorAndAlpha(
                                    item.getAttribute('android:color'))
                                stop.setAttribute('stop-color', color)
                                if len(alpha):
                                    stop.setAttribute('stop-opacity', alpha)
                                linearGradient.appendChild(stop)
                            defs.appendChild(linearGradient)
                            fillColor = f"url(#{id})"
                        path.removeChild(child)
                    else:
                        print(
                            f"Unsupported aapt:attr {attrName}", file=sys.stderr)
                else:
                    print(
                        f"Unsupported path child tag {child.tagName}", file=sys.stderr)

            path.setAttribute('fill', fillColor)

    # Insert gradients (if any).
    if len(defs.childNodes) > 0:
        svg.insertBefore(defs, svg.firstChild)
    pretty = svg.toprettyxml()
    pretty = pretty.replace('\t', '    ')
    pretty = "".join([s for s in pretty.splitlines(True) if s.strip()])
    print(pretty)
