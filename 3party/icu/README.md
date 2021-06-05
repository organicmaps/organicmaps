# Building and using icu

To use icu code, define UCONFIG_USE_LOCAL and add 3 include search paths:

1. $(OMIM_ROOT)/3party/icu
2. $(OMIM_ROOT)/3party/icu/icu/icu4c/source/common
3. $(OMIM_ROOT)/3party/icu/icu/icu4c/source/i18n

Only necessary sources are included for bidi and transliteration.
Please add other sources if you need more functionality.

## Example how to build filtered data file

```
ICU_DATA_FILTER_FILE=$(PWD)/icu_filter.json $(PWD)/icu/icu4c/source/./configure --disable-shared --enable-static --disable-renaming --disable-extras --disable-icuio --disable-tests --disable-samples --with-data-packaging=archive
```
