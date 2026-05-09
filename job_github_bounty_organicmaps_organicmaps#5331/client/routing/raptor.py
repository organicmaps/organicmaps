# client/routing/raptor.py
"""Time‑dependent RAPTOR routing implementation.

The module provides a production‑grade, pure‑Python implementation of the
RAPTOR (Round‑Based Public Transport Optimized Router) algorithm.  It works
with static schedule data and optional real‑time updates (delays or
cancellations).  The implementation is deliberately self‑contained – only
standard library modules and ``networkx`` (used for optional graph utilities)
are required.

Typical usage
-------------
>>> from client.routing.raptor import RAPTORRouter, Stop, Route, Trip, Transfer
>>> stops = {1: Stop(1, "A"), 2: Stop(2, "B"), 3: Stop(3, "C")}
>>> routes = {1: Route(1, [1, 2, 3])}
>>> trips = [
...     Trip(
...         101,
...         1,
...         {1: 3600, 2: 4200, 3: 4800},
...         {1: 0, 2: 3600, 3: 4200},
...     )
... ]
>>> transfers = [Transfer(1, 2, 300), Transfer(2, 3, 300)]
>>> router = RAPTORRouter(stops, routes, trips, transfers)
>>> path = router.find_route(1, 3, 3500)
>>> path
[(1, 3500), (2, 4200), (3, 4800)]
"""

from __future__ import annotations

import logging
import bisect
from dataclasses import dataclass, field
from typing import (
    Dict,
    List,
    Tuple,
    Optional,
    Iterable,
    Set,
    Generator,
    Any,
)

# Optional import – only needed for external utilities, not for core routing.
try:
    import networkx as nx  # pragma: no cover
except Exception:  # pragma: no cover
    nx = None  # type: ignore

log = logging.getLogger(__name__)
log.setLevel(logging.INFO)


# --------------------------------------------------------------------------- #
# Data model
# --------------------------------------------------------------------------- #
@dataclass(frozen=True, slots=True)
class Stop:
    """Transit stop."""
    stop_id: int
    name: str

    def __repr__(self) -> str:
        return f"Stop({self.stop_id}, {self.name!r})"


@dataclass(frozen=True, slots=True)
class Route:
    """Ordered list of stop identifiers that belong to a route."""
    route_id: int
    stop_sequence: Tuple[int, ...]  # immutable for hashability

    def __post_init__(self) -> None:
        if not self.stop_sequence:
            raise ValueError("Route must contain at least one stop")

    def __repr__(self) -> str:
        return f"Route({self.route_id}, {self.stop_sequence})"


@dataclass(slots=True)
class Trip:
    """Single vehicle trip on a route.

    ``departure_times`` and ``arrival_times`` map stop identifiers to seconds
    since midnight (or any epoch).  The dictionaries must contain the same
    set of stops as the associated ``Route``.
    """
    trip_id: int
    route_id: int
    departure_times: Dict[int, int] = field(default_factory=dict)
    arrival_times: Dict[int, int] = field(default_factory=dict)

    def __post_init__(self) -> None:
        if set(self.departure_times) != set(self.arrival_times):
            raise ValueError("Departure and arrival stop sets must match")
        if not self.departure_times:
            raise ValueError("Trip must contain at least one stop")

    def __repr__(self) -> str:
        return f"Trip({self.trip_id}, route={self.route_id})"


@dataclass(frozen=True, slots=True)
class Transfer:
    """Walking or other transfer between two stops."""
    from_stop: int
    to_stop: int
    duration: int  # seconds

    def __repr__(self) -> str:
        return (
            f"Transfer({self.from_stop}→{self.to_stop}, "
            f"{self.duration}s)"
        )


@dataclass(frozen=True, slots=True)
class RealTimeUpdate:
    """Real‑time modification for a trip.

    ``delay`` is added to all departure/arrival times.
    ``cancelled`` indicates the trip must be ignored.
    """
    delay: int = 0
    cancelled: bool = False

    def __repr__(self) -> str:
        return f"RealTimeUpdate(delay={self.delay}, cancelled={self.cancelled})"


# --------------------------------------------------------------------------- #
# Helper utilities
# --------------------------------------------------------------------------- #
def _apply_realtime(
    trip: Trip,
    rt_update: Optional[RealTimeUpdate],
) -> Tuple[Dict[int, int], Dict[int, int]]:
    """Return (departure, arrival) dictionaries after applying a real‑time update.

    If the trip is cancelled, empty dictionaries are returned.
    """
    if rt_update is None:
        return trip.departure_times, trip.arrival_times
    if rt_update.cancelled:
        log.debug("Trip %s cancelled by real‑time update", trip.trip_id)
        return {}, {}
    if rt_update.delay == 0:
        return trip.departure_times, trip.arrival_times

    delay = rt_update.delay
    dep = {s: t + delay for s, t in trip.departure_times.items()}
    arr = {s: t + delay for s, t in trip.arrival_times.items()}
    log.debug(
        "Applied %d s delay to trip %s (now %s)",
        delay,
        trip.trip_id,
        dep,
    )
    return dep, arr


def _next_departure(
    departure_times: Dict[int, int],
    stop_id: int,
    earliest: int,
) -> Optional[int]:
    """Return the earliest departure time from ``stop_id`` not earlier than ``earliest``."""
    if stop_id not in departure_times:
        return None
    dep = departure_times[stop_id]
    return dep if dep >= earliest else None


# --------------------------------------------------------------------------- #
# Core RAPTOR implementation
# --------------------------------------------------------------------------- #
class RAPTORRouter:
    """Time‑dependent RAPTOR router.

    The router works with a static schedule (stops, routes, trips, transfers) and
    optional real‑time updates.  It returns the earliest‑arrival path from an
    origin stop to a destination stop for a given departure time.
    """

    def __init__(
        self,
        stops: Dict[int, Stop],
        routes: Dict[int, Route],
        trips: Iterable[Trip],
        transfers: Iterable[Transfer],
        realtime_updates: Optional[Dict[int, RealTimeUpdate]] = None,
    ) -> None:
        """
        Parameters
        ----------
        stops
            Mapping ``stop_id → Stop``.
        routes
            Mapping ``route_id → Route``.
        trips
            Collection of :class:`Trip` objects.
        transfers
            Collection of :class:`Transfer` objects.
        realtime_updates
            Optional mapping ``trip_id → RealTimeUpdate``.
        """
        self.stops = stops
        self.routes = routes
        self.trips_by_route: Dict[int, List[Trip]] = {}
        for trip in trips:
            self.trips_by_route.setdefault(trip.route_id, []).append(trip)

        self.transfers_by_stop: Dict[int, List[Transfer]] = {}
        for tr in transfers:
            self.transfers_by_stop.setdefault(tr.from_stop, []).append(tr)

        self.realtime_updates = realtime_updates or {}
        log.info(
            "RAPTORRouter initialised with %d stops, %d routes, %d trips, %d transfers",
            len(stops),
            len(routes),
            len(self.trips_by_route),
            len(transfers),
        )

    # --------------------------------------------------------------------- #
    # Public API
    # --------------------------------------------------------------------- #
    def set_realtime_updates(
        self,
        updates: Dict[int, RealTimeUpdate],
    ) -> None:
        """Replace the current real‑time update set."""
        self.realtime_updates = updates
        log.info("Real‑time updates set for %d trips", len(updates))

    def find_route(
        self,
        origin: int,
        target: int,
        departure_time: int,
        max_rounds: int = 10,
    ) -> List[Tuple[int, int]]:
        """Return the earliest‑arrival path from ``origin`` to ``target``.

        The result is a list of ``(stop_id, arrival_time)`` tuples, starting
        with the origin stop at ``departure_time`` and ending with the target
        stop at the earliest possible arrival time.  If no route exists,
        an empty list is returned.

        Parameters
        ----------
        origin
            Identifier of the start stop.
        target
            Identifier of the destination stop.
        departure_time
            Desired departure time expressed in seconds since an epoch.
        max_rounds
            Upper bound on the number of RAPTOR rounds (default 10).

        Raises
        ------
        ValueError
            If ``origin`` or ``target`` is unknown.
        """
        if origin not in self.stops:
            raise ValueError(f"Origin stop {origin!r} not found")
        if target not in self.stops:
            raise ValueError(f"Target stop {target!r} not found")

        # -----------------------------------------------------------------
        # RAPTOR state
        # -----------------------------------------------------------------
        # best_arrival[stop] = earliest known arrival time (across all rounds)
        best_arrival: Dict[int, int] = {s: float("inf") for s in self.stops}
        best_arrival[origin] = departure_time

        # predecessor[stop] = (prev_stop, arrival_time) for path reconstruction
        predecessor: Dict[int, Tuple[int, int]] = {}

        # stops_marked[round] = set of stops that changed in the previous round
        marked: Set[int] = {origin}
        round_counter = 0

        while marked and round_counter < max_rounds:
            round_counter += 1
            log.debug("Round %d – marked stops: %s", round_counter, marked)

            # -----------------------------------------------------------------
            # 1️⃣  Scan routes that contain any marked stop
            # -----------------------------------------------------------------
            for route_id, route in self.routes.items():
                # Fast check: does the route intersect the marked set?
                if not any(stop in marked for stop in route.stop_sequence):
                    continue

                # Process each trip on the route
                for trip in self.trips_by_route.get(route_id, []):
                    dep_times, arr_times = _apply_realtime(
                        trip, self.realtime_updates.get(trip.trip_id)
                    )
                    if not dep_times:  # cancelled
                        continue

                    # Find the earliest stop on the trip that we can board
                    board_stop = None
                    board_time = None
                    for stop in route.stop_sequence:
                        dep = _next_departure(dep_times, stop, best_arrival[stop])
                        if dep is not None:
                            board_stop = stop
                            board_time = dep
                            break
                    if board_stop is None:
                        continue  # cannot board this trip

                    # Propagate forward along the trip
                    for i in range(route.stop_sequence.index(board_stop) + 1, len(route.stop_sequence)):
                        stop = route.stop_sequence[i]
                        arrival = arr_times[stop]
                        if arrival < best_arrival[stop]:
                            log.debug(
                                "Trip %s improves arrival at stop %s: %s → %s",
                                trip.trip_id,
                                stop,
                                best_arrival[stop],
                                arrival,
                            )
                            best_arrival[stop] = arrival
                            predecessor[stop] = (board_stop, arrival)
                            marked.add(stop)

            # -----------------------------------------------------------------
            # 2️⃣  Process walking transfers from newly improved stops
            # -----------------------------------------------------------------
            new_marked: Set[int] = set()
            for stop in marked:
                for tr in self.transfers_by_stop.get(stop, []):
                    arrival_via_transfer = best_arrival[stop] + tr.duration
                    if arrival_via_transfer < best_arrival[tr.to_stop]:
                        log.debug(
                            "Transfer %s→%s improves arrival: %s → %s",
                            tr.from_stop,
                            tr.to_stop,
                            best_arrival[tr.to_stop],
                            arrival_via_transfer,
                        )
                        best_arrival[tr.to_stop] = arrival_via_transfer
                        predecessor[tr.to_stop] = (tr.from_stop, arrival_via_transfer)
                        new_marked.add(tr.to_stop)
            marked = new_marked

        # -----------------------------------------------------------------
        # Path reconstruction
        # -----------------------------------------------------------------
        if best_arrival[target] == float("inf"):
            log.info("No route found from %s to %s", origin, target)
            return []

        path: List[Tuple[int, int]] = []
        cur_stop = target
        cur_time = best_arrival[target]
        while cur_stop != origin:
            path.append((cur_stop, cur_time))
            prev_stop, prev_time = predecessor[cur_stop]
            cur_stop, cur_time = prev_stop, prev_time
        path.append((origin, departure_time))
        path.reverse()
        log.info(
            "Route found (duration %d s) with %d stops",
            best_arrival[target] - departure_time,
            len(path),
        )
        return path

    # --------------------------------------------------------------------- #
    # Optional utilities (graph export for debugging / visualisation)
    # --------------------------------------------------------------------- #
    def export_graph(self) -> Any:
        """Export the public‑transport network as a NetworkX DiGraph.

        The graph contains two types of edges:

        * **Trip edges** – from a stop to the next stop on a trip, weighted by
          travel time (arrival‑departure).
        * **Transfer edges** – walking connections, weighted by the transfer duration.

        Returns
        -------
        networkx.DiGraph
            The constructed graph, or ``None`` if NetworkX is not installed.
        """
        if nx is None:  # pragma: no cover
            log.warning("NetworkX not available – cannot export graph")
            return None

        g = nx.DiGraph()
        for stop_id, stop in self.stops.items():
            g.add_node(stop_id, name=stop.name)

        # Trip edges
        for route in self.routes.values():
            for trip in self.trips_by_route.get(route.route_id, []):
                dep, arr = _apply_realtime(
                    trip, self.realtime_updates.get(trip.trip_id)
                )
                if not dep:
                    continue
                seq = route.stop_sequence
                for i in range(len(seq) - 1):
                    u, v = seq[i], seq[i + 1]
                    travel = arr[v] - dep[u]
                    if travel < 0:
                        continue  # malformed data, ignore
                    g.add_edge(
                        u,
                        v,
                        trip_id=trip.trip_id,
                        travel_time=travel,
                        weight=travel,
                    )

        # Transfer edges
        for tr in self.transfers_by_stop.values():
            for t in tr:
                g.add_edge(
                    t.from_stop,
                    t.to_stop,
                    travel_time=t.duration,
                    weight=t.duration,
                    transfer=True,
                )
        return g