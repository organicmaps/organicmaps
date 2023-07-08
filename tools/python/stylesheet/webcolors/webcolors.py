# -*- coding: utf-8 -*-
"""
A simple library for working with the color names and color codes
defined by the HTML and CSS specifications.

An overview of HTML and CSS colors
----------------------------------

Colors on the Web are specified in `the sRGB color space`_, where each
color is made up of a red component, a green component and a blue
component. This is useful because it maps (fairly) cleanly to the red,
green and blue components of pixels on a computer display, and to the
cone cells of a human eye, which come in three sets roughly
corresponding to the wavelengths of light associated with red, green
and blue.

`The HTML 4 standard`_ defines two ways to specify sRGB colors:

* A hash mark ('#') followed by three pairs of hexdecimal digits,
  specifying values for red, green and blue components in that order;
  for example, ``#0099cc``. Since each pair of hexadecimal digits can
  express 256 different values, this allows up to 256**3 or 16,777,216
  unique colors to be specified (though, due to differences in display
  technology, not all of these colors may be clearly distinguished on
  any given physical display).

* A set of predefined color names which correspond to specific
  hexadecimal values; for example, ``white``. HTML 4 defines sixteen
  such colors.

`The CSS 2 standard`_ allows any valid HTML 4 color specification, and
adds three new ways to specify sRGB colors:

* A hash mark followed by three hexadecimal digits, which is expanded
  into three hexadecimal pairs by repeating each digit; thus ``#09c``
  is equivalent to ``#0099cc``.

* The string 'rgb', followed by parentheses, between which are three
  numeric values each between 0 and 255, inclusive, which are taken to
  be the values of the red, green and blue components in that order;
  for example, ``rgb(0, 153, 204)``.

* The same as above, except using percentages instead of numeric
  values; for example, ``rgb(0%, 60%, 80%)``.

`The CSS 2.1 revision`_ does not add any new methods of specifying
sRGB colors, but does add one additional named color.

`The CSS 3 color module`_ (currently a W3C Candidate Recommendation)
adds one new way to specify sRGB colors:

* A hue-saturation-lightness triple (HSL), using the construct
  ``hsl()``.

It also adds support for variable opacity of colors, by allowing the
specification of alpha-channel information, through the ``rgba()`` and
``hsla()`` constructs, which are identical to ``rgb()`` and ``hsl()``
with one exception: a fourth value is supplied, indicating the level
of opacity from ``0.0`` (completely transparent) to ``1.0``
(completely opaque). Though not technically a color, the keyword
``transparent`` is also made available in lieu of a color value, and
corresponds to ``rgba(0,0,0,0)``.

Additionally, CSS3 defines a new set of color names; this set is taken
directly from the named colors defined for SVG (Scalable Vector
Graphics) markup, and is a proper superset of the named colors defined
in CSS 2.1. This set also has significant overlap with traditional X11
color sets as defined by the ``rgb.txt`` file on many Unix and
Unix-like operating systems, though the correspondence is not exact;
the set of X11 colors is not standardized, and the set of CSS3 colors
contains some definitions which diverge significantly from customary
X11 definitions (for example, CSS3's ``green`` is not equivalent to
X11's ``green``; the value which X11 designates ``green`` is
designated ``lime`` in CSS3).

.. _the sRGB color space: http://www.w3.org/Graphics/Color/sRGB
.. _The HTML 4 standard: http://www.w3.org/TR/html401/types.html#h-6.5
.. _The CSS 2 standard: http://www.w3.org/TR/REC-CSS2/syndata.html#value-def-color
.. _The CSS 2.1 revision: http://www.w3.org/TR/CSS21/
.. _The CSS 3 color module: http://www.w3.org/TR/css3-color/

What this module supports
-------------------------

The mappings and functions within this module support the following
methods of specifying sRGB colors, and conversions between them:

* Six-digit hexadecimal.

* Three-digit hexadecimal.

* Integer ``rgb()`` triplet.

* Percentage ``rgb()`` triplet.

* Varying selections of predefined color names (see below).

This module does not support ``hsl()`` triplets, nor does it support
opacity/alpha-channel information via ``rgba()`` or ``hsla()``.

If you need to convert between RGB-specified colors and HSL-specified
colors, or colors specified via other means, consult `the colorsys
module`_ in the Python standard library, which can perform conversions
amongst several common color spaces.

.. _the colorsys module: http://docs.python.org/library/colorsys.html

Normalization
-------------

For colors specified via hexadecimal values, this module will accept
input in the following formats:

* A hash mark (#) followed by three hexadecimal digits, where letters
  may be upper- or lower-case.

* A hash mark (#) followed by six hexadecimal digits, where letters
  may be upper- or lower-case.

For output which consists of a color specified via hexadecimal values,
and for functions which perform intermediate conversion to hexadecimal
before returning a result in another format, this module always
normalizes such values to the following format:

* A hash mark (#) followed by six hexadecimal digits, with letters
  forced to lower-case.

The function ``normalize_hex()`` in this module can be used to perform
this normalization manually if desired; see its documentation for an
explanation of the normalization process.

For colors specified via predefined names, this module will accept
input in the following formats:

* An entirely lower-case name, such as ``aliceblue``.

* A name using initial capitals, such as ``AliceBlue``.

For output which consists of a color specified via a predefined name,
and for functions which perform intermediate conversion to a
predefined name before returning a result in another format, this
module always normalizes such values to be entirely lower-case.

Mappings of color names
-----------------------

For each set of defined color names -- HTML 4, CSS 2, CSS 2.1 and CSS
3 -- this module exports two mappings: one of normalized color names
to normalized hexadecimal values, and one of normalized hexadecimal
values to normalized color names. These eight mappings are as follows:

``html4_names_to_hex``
    Mapping of normalized HTML 4 color names to normalized hexadecimal
    values.

``html4_hex_to_names``
    Mapping of normalized hexadecimal values to normalized HTML 4
    color names.

``css2_names_to_hex``
    Mapping of normalized CSS 2 color names to normalized hexadecimal
    values. Because CSS 2 defines the same set of named colors as HTML
    4, this is merely an alias for ``html4_names_to_hex``.

``css2_hex_to_names``
    Mapping of normalized hexadecimal values to normalized CSS 2 color
    nams. For the reasons described above, this is merely an alias for
    ``html4_hex_to_names``.

``css21_names_to_hex``
    Mapping of normalized CSS 2.1 color names to normalized
    hexadecimal values. This is identical to ``html4_names_to_hex``,
    except for one addition: ``orange``.

``css21_hex_to_names``
    Mapping of normalized hexadecimal values to normalized CSS 2.1
    color names. As above, this is identical to ``html4_hex_to_names``
    except for the addition of ``orange``.

``css3_names_to_hex``
    Mapping of normalized CSS3 color names to normalized hexadecimal
    values.

``css3_hex_to_names``
    Mapping of normalized hexadecimal values to normalized CSS3 color
    names.

"""

import math
import re
from hashlib import md5


def _reversedict(d):
    """
    Internal helper for generating reverse mappings; given a
    dictionary, returns a new dictionary with keys and values swapped.

    """
    return dict(zip(d.values(), d.keys()))

HEX_COLOR_RE = re.compile(r'^#([a-fA-F0-9]{3}|[a-fA-F0-9]{6})$')

SUPPORTED_SPECIFICATIONS = ('html4', 'css2', 'css21', 'css3')


######################################################################
# Mappings of color names to normalized hexadecimal color values.
######################################################################


html4_names_to_hex = {
    'aqua': '#00ffff',
    'black': '#000000',
    'blue': '#0000ff',
    'fuchsia': '#ff00ff',
    'green': '#008000',
    'grey': '#808080',
    'lime': '#00ff00',
    'maroon': '#800000',
    'navy': '#000080',
    'olive': '#808000',
    'purple': '#800080',
    'red': '#ff0000',
    'silver': '#c0c0c0',
    'teal': '#008080',
    'white': '#ffffff',
    'yellow': '#ffff00'
}

css2_names_to_hex = html4_names_to_hex

css21_names_to_hex = dict(html4_names_to_hex, orange='#ffa500')

css3_names_to_hex = {
    'aliceblue': '#f0f8ff',
    'antiquewhite': '#faebd7',
    'aqua': '#00ffff',
    'aquamarine': '#7fffd4',
    'azure': '#f0ffff',
    'beige': '#f5f5dc',
    'bisque': '#ffe4c4',
    'black': '#000000',
    'blanchedalmond': '#ffebcd',
    'blue': '#0000ff',
    'blueviolet': '#8a2be2',
    'brown': '#a52a2a',
    'burlywood': '#deb887',
    'cadetblue': '#5f9ea0',
    'chartreuse': '#7fff00',
    'chocolate': '#d2691e',
    'coral': '#ff7f50',
    'cornflowerblue': '#6495ed',
    'cornsilk': '#fff8dc',
    'crimson': '#dc143c',
    'cyan': '#00ffff',
    'darkblue': '#00008b',
    'darkcyan': '#008b8b',
    'darkgoldenrod': '#b8860b',
    'darkgray': '#a9a9a9',
    'darkgrey': '#a9a9a9',
    'darkgreen': '#006400',
    'darkkhaki': '#bdb76b',
    'darkmagenta': '#8b008b',
    'darkolivegreen': '#556b2f',
    'darkorange': '#ff8c00',
    'darkorchid': '#9932cc',
    'darkred': '#8b0000',
    'darksalmon': '#e9967a',
    'darkseagreen': '#8fbc8f',
    'darkslateblue': '#483d8b',
    'darkslategray': '#2f4f4f',
    'darkslategrey': '#2f4f4f',
    'darkturquoise': '#00ced1',
    'darkviolet': '#9400d3',
    'deeppink': '#ff1493',
    'deepskyblue': '#00bfff',
    'dimgray': '#696969',
    'dimgrey': '#696969',
    'dodgerblue': '#1e90ff',
    'firebrick': '#b22222',
    'floralwhite': '#fffaf0',
    'forestgreen': '#228b22',
    'fuchsia': '#ff00ff',
    'gainsboro': '#dcdcdc',
    'ghostwhite': '#f8f8ff',
    'gold': '#ffd700',
    'goldenrod': '#daa520',
    'gray': '#808080',
    'grey': '#808080',
    'green': '#008000',
    'greenyellow': '#adff2f',
    'honeydew': '#f0fff0',
    'hotpink': '#ff69b4',
    'indianred': '#cd5c5c',
    'indigo': '#4b0082',
    'ivory': '#fffff0',
    'khaki': '#f0e68c',
    'lavender': '#e6e6fa',
    'lavenderblush': '#fff0f5',
    'lawngreen': '#7cfc00',
    'lemonchiffon': '#fffacd',
    'lightblue': '#add8e6',
    'lightcoral': '#f08080',
    'lightcyan': '#e0ffff',
    'lightgoldenrodyellow': '#fafad2',
    'lightgray': '#d3d3d3',
    'lightgrey': '#d3d3d3',
    'lightgreen': '#90ee90',
    'lightpink': '#ffb6c1',
    'lightsalmon': '#ffa07a',
    'lightseagreen': '#20b2aa',
    'lightskyblue': '#87cefa',
    'lightslategray': '#778899',
    'lightslategrey': '#778899',
    'lightsteelblue': '#b0c4de',
    'lightyellow': '#ffffe0',
    'lime': '#00ff00',
    'limegreen': '#32cd32',
    'linen': '#faf0e6',
    'magenta': '#ff00ff',
    'maroon': '#800000',
    'mediumaquamarine': '#66cdaa',
    'mediumblue': '#0000cd',
    'mediumorchid': '#ba55d3',
    'mediumpurple': '#9370d8',
    'mediumseagreen': '#3cb371',
    'mediumslateblue': '#7b68ee',
    'mediumspringgreen': '#00fa9a',
    'mediumturquoise': '#48d1cc',
    'mediumvioletred': '#c71585',
    'midnightblue': '#191970',
    'mintcream': '#f5fffa',
    'mistyrose': '#ffe4e1',
    'moccasin': '#ffe4b5',
    'navajowhite': '#ffdead',
    'navy': '#000080',
    'oldlace': '#fdf5e6',
    'olive': '#808000',
    'olivedrab': '#6b8e23',
    'orange': '#ffa500',
    'orangered': '#ff4500',
    'orchid': '#da70d6',
    'palegoldenrod': '#eee8aa',
    'palegreen': '#98fb98',
    'paleturquoise': '#afeeee',
    'palevioletred': '#d87093',
    'papayawhip': '#ffefd5',
    'peachpuff': '#ffdab9',
    'peru': '#cd853f',
    'pink': '#ffc0cb',
    'plum': '#dda0dd',
    'powderblue': '#b0e0e6',
    'purple': '#800080',
    'red': '#ff0000',
    'rosybrown': '#bc8f8f',
    'royalblue': '#4169e1',
    'saddlebrown': '#8b4513',
    'salmon': '#fa8072',
    'sandybrown': '#f4a460',
    'seagreen': '#2e8b57',
    'seashell': '#fff5ee',
    'sienna': '#a0522d',
    'silver': '#c0c0c0',
    'skyblue': '#87ceeb',
    'slateblue': '#6a5acd',
    'slategray': '#708090',
    'slategrey': '#708090',
    'snow': '#fffafa',
    'springgreen': '#00ff7f',
    'steelblue': '#4682b4',
    'tan': '#d2b48c',
    'teal': '#008080',
    'thistle': '#d8bfd8',
    'tomato': '#ff6347',
    'turquoise': '#40e0d0',
    'violet': '#ee82ee',
    'wheat': '#f5deb3',
    'white': '#ffffff',
    'whitesmoke': '#f5f5f5',
    'yellow': '#ffff00',
    'yellowgreen': '#9acd32',
}


######################################################################
# Mappings of normalized hexadecimal color values to color names.
######################################################################


html4_hex_to_names = _reversedict(html4_names_to_hex)

css2_hex_to_names = html4_hex_to_names

css21_hex_to_names = _reversedict(css21_names_to_hex)

css3_hex_to_names = _reversedict(css3_names_to_hex)


######################################################################
# Normalization routines.
######################################################################


def normalize_hex(hex_value):
    """
    Normalize a hexadecimal color value to the following form and
    return the result::

        #[a-f0-9]{6}

    In other words, the following transformations are applied as
    needed:

    * If the value contains only three hexadecimal digits, it is
      expanded to six.

    * The value is normalized to lower-case.

    If the supplied value cannot be interpreted as a hexadecimal color
    value, ``ValueError`` is raised.

    Examples:

    >>> normalize_hex('#09c')
    '#0099cc'
    >>> normalize_hex('#0099cc')
    '#0099cc'
    >>> normalize_hex('#09C')
    '#0099cc'
    >>> normalize_hex('#0099CC')
    '#0099cc'
    >>> normalize_hex('0099cc')
    Traceback (most recent call last):
        ...
    ValueError: '0099cc' is not a valid hexadecimal color value.
    >>> normalize_hex('#0099QX')
    Traceback (most recent call last):
        ...
    ValueError: '#0099QX' is not a valid hexadecimal color value.
    >>> normalize_hex('foobarbaz')
    Traceback (most recent call last):
        ...
    ValueError: 'foobarbaz' is not a valid hexadecimal color value.
    >>> normalize_hex('#0')
    Traceback (most recent call last):
        ...
    ValueError: '#0' is not a valid hexadecimal color value.

    """
    try:
        hex_digits = HEX_COLOR_RE.match(hex_value).groups()[0]
    except AttributeError:
        raise ValueError("'%s' is not a valid hexadecimal color value." % hex_value)
    if len(hex_digits) == 3:
        hex_digits = ''.join(map(lambda s: 2 * s, hex_digits))
    return '#%s' % hex_digits.lower()


######################################################################
# Conversions from color names to various formats.
######################################################################


def name_to_hex(name, spec='css3'):
    """
    Convert a color name to a normalized hexadecimal color value.

    The optional keyword argument ``spec`` determines which
    specification's list of color names will be used; valid values are
    ``html4``, ``css2``, ``css21`` and ``css3``, and the default is
    ``css3``.

    The color name will be normalized to lower-case before being
    looked up, and when no color of that name exists in the given
    specification, ``ValueError`` is raised.

    Examples:

    >>> name_to_hex('deepskyblue')
    '#00bfff'
    >>> name_to_hex('DeepSkyBlue')
    '#00bfff'
    >>> name_to_hex('white', spec='html4')
    '#ffffff'
    >>> name_to_hex('white', spec='css2')
    '#ffffff'
    >>> name_to_hex('white', spec='css21')
    '#ffffff'
    >>> name_to_hex('white', spec='css3')
    '#ffffff'
    >>> name_to_hex('white', spec='css4')
    Traceback (most recent call last):
        ...
    TypeError: 'css4' is not a supported specification for color name lookups; supported specifications are: html4, css2, css21, css3.
    >>> name_to_hex('deepskyblue', spec='css2')
    Traceback (most recent call last):
        ...
    ValueError: 'deepskyblue' is not defined as a named color in css2.

    """
    if spec not in SUPPORTED_SPECIFICATIONS:
        raise TypeError("'%s' is not a supported specification for color name lookups; supported specifications are: %s." % (spec,
                                                                                                                             ', '.join(SUPPORTED_SPECIFICATIONS)))
    normalized = name.lower()
    try:
        hex_value = globals()['%s_names_to_hex' % spec][normalized]
    except KeyError:
        raise ValueError("'%s' is not defined as a named color in %s." % (name, spec))
    return hex_value


def name_to_rgb(name, spec='css3'):
    """
    Convert a color name to a 3-tuple of integers suitable for use in
    an ``rgb()`` triplet specifying that color.

    The optional keyword argument ``spec`` determines which
    specification's list of color names will be used; valid values are
    ``html4``, ``css2``, ``css21`` and ``css3``, and the default is
    ``css3``.

    The color name will be normalized to lower-case before being
    looked up, and when no color of that name exists in the given
    specification, ``ValueError`` is raised.

    Examples:

    >>> name_to_rgb('navy')
    (0, 0, 128)
    >>> name_to_rgb('cadetblue')
    (95, 158, 160)
    >>> name_to_rgb('cadetblue', spec='html4')
    Traceback (most recent call last):
        ...
    ValueError: 'cadetblue' is not defined as a named color in html4.

    """
    return hex_to_rgb(name_to_hex(name, spec=spec))


def name_to_rgb_percent(name, spec='css3'):
    """
    Convert a color name to a 3-tuple of percentages suitable for use
    in an ``rgb()`` triplet specifying that color.

    The optional keyword argument ``spec`` determines which
    specification's list of color names will be used; valid values are
    ``html4``, ``css2``, ``css21`` and ``css3``, and the default is
    ``css3``.

    The color name will be normalized to lower-case before being
    looked up, and when no color of that name exists in the given
    specification, ``ValueError`` is raised.

    Examples:

    >>> name_to_rgb_percent('white')
    ('100%', '100%', '100%')
    >>> name_to_rgb_percent('navy')
    ('0%', '0%', '50%')
    >>> name_to_rgb_percent('goldenrod')
    ('85.49%', '64.71%', '12.5%')

    """
    return rgb_to_rgb_percent(name_to_rgb(name, spec=spec))


######################################################################
# Conversions from hexadecimal color values to various formats.
######################################################################


def hex_to_name(hex_value, spec='css3'):
    """
    Convert a hexadecimal color value to its corresponding normalized
    color name, if any such name exists.

    The optional keyword argument ``spec`` determines which
    specification's list of color names will be used; valid values are
    ``html4``, ``css2``, ``css21`` and ``css3``, and the default is
    ``css3``.

    The hexadecimal value will be normalized before being looked up,
    and when no color name for the value is found in the given
    specification, ``ValueError`` is raised.

    Examples:

    >>> hex_to_name('#000080')
    'navy'
    >>> hex_to_name('#000080', spec='html4')
    'navy'
    >>> hex_to_name('#000080', spec='css2')
    'navy'
    >>> hex_to_name('#000080', spec='css21')
    'navy'
    >>> hex_to_name('#8b4513')
    'saddlebrown'
    >>> hex_to_name('#8b4513', spec='html4')
    Traceback (most recent call last):
        ...
    ValueError: '#8b4513' has no defined color name in html4.
    >>> hex_to_name('#8b4513', spec='css4')
    Traceback (most recent call last):
        ...
    TypeError: 'css4' is not a supported specification for color name lookups; supported specifications are: html4, css2, css21, css3.

    """
    if spec not in SUPPORTED_SPECIFICATIONS:
        raise TypeError("'%s' is not a supported specification for color name lookups; supported specifications are: %s." % (spec,
                                                                                                                             ', '.join(SUPPORTED_SPECIFICATIONS)))
    normalized = normalize_hex(hex_value)
    try:
        name = globals()['%s_hex_to_names' % spec][normalized]
    except KeyError:
        raise ValueError("'%s' has no defined color name in %s." % (hex_value, spec))
    return name


def hex_to_color_name(hex_value):
    hex_value = hex_value.strip('#')
    hex_value = "#" + "0" * (6 - len(hex_value)) + hex_value
    try:
        return hex_to_name(hex_value)
    except ValueError:
        return hex_value


def hex_to_rgb(hex_value):
    """
    Convert a hexadecimal color value to a 3-tuple of integers
    suitable for use in an ``rgb()`` triplet specifying that color.

    The hexadecimal value will be normalized before being converted.

    Examples:

    >>> hex_to_rgb('#000080')
    (0, 0, 128)
    >>> hex_to_rgb('#ffff00')
    (255, 255, 0)
    >>> hex_to_rgb('#f00')
    (255, 0, 0)
    >>> hex_to_rgb('#deb887')
    (222, 184, 135)

    """
    hex_digits = normalize_hex(hex_value)
    return tuple(map(lambda s: int(s, 16),
                     (hex_digits[1:3], hex_digits[3:5], hex_digits[5:7])))


def hex_to_rgb_percent(hex_value):
    """
    Convert a hexadecimal color value to a 3-tuple of percentages
    suitable for use in an ``rgb()`` triplet representing that color.

    The hexadecimal value will be normalized before converting.

    Examples:

    >>> hex_to_rgb_percent('#ffffff')
    ('100%', '100%', '100%')
    >>> hex_to_rgb_percent('#000080')
    ('0%', '0%', '50%')

    """
    return rgb_to_rgb_percent(hex_to_rgb(hex_value))


######################################################################
# Conversions from  integer rgb() triplets to various formats.
######################################################################


def rgb_to_name(rgb_triplet, spec='css3'):
    """
    Convert a 3-tuple of integers, suitable for use in an ``rgb()``
    color triplet, to its corresponding normalized color name, if any
    such name exists.

    The optional keyword argument ``spec`` determines which
    specification's list of color names will be used; valid values are
    ``html4``, ``css2``, ``css21`` and ``css3``, and the default is
    ``css3``.

    If there is no matching name, ``ValueError`` is raised.

    Examples:

    >>> rgb_to_name((0, 0, 0))
    'black'
    >>> rgb_to_name((0, 0, 128))
    'navy'
    >>> rgb_to_name((95, 158, 160))
    'cadetblue'

    """
    return hex_to_name(rgb_to_hex(rgb_triplet), spec=spec)


def rgb_to_hex(rgb_triplet):
    """
    Convert a 3-tuple of integers, suitable for use in an ``rgb()``
    color triplet, to a normalized hexadecimal value for that color.

    Examples:

    >>> rgb_to_hex((255, 255, 255))
    '#ffffff'
    >>> rgb_to_hex((0, 0, 128))
    '#000080'
    >>> rgb_to_hex((33, 56, 192))
    '#2138c0'

    """
    return '#%02x%02x%02x' % rgb_triplet


def rgb_to_rgb_percent(rgb_triplet):
    """
    Convert a 3-tuple of integers, suitable for use in an ``rgb()``
    color triplet, to a 3-tuple of percentages suitable for use in
    representing that color.

    This function makes some trade-offs in terms of the accuracy of
    the final representation; for some common integer values,
    special-case logic is used to ensure a precise result (e.g.,
    integer 128 will always convert to '50%', integer 32 will always
    convert to '12.5%'), but for all other values a standard Python
    ``float`` is used and rounded to two decimal places, which may
    result in a loss of precision for some values.

    Examples:

    >>> rgb_to_rgb_percent((255, 255, 255))
    ('100%', '100%', '100%')
    >>> rgb_to_rgb_percent((0, 0, 128))
    ('0%', '0%', '50%')
    >>> rgb_to_rgb_percent((33, 56, 192))
    ('12.94%', '21.96%', '75.29%')
    >>> rgb_to_rgb_percent((64, 32, 16))
    ('25%', '12.5%', '6.25%')

    """
    # In order to maintain precision for common values,
    # 256 / 2**n is special-cased for values of n
    # from 0 through 4, as well as 0 itself.
    specials = {255: '100%', 128: '50%', 64: '25%',
                32: '12.5%', 16: '6.25%', 0: '0%'}
    return tuple(map(lambda d: specials.get(d, '%.02f%%' % ((d / 255.0) * 100)),
                     rgb_triplet))


######################################################################
# Conversions from percentage rgb() triplets to various formats.
######################################################################


def rgb_percent_to_name(rgb_percent_triplet, spec='css3'):
    """
    Convert a 3-tuple of percentages, suitable for use in an ``rgb()``
    color triplet, to its corresponding normalized color name, if any
    such name exists.

    The optional keyword argument ``spec`` determines which
    specification's list of color names will be used; valid values are
    ``html4``, ``css2``, ``css21`` and ``css3``, and the default is
    ``css3``.

    If there is no matching name, ``ValueError`` is raised.

    Examples:

    >>> rgb_percent_to_name(('0%', '0%', '0%'))
    'black'
    >>> rgb_percent_to_name(('0%', '0%', '50%'))
    'navy'
    >>> rgb_percent_to_name(('85.49%', '64.71%', '12.5%'))
    'goldenrod'

    """
    return rgb_to_name(rgb_percent_to_rgb(rgb_percent_triplet), spec=spec)


def rgb_percent_to_hex(rgb_percent_triplet):
    """
    Convert a 3-tuple of percentages, suitable for use in an ``rgb()``
    color triplet, to a normalized hexadecimal color value for that
    color.

    Examples:

    >>> rgb_percent_to_hex(('100%', '100%', '0%'))
    '#ffff00'
    >>> rgb_percent_to_hex(('0%', '0%', '50%'))
    '#000080'
    >>> rgb_percent_to_hex(('85.49%', '64.71%', '12.5%'))
    '#daa520'

    """
    return rgb_to_hex(rgb_percent_to_rgb(rgb_percent_triplet))


def _percent_to_integer(percent):
    """
    Internal helper for converting a percentage value to an integer
    between 0 and 255 inclusive.

    """
    num = float(percent.split('%')[0]) / 100.0 * 255
    e = num - math.floor(num)
    return e < 0.5 and int(math.floor(num)) or int(math.ceil(num))


def rgb_percent_to_rgb(rgb_percent_triplet):
    """
    Convert a 3-tuple of percentages, suitable for use in an ``rgb()``
    color triplet, to a 3-tuple of integers suitable for use in
    representing that color.

    Some precision may be lost in this conversion. See the note
    regarding precision for ``rgb_to_rgb_percent()`` for details;
    generally speaking, the following is true for any 3-tuple ``t`` of
    integers in the range 0...255 inclusive::

        t == rgb_percent_to_rgb(rgb_to_rgb_percent(t))

    Examples:

    >>> rgb_percent_to_rgb(('100%', '100%', '100%'))
    (255, 255, 255)
    >>> rgb_percent_to_rgb(('0%', '0%', '50%'))
    (0, 0, 128)
    >>> rgb_percent_to_rgb(('25%', '12.5%', '6.25%'))
    (64, 32, 16)
    >>> rgb_percent_to_rgb(('12.94%', '21.96%', '75.29%'))
    (33, 56, 192)

    """
    return tuple(map(_percent_to_integer, rgb_percent_triplet))


def whatever_to_rgb(string):
    """
    Converts CSS3 color or a hex into rgb triplet; hash of string if fails.
    """
    string = string.strip().lower()
    try:
        return name_to_rgb(string)
    except ValueError:
        try:
            return hex_to_rgb(string)
        except ValueError:
            try:
                if string[:3] == "rgb":
                    return tuple([float(i) for i in string[4:-1].split(",")][0:3])
            except:
                return hex_to_rgb("#" + md5(string).hexdigest()[:6])


def whatever_to_hex(string):
    if type(string) == tuple:
        return cairo_to_hex(string)
    return rgb_to_hex(whatever_to_rgb(string))


def whatever_to_cairo(string):
    a = whatever_to_rgb(string)
    return a[0] / 255., a[1] / 255., a[2] / 255.,


def cairo_to_hex(cairo):
    return rgb_to_hex((cairo[0] * 255, cairo[1] * 255, cairo[2] * 255,))


if __name__ == '__main__':
    import doctest
    doctest.testmod()
