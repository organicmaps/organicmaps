# Driving palettes

The default and vehicle style families share the day and night palettes in this
directory. Each family imports its palette after the original color definitions
and imports its road-color overrides after the geometry rules. The outdoors
family keeps its existing palette.

The palettes define land, water, vegetation, buildings, roads, labels and shields.
Road colors vary with map zoom to keep major roads distinct at overview scales.
The road overrides change color only and retain the existing widths, casing,
opacity, priorities and visibility. Zooms above 19 use the zoom-19 color. Links
and tunnels follow their road class.

Route and traffic colors, together with the position-arrow colors, are defined
in each family's `style.mapcss`. The route uses a green fill and darker green
outline; the position arrow uses a yellow fill and pale outline. These are
presentation changes and do not add a traffic-data source.

Regenerate all drawing rules and color palettes from the repository root:

```sh
tools/unix/generate_drules.sh
```

Keep generated drawing-rule binaries, text dumps and `colors.txt` in a separate
commit. Generated files must not be edited by hand. The style changes require
no application or rendering-engine changes and use the existing map geometry,
labels, symbols and fonts.
