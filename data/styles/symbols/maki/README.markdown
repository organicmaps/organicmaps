# Maki 

Maki is a point of interest icon set made especially for use with MapBox maps. For more information on how to use Maki with MapBox, see documentation at http://mapbox.com/tilemill/docs/guides/using-maki-icons/.

## src

Maki's source [SVG][] files are in the `src` subdirectory. To create pixel-perfect icons at different sizes, each icon is designed 3 times for 12, 18, and 24 pixels wide/tall. 

Maki is designed using [Inkscape][]. For information on contributing to Maki see CONTRIBUTING.md.

## renders

PNG renders of all of the SVGs are in the `renders` directory. High-resolution (aka Retina) versions of each icon are present as well, named using the common `@2x` convention.

## ArcGIS

Style files for ArcGIS 10.1+ are in the `ArcGIS` subdirectory and are maintained by @williamscraigm. Both Desktop (.style) and Server (.ServerStyle) versions are provided.

Currently only PNG based picture marker symbols are contained in these styles. In addition to keeping up to date with maki symbols, the following roadmap is planned:

- vector versions created from SVG source
- vector versions created from SVG source with added white outline
- representation marker versions of the two above vector versions

## render.sh

You can use the SVGs and PNGs in this repository as they are without building anything, however a render script is included to assist designers/developers who want to modify or create Maki icons. It will render SVGs to PNGs at 100% and 200% resolution, create sprites used for the Maki website, and generate corresponding CSS styles for the sprites.

The script requires [Bash][], [Inkscape][], and [ImageMagick][] to function correctly. Each icon must have an appropriate entry in `www/maki.json` to be rendered correctly.

[SVG]: http://en.wikipedia.org/wiki/Scalable_Vector_Graphics
[Inkscape]: http://inkscape.org
[Bash]: http://www.gnu.org/software/bash/bash.html
[ImageMagick]: http://www.imagemagick.org/

## Versioning

Maki uses a semantic versioning scheme.

* 0.0.z: bugfixes, modifications
* 0.y.0: icons added
* x.0.0: icons removed, sprite scheme changed, or major features added
