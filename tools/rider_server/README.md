# Rider Tracking Server

Lightweight cloud-side location relay for the Organic Maps Android "Rider Live
Tracking" feature. Riders POST their coordinates; friends poll for them.

## Deploy

```bash
# On any Linux VSI with Python 3.10+
RIDER_USER=alice RIDER_PASS=hunter2 python3 rider_server.py
```

Env vars (all optional, defaults shown):

| Var          | Default   | Purpose                                      |
| ------------ | --------- | -------------------------------------------- |
| `RIDER_HOST` | `0.0.0.0` | Bind address                                 |
| `RIDER_PORT` | `8080`    | Bind port                                    |
| `RIDER_TTL`  | `600`     | Location expiry in seconds                   |
| `RIDER_USER` | `rider`   | Basic Auth username (both required to gate)  |
| `RIDER_PASS` | `changeme`| Basic Auth password                          |

If either `RIDER_USER` or `RIDER_PASS` is empty, auth is disabled (open server).
Otherwise every endpoint except `/health` requires HTTP Basic Auth.

The same user/pass must be entered in the Android app under
**Settings → Rider Live Tracking → Cloud VSI Server Settings**.

## API

| Method | Path                    | Body                                                    | Purpose                          |
| ------ | ----------------------- | ------------------------------------------------------- | -------------------------------- |
| GET    | `/health`               | -                                                       | Liveness probe                   |
| POST   | `/api/location`         | `{code, lat, lon, speed?, bearing?}`                    | Upsert own position              |
| GET    | `/api/location?code=X`  | -                                                       | Fetch single rider               |
| POST   | `/api/locations/batch`  | `{codes: [...]}`                                        | Fetch many riders (used by app)  |

Locations older than `LOCATION_TTL_SECONDS` (default 10 min) are treated as
missing.

## Storage

In-memory SQLite. State resets on restart. For persistence, swap
`":memory:"` for a file path.

## Security

HTTP Basic Auth gates the server (username + password) but is not per-rider.
Rider codes are still shareable secrets — anyone with a friend's code can
see their live location. For real deployments front with TLS (nginx / Caddy)
so credentials aren't sent in cleartext.
