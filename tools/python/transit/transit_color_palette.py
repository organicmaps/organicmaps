def to_rgb(color_str):
    if len(color_str) != 6:
        return (0, 0, 0)
    r = int(color_str[0:2], 16)
    g = int(color_str[2:4], 16)
    b = int(color_str[4:], 16)
    return (r, g, b)


class Palette:
    def __init__(self, colors):
        self.colors = []
        for color in colors['colors']:
            color_info = {'name': color['name'],
                          'clear': to_rgb(color['clear'])}
            self.colors.append(color_info)

    def get_default_color(self):
        return self.colors[0]['name']

    def get_nearest_color(self, color_str):
        """Returns the nearest color from the palette."""
        nearest_color_info = None
        color = to_rgb(color_str)
        min_diff = None
        for color_info in self.colors:
            palette_color = color_info['clear']
            diff = 30 * (palette_color[0] - color[0]) ** 2 +\
                   59 * (palette_color[1] - color[1]) ** 2 +\
                   11 * (palette_color[2] - color[2]) ** 2
            if min_diff is None or diff < min_diff:
                min_diff = diff
                nearest_color_info = color_info
        return nearest_color_info['name']
