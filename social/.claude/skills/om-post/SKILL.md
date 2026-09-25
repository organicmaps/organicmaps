---
name: om-post
description: "Turn an Organic Maps release changelog plus raw screenshots into ready-to-publish Instagram images (4:5, 1:1, 9:16) and a caption. Use when asked to make a social post, Instagram carousel, or release announcement graphics for Organic Maps."
---

# Organic Maps → Instagram post

Input: a release changelog (plain text) and a handful of screenshots.
Output: `releases/<YYYY-MM>/export/{4x5,1x1,9x16}/*.png` plus `caption.txt`.

Everything lives in `social/` inside the organicmaps repo. Brand assets are
already copied into `social/brand/` — read from the rest of the repo if you
need something new, but do not modify it.

## 1. Ingest

```
releases/<YYYY-MM>/
  raw/         # screenshots, named for what they show: color-palette-ios.jpg
  post.toml    # the slide script
  html/        # generated HTML (disposable, gitignored)
  export/      # final PNGs — the deliverable
  caption.txt
```

Copy screenshots into `raw/` with descriptive names. Note each one's pixel size —
`device = "phone"` assumes a portrait phone shot, `desktop` a landscape window
shot that already contains its own window chrome.

## 2. Write `post.toml`

```toml
release = "2026-07"
lang = "en"
formats = ["4x5", "1x1", "9x16"]

[[slides]]
type = "cover"           # cover | feature | list | cta
kicker = "Organic Maps"
title = "July Update"
subtitle = "One or two sentences that set up the carousel."

[[slides]]
type = "feature"
eyebrow = "iOS · Bookmarks"
title = "Pick any color"
body = "One sentence. What changed and why the reader cares."
media = "raw/color-palette-ios.jpg"
device = "phone"         # phone | desktop | plain
theme = "blue"           # green | blue | light | dark
bleed = 120              # optional, see below

[[slides]]
type = "list"
eyebrow = "Android"
title = "Smoother day to day"
theme = "light"
items = ["…", "…"]       # 4-5 max, one line each ideally

[[slides]]
type = "cta"
title = "Get the July update"
body = "Free, offline, ad-free and open source."
url = "get.omaps.org"
badges = ["apple-appstore", "google-play", "fdroid"]
```

Keys are all optional except `type` and `title`. Available badge names are the
filenames in `brand/badges/`.

**`bleed`** controls portrait mockups. By default a phone runs off the bottom
edge so it reads large instead of shrinking to a sliver. If the feature being
demonstrated sits low in the screenshot, set `bleed` to a smaller pixel number
(e.g. `120`) so it stays on canvas, or `false` to fit the whole phone.

See [references/layouts.md](references/layouts.md) for what each slide type
looks like and [references/copywriting.md](references/copywriting.md) for
turning changelog bullets into slide copy and a caption.

## 3. Build

```
cd <repo>/social
python3 .claude/skills/om-post/scripts/build.py releases/2026-07
```

Flags: `--only 4x5` and `--slide 3` to iterate fast on one image.

The script renders each slide through headless Chrome at 2x, downsamples with
Pillow to the exact Instagram size, and fails loudly if a PNG is missing or came
out as a flat frame. Two Chrome quirks are already handled inside it and must
not be "simplified" away:

- top-level `data:` URLs hang Chrome forever, so HTML is always written to disk
  and loaded over `file://`;
- `--screenshot` writes a valid PNG but never exits, so the process is polled
  and killed. `timeout(1)` is not installed on this machine.

A throwaway `--user-data-dir` under `.cache/` keeps the user's own Chrome
profile untouched; it is deleted afterwards.

## 4. Review before delivering

Always look at the rendered PNGs — build success does not mean it looks right.
Assemble a contact sheet and read it:

```python
from PIL import Image; import glob
ps = sorted(glob.glob('releases/2026-07/export/4x5/*.png'))
th = [Image.open(p).convert('RGB').resize((360,450), Image.LANCZOS) for p in ps]
sheet = Image.new('RGB', (4*370+10, 2*460+10), (230,230,230))
for i,t in enumerate(th): sheet.paste(t, (10+(i%4)*370, 10+(i//4)*460))
sheet.save('/tmp/sheet.png')
```

Check specifically:

- the feature being announced is actually visible in the screenshot, not cropped
  off by the bleed;
- no title wraps to an ugly orphan word — shorten the copy rather than the type;
- 9:16 content clears the Stories UI (nothing in the top 250px / bottom 300px);
- long list slides did not overflow their canvas.

## 5. Caption

Write `caption.txt`: a one-line hook, the highlights as short bullets, the
download link, then a hashtag block. Keep it under ~2 200 characters.
Details in [references/copywriting.md](references/copywriting.md).

## 6. Deliver

Send the 4:5 set with `SendUserFile` in slide order, plus the caption.

## Design changes

All styling is in `assets/brand.css` — tokens, canvas formats, slide types and
device frames. `build.py` only emits structure and never inlines styles beyond
the per-slide `--bleed` override. Brand green is `#006C35`; type is the system
SF stack, since no webfont can be fetched offline.
