import math

def to_rgb(color_str):
    if len(color_str) != 6:
        return (0, 0, 0)
    r = int(color_str[0:2], 16)
    g = int(color_str[2:4], 16)
    b = int(color_str[4:], 16)
    return (r, g, b)


def blend_colors(rgb_array1, rgb_array2, k):
    return (rgb_array1[0] * (1.0 - k) + rgb_array2[0] * k,
            rgb_array1[1] * (1.0 - k) + rgb_array2[1] * k, 
            rgb_array1[2] * (1.0 - k) + rgb_array2[2] * k)


def rgb_pivot(n):
    result = n / 12.92
    if n > 0.04045:
        result = ((n + 0.055) / 1.055) ** 2.4
    return result * 100.0;


def to_xyz(rgb_array):
    r = rgb_pivot(rgb_array[0] / 255.0);
    g = rgb_pivot(rgb_array[1] / 255.0);
    b = rgb_pivot(rgb_array[2] / 255.0);
    return (r * 0.4124 + g * 0.3576 + b * 0.1805,
            r * 0.2126 + g * 0.7152 + b * 0.0722,
            r * 0.0193 + g * 0.1192 + b * 0.9505)


#https://en.wikipedia.org/wiki/Lab_color_space#CIELAB
def lab_pivot(n):
    if n > 0.008856:
        return n ** (1.0/3.0)
    return (903.3 * n + 16.0) / 116.0


def to_lab(rgb_array):
    xyz = to_xyz(rgb_array)
    x = lab_pivot(xyz[0] / 95.047)
    y = lab_pivot(xyz[1] / 100.0)
    z = lab_pivot(xyz[2] / 108.883)
    l = 116.0 * y - 16.0
    if l < 0.0:
        l = 0.0
    a = 500.0 * (x - y)
    b = 200.0 * (y - z)
    return (l, a, b)


def lum_distance(ref_color, src_color):
    return 30 * (ref_color[0] - src_color[0]) ** 2 +\
           59 * (ref_color[1] - src_color[1]) ** 2 +\
           11 * (ref_color[2] - src_color[2]) ** 2


def is_bluish(rgb_array):
    d1 = lum_distance((255, 0, 0), rgb_array)
    d2 = lum_distance((0, 0, 255), rgb_array)
    return d2 < d1


#http://en.wikipedia.org/wiki/Color_difference#CIE94
def cie94(ref_color, src_color):
    lab_ref = to_lab(ref_color)
    lab_src = to_lab(src_color)
    deltaL = lab_ref[0] - lab_src[0]
    deltaA = lab_ref[1] - lab_src[1]
    deltaB = lab_ref[2] - lab_src[2]
    c1 = math.sqrt(lab_ref[0] * lab_ref[0] + lab_ref[1] * lab_ref[1])
    c2 = math.sqrt(lab_src[0] * lab_src[0] + lab_src[1] * lab_src[1])
    deltaC = c1 - c2
    deltaH = deltaA * deltaA + deltaB * deltaB - deltaC * deltaC
    if deltaH < 0.0:
        deltaH = 0.0
    else:
        deltaH = math.sqrt(deltaH)
    # cold tones if a color is more bluish.
    Kl = 1.0
    K1 = 0.045
    K2 = 0.015
    sc = 1.0 + K1 * c1
    sh = 1.0 + K2 * c1
    deltaLKlsl = deltaL / Kl
    deltaCkcsc = deltaC / sc
    deltaHkhsh = deltaH / sh
    i = deltaLKlsl * deltaLKlsl + deltaCkcsc * deltaCkcsc + deltaHkhsh * deltaHkhsh
    if i < 0:
        return 0.0
    return math.sqrt(i)


class Palette:
    def __init__(self, colors):
        self.colors = {}
        for name, color_info in colors['colors'].items():
            self.colors[name] = to_rgb(color_info['clear'])

    def get_default_color(self):
        return 'default'

    def get_nearest_color(self, color_str, casing_color_str, excluded_names):
        """Returns the nearest color from the palette."""
        nearest_color_name = self.get_default_color()
        color = to_rgb(color_str)
        if (casing_color_str is not None and len(casing_color_str) != 0):
            color = blend_colors(color, to_rgb(casing_color_str), 0.5)
        min_diff = None

        bluish = is_bluish(color)
        for name, palette_color in self.colors.items():
            # Uncomment if you want to exclude duplicates.
            #if name in excluded_names:
            #    continue
            if bluish:
                diff = lum_distance(palette_color, color)
            else:
                diff = cie94(palette_color, color)
            if min_diff is None or diff < min_diff:
                min_diff = diff
                nearest_color_name = name
        # Left here for debug purposes. 
        #print("Result: " + color_str + "," + str(casing_color_str) +
        #      " - " + nearest_color_name + ": bluish = " + str(bluish))
        return nearest_color_name
