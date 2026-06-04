# Organic Maps API and deep links

Organic Maps supports `om://` deep links (and corresponding `https://omaps.app/...` links) for showing points, search, route planning, and position picking.

## Route links v2

Use v2 route links for multi-stop routes and navigation-app integrations:

- `om://v2/dir?...` previews/builds a route. It honors an explicit `origin`.
- `om://v2/nav?...` builds the route and starts navigation after it is ready,
  always from the current position ("navigate to these places starting from
  me"). An explicit `origin=lat,lon` is optional and currently ignored here
  (accepted and reserved for possible future use as a routing hint); to preview a
  route from a specific start point use `/v2/dir` instead.

Required parameter:

- `destination=lat,lon`

Optional parameters:

- `origin=lat,lon` is optional. For `/v2/dir` it sets the start point; when
  omitted, Organic Maps routes from the current position. `/v2/nav` always
  navigates from the current position, so it accepts but ignores `origin` for now;
  the parameter is reserved as a possible future routing hint, so including it is
  safe and never fails a `/v2/nav` request.
- `origin_heading=degrees` to prefer the initial road direction from the origin. Degrees are clockwise from north:
  `0` north, `90` east, `180` south, `270` west.
- `waypoints=lat,lon|lat,lon|...` for intermediate stops in URL order.
- `origin_name=...`, `destination_name=...`, `waypoint_names=name|name|...`.
- `origin_callback=...`, `destination_callback=...`, `waypoint_callbacks=url|url|...` for caller-specific stop callbacks.
  Organic Maps opens a stop callback when the corresponding route point is passed while the app is in the
  foreground. A stop passed while Organic Maps is in the background is delivered the next time it returns to
  the foreground; delivery is best-effort, so make callbacks idempotent and do not rely on every intermediate
  stop opening when the app is not visible (for example on a locked screen).
  Stop callbacks are aligned to `waypoints` by position; supply fewer to leave later stops without a callback.

The pipe-separated lists (`waypoints`, `waypoint_names`, `waypoint_callbacks`) accept either a literal `|` or its
percent-encoded form `%7C` as the item separator, so a builder that percent-encodes the whole query value still
splits correctly. A literal `|` that belongs inside a value must be double-encoded as `%257C`: it decodes once to
the text `%7C` and is not treated as a separator.
- `mode=drive|walk|bike|transit` (`drive` is the default). For easier migration from Google Maps URLs, `travelmode=driving|walking|bicycling|transit` is accepted as an alias.
- `dir_action=navigate` on `/v2/dir` is accepted as a Google Maps-style alias for `/v2/nav`.
- `optimize=true` to allow Organic Maps to reorder intermediate stops; otherwise URL order is preserved.
- `ref_name=...` for the calling app title (`appname` equivalent) and `callback=...` for a global return URL.
- Future extension parameters such as `api=1`, `avoid=...`, `ref=...`, and `callback_label=...` are accepted without failing the whole route.

Explicit-origin preview example:

```text
om://v2/dir?origin=52.5200,13.4050&origin_name=Warehouse%20Berlin&destination=52.5163,13.3777&destination_name=Warehouse%20Berlin%20(Return)&waypoints=52.5304,13.3850|52.5450,13.3920|52.5612,13.4150&waypoint_names=Anna%20Schmidt|Bauer%20GmbH|M%C3%BCller%20Family&mode=drive&avoid=tolls&ref_name=DeliveryCo%20Driver&callback=delivery%3A%2F%2Fjob%2F4521%2Freturn
```

Current-position navigation example:

```text
om://v2/nav?destination=52.5163,13.3777&destination_name=Warehouse%20Berlin%20(Return)&waypoints=52.5304,13.3850|52.5450,13.3920|52.5612,13.4150&waypoint_names=Anna%20Schmidt|Bauer%20GmbH|M%C3%BCller%20Family&mode=drive&avoid=tolls&ref_name=DeliveryCo%20Driver&callback=delivery%3A%2F%2Fjob%2F4521%2Freturn
```

Equivalent HTTPS form:

```text
https://omaps.app/v2/dir?destination=47.38568,8.566878&waypoints=47.395084,8.552692|47.3890,8.5580&mode=bike
```

## Legacy two-point route links

The previous route format remains supported for backward compatibility:

```text
om://route?v=1&sll=50.183933,8.942871&saddr=Start%20Point&dll=49.998912,8.278198&daddr=End%20Point&type=vehicle
```

Legacy route links also accept the common `appname=...` and `cll=lat,lon` parameters. Use v2 links for caller callbacks: legacy route links ignore `callback=...` and `backurl=...`.

Do not mix the legacy `sll`/`saddr`/`dll`/`daddr` parameters with the v2 `/v2/dir` or `/v2/nav` syntax.
