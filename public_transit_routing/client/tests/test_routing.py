python
"""
client/tests/test_routing.py

Unit and integration tests for the public‑transport routing algorithm.

The tests cover:
* Basic static routing using a deterministic graph.
* Real‑time schedule updates (delta handling).
* Error handling for malformed inputs.
* Integration with the HTTP schedule endpoint (mocked).

Dependencies:
    pytest
    networkx
    requests
    responses
"""

from __future__ import annotations

import logging
from datetime import datetime, timedelta
from typing import Dict, List, Tuple

import networkx as nx
import pytest
import responses

# The routing implementation is expected to live in ``client.routing``.
# It provides a ``RoutePlanner`` class with a ``find_route`` method.
# The method signature is:
#   find_route(
#       source: str,
#       target: str,
#       departure: datetime,
#       realtime: bool = False,
#   ) -> List[Tuple[str, datetime]]
# where the returned list contains (stop_id, arrival_time) tuples.
from client.routing import RoutePlanner, RoutingError

# --------------------------------------------------------------------------- #
# Logging configuration (tests should not interfere with library logging)
# --------------------------------------------------------------------------- #
logger = logging.getLogger(__name__)
if not logger.handlers:
    handler = logging.StreamHandler()
    formatter = logging.Formatter(
        fmt="%(asctime)s %(levelname)s %(name)s – %(message)s",
        datefmt="%Y-%m-%d %H:%M:%S",
    )
    handler.setFormatter(formatter)
    logger.addHandler(handler)
    logger.setLevel(logging.DEBUG)


# --------------------------------------------------------------------------- #
# Fixtures
# --------------------------------------------------------------------------- #
@pytest.fixture(scope="module")
def static_graph() -> nx.DiGraph:
    """
    Build a tiny multimodal graph used for deterministic unit tests.

    Nodes represent stops or intersections.
    Edge weight = travel time in minutes.
    """
    g = nx.DiGraph()
    # Simple line: A -> B -> C
    g.add_edge("A", "B", weight=5)   # 5 min
    g.add_edge("B", "C", weight=7)   # 7 min
    # Direct shortcut with longer travel time (to test optimality)
    g.add_edge("A", "C", weight=15)  # 15 min
    logger.debug("Static graph created with %d nodes and %d edges", g.number_of_nodes(), g.number_of_edges())
    return g


@pytest.fixture(scope="module")
def static_schedule() -> Dict[Tuple[str, str], List[datetime]]:
    """
    Minimal timetable for the static graph.
    Keys are (origin, destination) pairs, values are sorted departure times.
    """
    now = datetime.utcnow().replace(second=0, microsecond=0)
    schedule = {
        ("A", "B"): [now + timedelta(minutes=0), now + timedelta(minutes=10), now + timedelta(minutes=20)],
        ("B", "C"): [now + timedelta(minutes=6), now + timedelta(minutes=16), now + timedelta(minutes=26)],
        ("A", "C"): [now + timedelta(minutes=2), now + timedelta(minutes=12), now + timedelta(minutes=22)],
    }
    logger.debug("Static schedule generated for %d routes", len(schedule))
    return schedule


@pytest.fixture(scope="module")
def realtime_delta() -> Dict[Tuple[str, str], List[datetime]]:
    """
    Simulated real‑time delta that delays the B→C connection by 3 minutes.
    """
    now = datetime.utcnow().replace(second=0, microsecond=0)
    delta = {
        ("B", "C"): [now + timedelta(minutes=9), now + timedelta(minutes=19), now + timedelta(minutes=29)],
    }
    logger.debug("Realtime delta prepared for %d routes", len(delta))
    return delta


# --------------------------------------------------------------------------- #
# Helper – a tiny HTTP mock for the schedule endpoint
# --------------------------------------------------------------------------- #
SCHEDULE_ENDPOINT = "http://testserver/api/v1/schedule"


def _mock_schedule_response(schedule: Dict[Tuple[str, str], List[datetime]]) -> None:
    """
    Register a ``responses`` mock that returns the provided schedule as JSON.
    The JSON format mirrors the expected server contract:
        {
            "routes": [
                {"origin": "A", "dest": "B", "departures": ["2023-01-01T12:00:00Z", ...]},
                ...
            ]
        }
    """
    payload = {
        "routes": [
            {
                "origin": o,
                "dest": d,
                "departures": [dt.isoformat() + "Z" for dt in times],
            }
            for (o, d), times in schedule.items()
        ]
    }
    responses.add(
        responses.GET,
        SCHEDULE_ENDPOINT,
        json=payload,
        status=200,
    )
    logger.debug("Mocked schedule endpoint with %d routes", len(payload["routes"])))


# --------------------------------------------------------------------------- #
# Unit tests – static routing
# --------------------------------------------------------------------------- #
def test_find_route_static(
    static_graph: nx.DiGraph,
    static_schedule: Dict[Tuple[str, str], List[datetime]],
) -> None:
    """
    Verify that the planner chooses the fastest path using only static data.
    """
    planner = RoutePlanner(graph=static_graph, schedule=static_schedule)
    departure = datetime.utcnow().replace(second=0, microsecond=0)

    route = planner.find_route(source="A", target="C", departure=departure, realtime=False)

    # Expected optimal path: A → B → C (5 + 7 = 12 min) vs direct A → C (15 min)
    expected_stops = ["A", "B", "C"]
    assert [stop for stop, _ in route] == expected_stops, "Planner did not select optimal path"
    # Verify monotonic increase of arrival times
    arrival_times = [t for _, t in route]
    assert arrival_times == sorted(arrival_times), "Arrival times are not chronological"


# --------------------------------------------------------------------------- #
# Unit tests – real‑time delta handling
# --------------------------------------------------------------------------- #
def test_find_route_realtime(
    static_graph: nx.DiGraph,
    static_schedule: Dict[Tuple[str, str], List[datetime]],
    realtime_delta: Dict[Tuple[str, str], List[datetime]],
) -> None:
    """
    The real‑time delta delays B→C by 3 minutes.
    The planner should still pick A→B→C because the alternative A→C remains unchanged.
    """
    planner = RoutePlanner(
        graph=static_graph,
        schedule=static_schedule,
        realtime_updates=realtime_delta,
    )
    departure = datetime.utcnow().replace(second=0, microsecond=0)

    route = planner.find_route(source="A", target="C", departure=departure, realtime=True)

    expected_stops = ["A", "B", "C"]
    assert [stop for stop, _ in route] == expected_stops, "Realtime handling broke optimal path selection"


# --------------------------------------------------------------------------- #
# Error handling tests
# --------------------------------------------------------------------------- #
def test_invalid_source_target(
    static_graph: nx.DiGraph,
    static_schedule: Dict[Tuple[str, str], List[datetime]],
) -> None:
    """
    Passing an unknown source or target must raise ``RoutingError``.
    """
    planner = RoutePlanner(graph=static_graph, schedule=static_schedule)
    departure = datetime.utcnow()

    with pytest.raises(RoutingError) as excinfo:
        planner.find_route(source="X", target="C", departure=departure, realtime=False)
    assert "source" in str(excinfo.value).lower()

    with pytest.raises(RoutingError) as excinfo:
        planner.find_route(source="A", target="Y", departure=departure, realtime=False)
    assert "target" in str(excinfo.value).lower()


def test_missing_schedule(
    static_graph: nx.DiGraph,
) -> None:
    """
    If the schedule does not contain a required route, ``RoutingError`` is raised.
    """
    planner = RoutePlanner(graph=static_graph, schedule={})
    departure = datetime.utcnow()

    with pytest.raises(RoutingError) as excinfo:
        planner.find_route(source="A", target="B", departure=departure, realtime=False)
    assert "schedule" in str(excinfo.value).lower()


def test_malformed_departure(
    static_graph: nx.DiGraph,
    static_schedule: Dict[Tuple[str, str], List[datetime]],
) -> None:
    """
    Passing a non‑datetime ``departure`` argument must raise ``RoutingError``.
    """
    planner = RoutePlanner(graph=static_graph, schedule=static_schedule)

    with pytest.raises(RoutingError) as excinfo:
        planner.find_route(source="A", target="B", departure="not-a-datetime", realtime=False)  # type: ignore[arg-type]
    assert "departure" in str(excinfo.value).lower()


# --------------------------------------------------------------------------- #
# Integration test – HTTP schedule endpoint (mocked)
# --------------------------------------------------------------------------- #
@responses.activate
def test_http_schedule_integration(
    static_graph: nx.DiGraph,
    static_schedule: Dict[Tuple[str, str], List[datetime]],
) -> None:
    """
    Ensure the planner can fetch a schedule from the HTTP endpoint and use it.
    """
    _mock_schedule_response(static_schedule)

    # The planner is expected to accept a ``schedule_url`` parameter.
    planner = RoutePlanner(graph=static_graph, schedule_url=SCHEDULE_ENDPOINT)

    departure = datetime.utcnow().replace(second=0, microsecond=0)
    route = planner.find_route(source="A", target="C", departure=departure, realtime=False)

    expected_stops = ["A", "B", "C"]
    assert [stop for stop, _ in route] == expected_stops, "HTTP schedule integration failed"