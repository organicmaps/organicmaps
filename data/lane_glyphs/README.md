# Lane glyph paths

`tools/python/generate_lane_glyphs.py` generates these SVG components and the
matching Android VectorDrawables from one 64×64 coordinate set. Run it after
changing a path; use `--check` to verify generated files.

Android keeps the existing bitmap artwork for single-direction lanes. These
components are composed only when a lane allows multiple directions.

Each composite lane draws its allowed branches, one shared stem, then its
recommended branch. Draw inactive branches first so the recommended branch
and stem remain clear. Apply translucent tint once to each completed branch
group; tinting overlapping path pieces separately makes a visible dot at
the junction.
`left_inner` and `right_inner` leave space for an accompanying U-turn.
The SVG components can also be converted to native vector assets or paths for
iOS and desktop UIs. Colors and sizing belong to each platform's UI.

OSM `reverse` does not say which side to draw. If neither side is recommended,
the Android renderer keeps the existing left-side fallback. It must not draw
both U-turn branches. Choosing a side from local driving rules would require
passing driving-side information into the lane widget.
