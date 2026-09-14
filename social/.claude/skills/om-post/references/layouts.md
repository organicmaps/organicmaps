# Slide types

Four types cover every release post. Resist inventing new ones — vary the
`theme` instead.

## `cover`

App icon, kicker, oversized title, one-sentence subtitle, a short rule.
Always green. This is the thumbnail in the feed, so the title must be readable
at 150px wide: three words at most, no punctuation.

Keys: `kicker`, `title`, `subtitle`.

## `feature`

One change, one screenshot. Eyebrow chip names the platform and area
(`iOS · Bookmarks`, `Routing`), title states the benefit, body gives one
sentence of detail.

Keys: `eyebrow`, `title`, `body`, `media`, `device`, `theme`, `bleed`.

`device`:

- `phone` — portrait shot. Thin dark bezel via box-shadow, bleeds off the
  bottom edge by default so it reads large.
- `desktop` — landscape shot. Rounded corners and a drop shadow only; the
  macOS traffic lights are already inside the screenshot. Runs nearly full
  canvas width.
- `plain` — rounded card, no device pretence. For diagrams or crops.

## `list`

Changes with no screenshot worth showing. Light theme by default so the
carousel breathes between image slides. Keep to 4–5 items; each item should
fit two lines at most.

Keys: `eyebrow`, `title`, `items`, `theme`.

## `cta`

Centred. Title, optional body, a pill with the short URL, then a row of store
badges. Always the last slide.

Keys: `title`, `body`, `url`, `badges`.

# Themes

| theme   | use                                            |
|---------|------------------------------------------------|
| `green` | default; brand gradient, white text            |
| `blue`  | one or two slides for rhythm — do not overuse  |
| `light` | list slides, and any screenshot with a dark UI |
| `dark`  | rare; a night-mode screenshot                  |

# Rhythm

A seven-slide carousel that works: cover → feature → feature (different theme)
→ feature → list → list → cta. Never put two `light` slides adjacent to two
image slides of the same theme; alternate so the swipe has contrast.

# Formats

All three render from the same `post.toml`. The canvas differs only in size,
padding and type scale, so a slide that reads well at 4:5 generally survives
the other two. The exception is 9:16, which reserves 250px at the top and 300px
at the bottom for the Stories interface — check long list slides there.
