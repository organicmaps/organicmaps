import datetime
import logging
import os
import re
import tempfile
import unittest
from collections import Counter

from maps_generator.checks.logs import logs_reader


class TestLogsReader(unittest.TestCase):
    def setUp(self):
        self.dir = tempfile.TemporaryDirectory()
        with open(
            os.path.join(self.dir.name, "Czech_Jihovychod_Jihomoravsky kraj.log"), "w"
        ) as file:
            file.write(LOG_STRING)

        logs = list(logs_reader.LogsReader(self.dir.name))
        self.assertEqual(len(logs), 1)
        self.log = logs[0]

    def tearDown(self):
        self.dir.cleanup()

    def test_read_logs(self):
        self.assertTrue(self.log.name.startswith("Czech_Jihovychod_Jihomoravsky kraj"))
        self.assertTrue(self.log.is_mwm_log)
        self.assertFalse(self.log.is_stage_log)
        self.assertEqual(len(self.log.lines), 46)

    def test_split_into_stages(self):
        st = logs_reader.split_into_stages(self.log)
        self.assertEqual(len(st), 4)
        names_counter = Counter(s.name for s in st)
        self.assertEqual(
            names_counter,
            Counter({"Routing": 1, "RoutingTransit": 1, "MwmStatistics": 2}),
        )

    def test_split_and_normalize_logs(self):
        st = logs_reader.normalize_logs(logs_reader.split_into_stages(self.log))
        self.assertEqual(len(st), 3)
        m = {s.name: s for s in st}
        self.assertEqual(
            m["MwmStatistics"].duration, datetime.timedelta(seconds=3.628742)
        )

    def test_count_levels(self):
        st = logs_reader.normalize_logs(logs_reader.split_into_stages(self.log))
        self.assertEqual(len(st), 3)
        m = {s.name: s for s in st}
        c = logs_reader.count_levels(m["Routing"])
        self.assertEqual(c, Counter({logging.INFO: 22, logging.ERROR: 1}))

        c = logs_reader.count_levels(self.log.lines)
        self.assertEqual(c, Counter({logging.INFO: 45, logging.ERROR: 1}))

    def test_find_and_parse(self):
        st = logs_reader.normalize_logs(logs_reader.split_into_stages(self.log))
        self.assertEqual(len(st), 3)
        m = {s.name: s for s in st}
        pattern_str = (
            r".*Leaps finished, elapsed: [0-9.]+ seconds, routes found: "
            r"(?P<routes_found>\d+) , not found: (?P<routes_not_found>\d+)$"
        )
        for found in (
            logs_reader.find_and_parse(m["Routing"], pattern_str),
            logs_reader.find_and_parse(self.log.lines, re.compile(pattern_str)),
        ):

            self.assertEqual(len(found), 1)
            line = found[0]
            self.assertEqual(
                line[0], {"routes_found": "996363", "routes_not_found": "126519"}
            )


if __name__ == "main":
    unittest.main()


LOG_STRING = """
[2020-05-24 04:19:37,032] INFO stages Stage Routing: start ...
[2020-05-24 04:19:37,137] INFO gen_tool Run generator tool [generator_tool version 1590177464 f52c6496c4d90440f2e0d8088acdb3350dcf7c69]: /home/Projects/build-omim-Desktop_Qt_5_10_1_GCC_64bit-Release/generator_tool --threads_count=1 --data_path=/home/maps_build/2020_05_23__16_58_17/draft --intermediate_data_path=/home/maps_build/2020_05_23__16_58_17/intermediate_data --user_resource_path=/home/Projects/omim/data --cities_boundaries_data=/home/maps_build/2020_05_23__16_58_17/intermediate_data/cities_boundaries.bin --generate_maxspeed=true --make_city_roads=true --make_cross_mwm=true --disable_cross_mwm_progress=true --generate_cameras=true --make_routing_index=true --generate_traffic_keys=true --output=Czech_Jihovychod_Jihomoravsky kraj 
LOG TID(1) INFO     3.29e-06 Loaded countries list for version: 200402
LOG TID(1) INFO    7.945e-05 generator/camera_info_collector.cpp:339 BuildCamerasInfo() Generating cameras info for /home/maps_build/2020_05_23__16_58_17/draft/Czech_Jihovychod_Jihomoravsky kraj.mwm
LOG TID(1) INFO     0.529856 generator/routing_index_generator.cpp:546 BuildRoutingIndex() Building routing index for /home/maps_build/2020_05_23__16_58_17/draft/Czech_Jihovychod_Jihomoravsky kraj.mwm
LOG TID(1) INFO      2.11074 generator/routing_index_generator.cpp:563 BuildRoutingIndex() Routing section created: 639872 bytes, 163251 roads, 193213 joints, 429334 points
LOG TID(1) INFO      2.90872 generator/restriction_generator.cpp:117 SerializeRestrictions() Routing restriction info: RestrictionHeader: { No => 430, Only => 284, NoUTurn => 123, OnlyUTurn => 0 }
LOG TID(1) INFO      3.00342 generator/road_access_generator.cpp:799 BuildRoadAccessInfo() Generating road access info for /home/maps_build/2020_05_23__16_58_17/draft/Czech_Jihovychod_Jihomoravsky kraj.mwm
LOG TID(1) INFO      3.77435 generator_tool/generator_tool.cpp:621 operator()() Generating cities boundaries roads for /home/maps_build/2020_05_23__16_58_17/draft/Czech_Jihovychod_Jihomoravsky kraj.mwm
LOG TID(1) INFO      3.85993 generator/city_roads_generator.cpp:51 LoadCitiesBoundariesGeometry() Read: 14225 boundaries from: /home/maps_build/2020_05_23__16_58_17/intermediate_data/routing_city_boundaries.bin
LOG TID(1) INFO      6.82577 routing/city_roads_serialization.hpp:78 Serialize() Serialized 81697 road feature ids in cities. Size: 77872 bytes.
LOG TID(1) INFO      6.82611 generator_tool/generator_tool.cpp:621 operator()() Generating maxspeeds section for /home/maps_build/2020_05_23__16_58_17/draft/Czech_Jihovychod_Jihomoravsky kraj.mwm
LOG TID(1) INFO      6.82616 generator/maxspeeds_builder.cpp:186 BuildMaxspeedsSection() BuildMaxspeedsSection( /home/maps_build/2020_05_23__16_58_17/draft/Czech_Jihovychod_Jihomoravsky kraj.mwm , /home/maps_build/2020_05_23__16_58_17/draft/Czech_Jihovychod_Jihomoravsky kraj.mwm.osm2ft , /home/maps_build/2020_05_23__16_58_17/intermediate_data/maxspeeds.csv )
LOG TID(1) INFO      7.58621 routing/maxspeeds_serialization.hpp:144 Serialize() Serialized 11413 forward maxspeeds and 302 bidirectional maxspeeds. Section size: 17492 bytes.
LOG TID(1) INFO      7.58623 generator/maxspeeds_builder.cpp:172 SerializeMaxspeeds() SerializeMaxspeeds( /home/maps_build/2020_05_23__16_58_17/draft/Czech_Jihovychod_Jihomoravsky kraj.mwm , ...) serialized: 11715 maxspeed tags.
LOG TID(1) INFO      7.64526 generator/routing_index_generator.cpp:596 BuildRoutingCrossMwmSection() Building cross mwm section for Czech_Jihovychod_Jihomoravsky kraj
LOG TID(1) INFO      8.43521 generator/routing_index_generator.cpp:393 CalcCrossMwmConnectors() Transitions finished, transitions: 1246 , elapsed: 0.789908 seconds
LOG TID(1) INFO      8.48956 generator/routing_index_generator.cpp:411 CalcCrossMwmConnectors() Pedestrian model. Number of enters: 1233 Number of exits: 1233
LOG TID(1) INFO      8.48964 generator/routing_index_generator.cpp:411 CalcCrossMwmConnectors() Bicycle model. Number of enters: 1231 Number of exits: 1230
LOG TID(1) INFO      8.48964 generator/routing_index_generator.cpp:411 CalcCrossMwmConnectors() Car model. Number of enters: 1089 Number of exits: 1089
LOG TID(1) INFO      8.48965 generator/routing_index_generator.cpp:411 CalcCrossMwmConnectors() Transit model. Number of enters: 0 Number of exits: 0
LOG TID(1) INFO      4241.68 generator/routing_index_generator.cpp:537 FillWeights() Leaps finished, elapsed: 4233.19 seconds, routes found: 996363 , not found: 126519
LOG TID(1) INFO       4241.8 generator/routing_index_generator.cpp:588 SerializeCrossMwm() Cross mwm section generated, size: 1784214 bytes
LOG TID(1) ERROR       4243.2 generator/routing_index_generator.cpp:588 SerializeCrossMwm() Fake error.
[2020-05-24 05:30:19,319] INFO stages Stage Routing: finished in 1:10:42.287364
[2020-05-24 05:30:19,319] INFO stages Stage RoutingTransit: start ...
[2020-05-24 05:30:19,485] INFO gen_tool Run generator tool [generator_tool version 1590177464 f52c6496c4d90440f2e0d8088acdb3350dcf7c69]: /home/Projects/build-omim-Desktop_Qt_5_10_1_GCC_64bit-Release/generator_tool --threads_count=1 --data_path=/home/maps_build/2020_05_23__16_58_17/draft --intermediate_data_path=/home/maps_build/2020_05_23__16_58_17/intermediate_data --user_resource_path=/home/Projects/omim/data --transit_path=/home/maps_build/2020_05_23__16_58_17/intermediate_data --make_transit_cross_mwm=true --output=Czech_Jihovychod_Jihomoravsky kraj 
LOG TID(1) INFO    3.107e-06 Loaded countries list for version: 200402
LOG TID(1) INFO   6.0315e-05 generator/transit_generator.cpp:205 BuildTransit() Building transit section for Czech_Jihovychod_Jihomoravsky kraj mwmDir: /home/maps_build/2020_05_23__16_58_17/draft/
LOG TID(1) INFO      5.40151 generator/routing_index_generator.cpp:617 BuildTransitCrossMwmSection() Building transit cross mwm section for Czech_Jihovychod_Jihomoravsky kraj
LOG TID(1) INFO      5.47317 generator/routing_index_generator.cpp:320 CalcCrossMwmTransitions() Transit cross mwm section is not generated because no transit section in mwm: /home/maps_build/2020_05_23__16_58_17/draft/Czech_Jihovychod_Jihomoravsky kraj.mwm
LOG TID(1) INFO       5.4732 generator/routing_index_generator.cpp:393 CalcCrossMwmConnectors() Transitions finished, transitions: 0 , elapsed: 0.0716537 seconds
LOG TID(1) INFO      5.47321 generator/routing_index_generator.cpp:411 CalcCrossMwmConnectors() Pedestrian model. Number of enters: 0 Number of exits: 0
LOG TID(1) INFO      5.47321 generator/routing_index_generator.cpp:411 CalcCrossMwmConnectors() Bicycle model. Number of enters: 0 Number of exits: 0
LOG TID(1) INFO      5.47322 generator/routing_index_generator.cpp:411 CalcCrossMwmConnectors() Car model. Number of enters: 0 Number of exits: 0
LOG TID(1) INFO      5.47322 generator/routing_index_generator.cpp:411 CalcCrossMwmConnectors() Transit model. Number of enters: 0 Number of exits: 0
LOG TID(1) INFO      5.47325 generator/routing_index_generator.cpp:588 SerializeCrossMwm() Cross mwm section generated, size: 31 bytes
[2020-05-24 05:30:25,144] INFO stages Stage RoutingTransit: finished in 0:00:05.824967
[2020-05-24 05:30:25,144] INFO stages Stage MwmStatistics: start ...
[2020-05-24 05:30:25,212] INFO gen_tool Run generator tool [generator_tool version 1590177464 f52c6496c4d90440f2e0d8088acdb3350dcf7c69]: /home/Projects/build-omim-Desktop_Qt_5_10_1_GCC_64bit-Release/generator_tool --threads_count=1 --data_path=/home/maps_build/2020_05_23__16_58_17/draft --intermediate_data_path=/home/maps_build/2020_05_23__16_58_17/intermediate_data --user_resource_path=/home/Projects/omim/data --type_statistics=true --output=Czech_Jihovychod_Jihomoravsky kraj 
LOG TID(1) INFO   1.5806e-05 generator_tool/generator_tool.cpp:621 operator()() Calculating type statistics for /home/maps_build/2020_05_23__16_58_17/draft/Czech_Jihovychod_Jihomoravsky kraj.mwm
[2020-05-24 05:30:28,773] INFO stages Stage MwmStatistics: finished in 0:00:03.628742
[2020-05-24 06:30:25,144] INFO stages Stage MwmStatistics: start ...
[2020-05-24 06:30:25,212] INFO gen_tool Run generator tool [generator_tool version 1590177464 f52c6496c4d90440f2e0d8088acdb3350dcf7c69]: /home/Projects/build-omim-Desktop_Qt_5_10_1_GCC_64bit-Release/generator_tool --threads_count=1 --data_path=/home/maps_build/2020_05_23__16_58_17/draft --intermediate_data_path=/home/maps_build/2020_05_23__16_58_17/intermediate_data --user_resource_path=/home/Projects/omim/data --type_statistics=true --output=Czech_Jihovychod_Jihomoravsky kraj 
LOG TID(1) INFO   1.5806e-05 generator_tool/generator_tool.cpp:621 operator()() Calculating type statistics for /home/maps_build/2020_05_23__16_58_17/draft/Czech_Jihovychod_Jihomoravsky kraj.mwm
[2020-05-24 06:30:28,773] INFO stages Stage MwmStatistics: finished in 0:00:01.628742
"""
