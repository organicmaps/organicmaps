def to_rgb(color_str):
    if len(color_str) != 6:
        return (0, 0, 0)
    r = int(color_str[0:2], 16)
    g = int(color_str[2:4], 16)
    b = int(color_str[4:], 16)
    return (r, g, b)


class Palette:
    def __init__(self, colors):
        self.colors = {}
        for name, color_info in colors['colors'].iteritems():
            self.colors[name] = to_rgb(color_info['clear'])

    def get_default_color(self):
        return 'default'

    def get_nearest_color(self, color_str):
        """Returns the nearest color from the palette."""
        nearest_color_name = self.get_default_color()
        color = to_rgb(color_str)
        min_diff = None
        for name, palette_color in self.colors.iteritems():
            diff = 30 * (palette_color[0] - color[0]) ** 2 +\
                   59 * (palette_color[1] - color[1]) ** 2 +\
                   11 * (palette_color[2] - color[2]) ** 2
            if min_diff is None or diff < min_diff:
                min_diff = diff
                nearest_color_name = name
        return nearest_color_name
