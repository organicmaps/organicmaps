
# Change Log

All notable changes to this project will be documented in this file.
This project adheres to [Semantic Versioning](http://semver.org/).

## [unreleased] -

## [2.1.0] - 2015-03-31

### Added

- When writing PBF files, sorting the PBF stringtables is now optional.
- More tests and documentation.

### Changed

- Some functions are now declared `noexcept`.
- XML parser fails now if the top-level element is not `osm` or `osmChange`.

### Fixed

- Race condition in PBF reader.
- Multipolygon collector was accessing non-existent NodeRef.
- Doxygen documentation wan't showing all classes/functions due to a bug in
  Doxygen (up to version 1.8.8). This version contains a workaround to fix
  this.

[unreleased]: https://github.com/osmcode/libosmium/compare/v2.1.0...HEAD
[2.1.0]: https://github.com/osmcode/libosmium/compare/v2.0.0...v2.1.0

