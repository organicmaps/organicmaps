# Independent points in the map API

`om://map` links normally match each coordinate to a nearby or enclosing map
feature. A supplied `n` replaces the displayed name, while the matched feature's
classification and metadata remain available. This also preserves the behavior of
Organic Maps' own shared coordinate/name links.

A caller importing an independently identified place can append `match=none` after
that point's `ll` to display its supplied coordinates and name without matching an
OpenStreetMap feature:

```text
om://map?v=1&ll=40.2113819,19.5793909&n=Restorant%20Iliria%20Llogora&match=none
```

This prevents an imported restaurant from inheriting an enclosing park's metadata,
including when a different POI or building happens to occupy the same coordinate.
An unnamed point is shown with the normal coordinate-based title. The point's
`id` and the request's `backurl` retain their existing meanings.

The option applies only to the preceding `ll`, like `n` and `id`. Each new point
starts with matching enabled, so one request can mix policies:

```text
om://map?ll=1,1&n=Independent&match=none&ll=2,2&n=Meeting%20point
```

`match` before any `ll` makes the request invalid. Only the value `none` disables
matching; other values are ignored. The policy stays with the API pin when it is
selected again. It is not serialized into saved bookmarks or subsequent shares.

Existing links without this option, including generic `geo:` links and native
Organic Maps shares, retain feature matching. An importer must explicitly build
the map API link from its resolved coordinates and URL-encoded name. A resolver's
existing `geo:` redirect does not opt in. Older Organic Maps versions ignore the
new option and continue matching features.
