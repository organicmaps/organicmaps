Web colors
==========

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

Normalization functions
-----------------------

``normalize_hex(hex_value)``
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
    
    Example:
    
    >>> normalize_hex('#09c')
    '#0099cc'

Conversions from named colors
-----------------------------

``name_to_hex(name, spec='css3')``
    Convert a color name to a normalized hexadecimal color value.
    
    The optional keyword argument ``spec`` determines which
    specification's list of color names will be used; valid values are
    ``html4``, ``css2``, ``css21`` and ``css3``, and the default is
    ``css3``.
    
    The color name will be normalized to lower-case before being
    looked up, and when no color of that name exists in the given
    specification, ``ValueError`` is raised.
    
    Example:
    
    >>> name_to_hex('deepskyblue')
    '#00bfff'

``name_to_rgb(name, spec='css3')``
    Convert a color name to a 3-tuple of integers suitable for use in
    an ``rgb()`` triplet specifying that color.
    
    The optional keyword argument ``spec`` determines which
    specification's list of color names will be used; valid values are
    ``html4``, ``css2``, ``css21`` and ``css3``, and the default is
    ``css3``.
    
    The color name will be normalized to lower-case before being
    looked up, and when no color of that name exists in the given
    specification, ``ValueError`` is raised.
    
    Example:
    
    >>> name_to_rgb('navy')
    (0, 0, 128)

``name_to_rgb_percent(name, spec='css3')``
    Convert a color name to a 3-tuple of percentages suitable for use
    in an ``rgb()`` triplet specifying that color.
    
    The optional keyword argument ``spec`` determines which
    specification's list of color names will be used; valid values are
    ``html4``, ``css2``, ``css21`` and ``css3``, and the default is
    ``css3``.
    
    The color name will be normalized to lower-case before being
    looked up, and when no color of that name exists in the given
    specification, ``ValueError`` is raised.
    
    Example:
    
    >>> name_to_rgb_percent('navy')
    ('0%', '0%', '50%')


Conversions from hexadecimal values
-----------------------------------

``hex_to_name(hex_value, spec='css3')``
    Convert a hexadecimal color value to its corresponding normalized
    color name, if any such name exists.
    
    The optional keyword argument ``spec`` determines which
    specification's list of color names will be used; valid values are
    ``html4``, ``css2``, ``css21`` and ``css3``, and the default is
    ``css3``.
    
    The hexadecimal value will be normalized before being looked up,
    and when no color name for the value is found in the given
    specification, ``ValueError`` is raised.
    
    Example:
    
    >>> hex_to_name('#000080')
    'navy'

``hex_to_rgb(hex_value)``
    Convert a hexadecimal color value to a 3-tuple of integers
    suitable for use in an ``rgb()`` triplet specifying that color.
    
    The hexadecimal value will be normalized before being converted.
    
    Example:
    
    >>> hex_to_rgb('#000080')
    (0, 0, 128)

``hex_to_rgb_percent(hex_value)``
    Convert a hexadecimal color value to a 3-tuple of percentages
    suitable for use in an ``rgb()`` triplet representing that color.
    
    The hexadecimal value will be normalized before converting.
    
    Example:

    >>> hex_to_rgb_percent('#ffffff')
    ('100%', '100%', '100%')


Conversions from integer rgb() triplets
---------------------------------------

``rgb_to_name(rgb_triplet, spec='css3')``
    Convert a 3-tuple of integers, suitable for use in an ``rgb()``
    color triplet, to its corresponding normalized color name, if any
    such name exists.
    
    The optional keyword argument ``spec`` determines which
    specification's list of color names will be used; valid values are
    ``html4``, ``css2``, ``css21`` and ``css3``, and the default is
    ``css3``.
    
    If there is no matching name, ``ValueError`` is raised.
    
    Example:
    
    >>> rgb_to_name((0, 0, 0))
    'black'

``rgb_to_hex(rgb_triplet)``
    Convert a 3-tuple of integers, suitable for use in an ``rgb()``
    color triplet, to a normalized hexadecimal value for that color.
    
    Example:
    
    >>> rgb_to_hex((255, 255, 255))
    '#ffffff'

``rgb_to_rgb_percent(rgb_triplet)``
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


Conversions from percentage rgb() triplets
------------------------------------------

``rgb_percent_to_name(rgb_percent_triplet, spec='css3')``
    Convert a 3-tuple of percentages, suitable for use in an ``rgb()``
    color triplet, to its corresponding normalized color name, if any
    such name exists.
    
    The optional keyword argument ``spec`` determines which
    specification's list of color names will be used; valid values are
    ``html4``, ``css2``, ``css21`` and ``css3``, and the default is
    ``css3``.
    
    If there is no matching name, ``ValueError`` is raised.
    
    Example:
    
    >>> rgb_percent_to_name(('0%', '0%', '50%'))
    'navy'

``rgb_percent_to_hex(rgb_percent_triplet)``
    Convert a 3-tuple of percentages, suitable for use in an ``rgb()``
    color triplet, to a normalized hexadecimal color value for that
    color.
    
    Example:

    >>> rgb_percent_to_hex(('100%', '100%', '0%'))
    '#ffff00'

``rgb_percent_to_rgb(rgb_percent_triplet)``
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
