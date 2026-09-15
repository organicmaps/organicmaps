# Organic Maps API and deep links

Organic Maps supports `om://` deep links (and corresponding `https://omaps.app/...` links) for showing points, search, route planning, and position picking.

## Route links v2

Use v2 route links for multi-stop routes and navigation-app integrations:

- `om://v2/dir?...` previews/builds a route. It honors an explicit `origin`.
- `om://v2/nav?...` builds the route and starts navigation after it is ready,
  always from the current position. An explicit `origin=lat,lon` is accepted but ignored;
  to preview a route from a specific start point use `/v2/dir` instead.

Required parameter:

- `destination=lat,lon`

On iOS, opening a route link transfers routing to the phone if CarPlay owns the map. The old CarPlay trip
presentation is cancelled; the phone displays the new preview or starts navigation according to the link.

Optional parameters:

- `origin=lat,lon` is optional. For `/v2/dir` it sets the start point; use
  `origin=currentLocation` / `current-location` (or omit `origin`) to route from the current
  position. `/v2/nav` accepts but ignores `origin`, including malformed values, because
  navigation starts from the current position.
- `origin_heading=degrees` in the inclusive range `0`–`360` to prefer the initial road direction from the origin. Degrees are clockwise from north:
  `0` north, `90` east, `180` south, `270` west.
- `waypoints=lat,lon|lat,lon|...` for intermediate stops in URL order. Empty slots are skipped.
- `origin_name=...`, `destination_name=...`, `waypoint_names=name|name|...`.
- `destination_callback=...` and `waypoint_callbacks=url|url|...` for caller-specific stop callbacks.
  Organic Maps opens a stop callback when the corresponding route point is passed during navigation while the
  app is in the foreground. Preview tracking does not open callbacks. Stops passed while Organic Maps is in the
  background are deferred until it returns to the foreground, and only the most recent pending callback
  is opened then; delivery is best-effort, so make
  callbacks idempotent and do not rely on every intermediate stop opening when the app is not visible
  (for example on a locked screen).
  Replacing or cancelling an unfinished itinerary discards its pending callback; automatic completion retains
  the final callback for delivery when the app returns to the foreground.
  `waypoint_names` and `waypoint_callbacks` are aligned to `waypoints` by position (including empty `||` slots);
  supply fewer to leave later stops unnamed or without a callback, and a name or callback that lines up with an
  empty waypoint slot is dropped.

The pipe-separated lists (`waypoints`, `waypoint_names`, `waypoint_callbacks`) accept either a literal `|` or its
percent-encoded form `%7C` as the item separator, so a builder that percent-encodes the whole query value still
splits correctly. A literal `|` that belongs inside a value must be double-encoded as `%257C`: it decodes once to
the text `%7C` and is not treated as a separator.

Query values are URL-decoded once before parsing. Percent-encode the complete callback URL as a query value,
including any existing percent escapes. Unencoded `&` and `#` end the value; a callback's literal `+` must be
encoded as `%2B` to preserve it. Surrounding whitespace on each list item is trimmed.
Repeated parameters use the last value; `mode` and `travelmode` share the same value.

- `mode=drive|walk|bike|transit` (`drive` is the default; `driving`/`car`/`vehicle`, `walking`/`pedestrian`, and `bicycling`/`bicycle` are also accepted, but not `cycling`). `travelmode=...` is an interchangeable alias for easier migration from Google Maps URLs.
- `dir_action=navigate` on `/v2/dir` is accepted as a Google Maps-style alias for `/v2/nav`.
- `optimize=true` to allow Organic Maps to reorder intermediate stops; otherwise URL order is preserved.
- `ref_name=...` names the calling app (the `appname` equivalent; accepted and parsed, but not yet surfaced in the route UI) and `callback=...` is a global return URL.
  On both platforms, use the map menu's Back action, or the Back action under the navigation settings button,
  to open the return URL. Returning does not stop navigation. Opening settings, switching applications, or
  stopping navigation does not automatically open a v2 return URL. A successful return launch clears it.
  On iOS the Escape keyboard shortcut also opens it.
  As with stop callbacks, delivery is best-effort: the pending return URL does not survive an app restart.
- Any unrecognized parameter is ignored with a warning and never fails the route, so the format stays
  forward-compatible. Named-but-not-yet-applied extensions include `api=1`, `avoid=...` (e.g. `avoid=tolls` in the
  examples below), `ref=...`, and `callback_label=...`; they are accepted today but have no effect yet.
  `origin_callback=...` is also reserved: it is parsed but not opened, because the route start is departed from,
  never "passed" like a stop.

A whole route link is rejected (nothing is built) for the following errors:
no `destination`, a malformed `destination`/`waypoints` coordinate, a malformed `origin` in preview mode,
an out-of-range `origin_heading`, an unknown `mode`/`travelmode`, or more than 100 nonempty `waypoints`.
Navigation requests (`/v2/nav`, or `/v2/dir` with `dir_action=navigate`) ignore malformed `origin` values too.
Unrecognized *parameters*, in contrast, only warn. A successfully parsed request can still fail during route
building, for example when the current position is unavailable, maps are missing, or route points coincide.

Explicit-origin preview example:

```text
om://v2/dir?origin=52.5200,13.4050&origin_name=Warehouse%20Berlin&destination=52.5163,13.3777&destination_name=Warehouse%20Berlin%20(Return)&waypoints=52.5304,13.3850%7C52.5450,13.3920%7C52.5612,13.4150&waypoint_names=Anna%20Schmidt%7CBauer%20GmbH%7CM%C3%BCller%20Family&mode=drive&avoid=tolls&ref_name=DeliveryCo%20Driver&callback=delivery%3A%2F%2Fjob%2F4521%2Freturn
```

Current-position navigation example:

```text
om://v2/nav?destination=52.5163,13.3777&destination_name=Warehouse%20Berlin%20(Return)&waypoints=52.5304,13.3850%7C52.5450,13.3920%7C52.5612,13.4150&waypoint_names=Anna%20Schmidt%7CBauer%20GmbH%7CM%C3%BCller%20Family&mode=drive&avoid=tolls&ref_name=DeliveryCo%20Driver&callback=delivery%3A%2F%2Fjob%2F4521%2Freturn
```

Equivalent HTTPS form:

```text
https://omaps.app/v2/dir?destination=47.38568,8.566878&waypoints=47.395084,8.552692%7C47.3890,8.5580&mode=bike
```

## Legacy two-point route links

The legacy two-point route format is also supported:

```text
om://route?v=1&sll=50.183933,8.942871&saddr=Start%20Point&dll=49.998912,8.278198&daddr=End%20Point&type=vehicle
```

Legacy route links also accept the common `appname=...` and `cll=lat,lon` parameters. Use v2 links for caller callbacks: legacy route links ignore `callback=...` and `backurl=...`.

Legacy **map** links (`om://map?...&backurl=...`) retain Android's automatic return when the map activity stops
while navigation is inactive, including activity recreation or opening another Organic Maps activity.
Launching a stop callback suppresses the next automatic return. This compatibility behavior does not apply
to v2 route links.

Do not mix the legacy `sll`/`saddr`/`dll`/`daddr` parameters with the v2 `/v2/dir` or `/v2/nav` syntax.
