transit_palette = ["000000",
                   "ff0000",
                   "00ff00",
                   "0000ff"]


def to_rgb(color_str):
    if len(color_str) != 6:
        return (0, 0, 0)
    r = int('0x' + color_str[0:2], 16)
    g = int('0x' + color_str[2:4], 16)
    b = int('0x' + color_str[4:], 16)
    return (r, g, b)


class Palette:
    def __init__(self, colors):
        self.colors = {}
        for color in colors:
            self.colors[color] = to_rgb(color)

    def get_nearest_color(self, color_str):
        """Returns the nearest color from the palette."""
        nearest_color = None
        color = to_rgb(color_str)
        min_diff = None
        for palette_color in self.colors.values():
            diff = 30 * (palette_color[0] - color[0]) ** 2 +\
                   59 * (palette_color[1] - color[1]) ** 2 +\
                   11 * (palette_color[2] - color[2]) ** 2
            if min_diff is None or diff < min_diff:
                min_diff = diff
                nearest_color = palette_color
        return nearest_color
