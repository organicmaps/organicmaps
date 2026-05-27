---
name: add-category
description: "Add a new search category (OSM type with translations, synonyms, and emoji) to data/categories.txt. Use when asked to add, create, or insert a search category, search synonym, or map feature type."
---

# Add Search Category

## 1. Gather Requirements

Ask the user for:
- **OSM type(s)** — e.g. `shop-leather`, `amenity-cafe`. Multiple types separated by `|`
- **Group(s)** — which `@group` to attach (e.g. `@shop`, `@category_eat`). Grep `^@` in `data/categories.txt` for existing groups
- **Emoji** (optional) — `U+XXXX` Unicode format, placed as synonyms on the `en:` line

## 2. Ensure Translations Exist in types_strings.txt

Convert OSM type dashes to dots: `shop-leather` → `[type.shop.leather]`.

**Check** if the entry exists in `data/strings/types_strings.txt`.

### If absent — create translations first:
1. Ask user for the **English name** and **translation context** (what this place/thing is)
2. Run translation tool:
   ```
   python3 tools/python/translate.py --context "context here" "English Name"
   ```
   This outputs translations in both `categories.txt` and `strings.txt` formats via DeepL+Google
3. **Proof-read** the generated translations in all languages using the provided context — fix obvious errors
4. Add the `[type.osm.type]` entry to `data/strings/types_strings.txt`:
   ```
   [type.shop.leather]
   comment = context used for translation
   en = Leather Shop
   ar = متجر السلع الجلدية
   ...sorted alphabetically by language code...
   zh-Hant = 皮具店
   ```

### If present — read existing translations to use as the base.

## 3. Build the categories.txt Entry

### First translation = types_strings.txt match
The **first synonym** for each language MUST exactly match the translation in `types_strings.txt`. This is a hard requirement.

### Additional synonyms
Append popular search synonyms after `|` — only terms users would actually type in the search box. Keep it minimal.

### Prefix digits
A digit `1`-`9` before a synonym controls the minimum characters needed for the suggestion to appear:
- `3Cafe` → user must type 3+ chars
- No prefix → full text required

### Slavic language rule
For **ru, uk, be**: short nouns (under 6 letters) need both nominative and genitive forms:
```
ru:Вино|вина
```
For **sr**: threshold is 8 letters. Longer nouns rely on error correction.

## 4. Entry Format

```
osm-type1|osm-type2|@group1|@group2
en:4Type Name|synonym|U+1F6B0
ar:Arabic Name
be:Belarusian Name
...languages sorted alphabetically by code...
zh-Hant:Traditional Chinese
```

- `en:` line always **first** after the header
- Other languages sorted **alphabetically** by language code
- Each entry ends with a **blank line**
- Insert near related categories (same group or alphabetically by OSM type)
- Supported language codes: ar, be, bg, ca, cs, da, de, el, en, es, et, eu, fa, fi, fr, he, hi, hu, id, it, ja, ko, lt, lv, mr, nb, nl, pl, pt, pt-BR, ro, ru, sk, sl, sr, sv, sw, th, tr, uk, vi, zh-Hans, zh-Hant

## 5. Displayed Category (@category_*)

Only if adding a category shown in the app's search screen. Requires **three** coordinated file changes:

### a) `data/categories.txt`
```
# First keyword should match [category_name] definition in strings.txt!
@category_name
en:Display Name|synonym
...translations...
```

### b) `data/strings/strings.txt`
```
[category_name]
comment = Search category for ...; any changes should be duplicated in categories.txt @category_name!
tags = android-app,android-libs-car,apple-maps
en = Display Name
...translations for all supported locales...
```
The `en =` value must match the first keyword on the `en:` line (without prefix digit).

### c) `libs/search/displayed_categories.cpp`
Add `"category_name"` to the `m_keys` initializer list.

## 6. After Editing

1. **Sort languages:**
   ```
   python3 tools/python/sort_categories.py --in-place
   ```

2. **Build and run tests:**
   ```
   cmake --build build-agent --target search_tests && ctest -j --test-dir build-agent --stop-on-failure --output-on-failure -R search_tests
   ```

3. **If a displayed category was added**, also run:
   ```
   cmake --build build-agent --target indexer_tests editor_tests && ctest -j --test-dir build-agent --stop-on-failure --output-on-failure -R "indexer_tests|editor_tests"
   ```
