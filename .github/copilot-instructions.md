# Agent Instructions for Organic Maps

Organic Maps is a privacy-focused offline maps & GPS app built on OpenStreetMap data. 
This is a C++23 cross-platform project with iOS (Swift 5, ObjectiveC), Android (Java 17), and desktop (Qt 6) frontends.

## Code Style (C++)
- Format rules described in `.clang-format` 
- C++23 standard, `.hpp/.cpp` files (not `.h/.cc`)
- `#pragma once` for headers
- 2-space indent, 120 char line width
- Member variables: `m_` prefix (e.g., `m_countryFile`)
- `const` after the type: `auto const & ref = f();` 
- Namespaces: `lower_case` with underscores
- `using` instead of `typedef`
- Compile-time constants: `kCamelCase` and `constexpr`
- Auto-format: `clang-format -i file.cpp`

## Main focus
- On performance
- Unit tests covering most/all corner cases or changes
- Less code/cleaner code/less changes

## When translating content from English:
- "bookmark" or "bookmarks" (favorite place, saved by user) , use word "метка" or "метки" for Russian, use word "мітка" or "міткі" for Ukrainian
- "track" (recorded path on the map that user walked) to Russian, use word "трек"
- "route" to Russian, use word "маршрут"
- "icon" or "icons" (place's image symbol on the map), use word "иконка" или "иконки" for Russian
- "outdoors" or "outdoors style" or "outdoors map style" use "стиль для активного отдыха" for Russian, use "режим Активний відпочинок" for Ukrainian.
- "map" use "мапа" for Ukrainian and Belarusian.
- use "…" instead of "..."
- use ё instead е in Russian where applicable
- do not translate "Organic Maps" and "ID Editor"
- do not replace amounts like 5K with zeroes (5.000), either leave it (if it is a normal language practice) or use "5 thousands" equivalent (like 5 тыс. in Russian)
