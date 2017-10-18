#!/usr/bin/env python2.7
# Generates transit graph for MWM transit section generator.
# Also shows preview of transit scheme lines.
import argparse
import json
import math
import matplotlib.pyplot as plt
import numpy as np
import os.path
import sys, io

import bezier_curves
import transit_color_palette


class OsmIdCode:
    NODE = 0x4000000000000000
    WAY = 0x8000000000000000
    RELATION = 0xC000000000000000
    RESET = ~(NODE | WAY | RELATION)

    TYPE2CODE = {
        'n': NODE,
        'r': RELATION,
        'w': WAY
    }


def get_extended_osm_id(osm_id, osm_type):
    try:
        return str(osm_id | OsmIdCode.TYPE2CODE[osm_type[0]])
    except KeyError:
        raise ValueError('Unknown OSM type: ' + osm_type)


def get_line_id(road_id, line_index):
    return road_id << 8 | line_index


def get_interchange_node_id(min_stop_id):
    return 1 << 62 | min_stop_id


def clamp(value, min_value, max_value):
    return max(min(value, max_value), min_value)


def get_mercator_point(lat, lon):
    lat = clamp(lat, -86.0, 86.0)
    sin_x = math.sin(math.radians(lat))
    y = math.degrees(0.5 * math.log((1.0 + sin_x) / (1.0 - sin_x)))
    y = clamp(y, -180, 180)
    return {'x': lon, 'y': y}


class TransitGraphBuilder:
    def __init__(self, input_data, transit_colors, points_per_curve=100, alpha=0.5):
        self.palette = transit_color_palette.Palette(transit_colors)
        self.input_data = input_data
        self.points_per_curve = points_per_curve
        self.alpha = alpha
        self.networks = []
        self.lines = []
        self.stops = {}
        self.interchange_nodes = set()
        self.transfers = {}
        self.gates = {}
        self.edges = []
        self.segments = {}
        self.shapes = []
        self.transit_graph = None

    def __get_average_stops_point(self, stop_ids):
        """Returns an average position of the stops."""
        count = len(stop_ids)
        if count == 0:
            raise ValueError('Average stops point calculation failed: the list of stop id is empty.')
        average_point = [0, 0]
        for stop_id in stop_ids:
            point = self.__get_stop(stop_id)['point']
            average_point[0] += point['x']
            average_point[1] += point['y']
        return [average_point[0] / count, average_point[1] / count]

    def __add_gate(self, osm_id, is_entrance, is_exit, point, weight, stop_id):
        """Creates a new gate or adds information to the existing with the same weight."""
        if (osm_id, weight) in self.gates:
            gate_ref = self.gates[(osm_id, weight)]
            if stop_id not in gate_ref['stop_ids']:
                gate_ref['stop_ids'].append(stop_id)
            gate_ref['entrance'] |= is_entrance
            gate_ref['exit'] |= is_exit
            return
        gate = {'osm_id': osm_id,
                'point': point,
                'weight': weight,
                'stop_ids': [stop_id],
                'entrance': is_entrance,
                'exit': is_exit}
        self.gates[(osm_id, weight)] = gate

    def __get_interchange_node(self, stop_id):
        """Returns the existing interchange node or creates a new one."""
        for node_stops in self.interchange_nodes:
            if stop_id in node_stops:
                return node_stops
        return (stop_id,)

    def __get_stop(self, stop_id):
        """Returns the stop or the interchange node."""
        if stop_id in self.stops:
            return self.stops[stop_id]
        return self.transfers[stop_id]

    def __read_stops(self):
        """Reads stops, their exits and entrances."""
        for stop_item in self.input_data['stops']:
            stop = {}
            stop['id'] = stop_item['id']
            stop['osm_id'] = get_extended_osm_id(stop_item['osm_id'], stop_item['osm_type'])
            if 'zone_id' in stop_item:
                stop['zone_id'] = stop_item['zone_id']
            stop['point'] = get_mercator_point(stop_item['lat'], stop_item['lon'])
            stop['line_ids'] = []
            # TODO: Save stop names stop_item['name'] and stop_item['int_name'] for text anchors calculation.
            stop['title_anchors'] = []
            self.stops[stop['id']] = stop

            for entrance_item in stop_item['entrances']:
                ex_id = get_extended_osm_id(entrance_item['node_id'], 'n')
                point = get_mercator_point(entrance_item['lat'], entrance_item['lon'])
                self.__add_gate(ex_id, True, False, point, entrance_item['distance'], stop['id'])

            for exit_item in stop_item['exits']:
                ex_id = get_extended_osm_id(exit_item['node_id'], 'n')
                point = get_mercator_point(exit_item['lat'], exit_item['lon'])
                self.__add_gate(ex_id, False, True, point, exit_item['distance'], stop['id'])

    def __read_transfers(self):
        """Reads transfers between stops."""
        for transfer_item in self.input_data['transfers']:
            transfer_edge = {'stop_1_id': transfer_item[0],
                             'stop_2_id': transfer_item[1],
                             'weight': transfer_item[2],
                             'transfer': True}
            self.edges.append(transfer_edge)

    def __read_networks(self):
        """Reads networks and routes."""
        for network_item in self.input_data['networks']:
            network_id = network_item['agency_id']
            network = {'id': network_id,
                       'title': network_item['network']}
            self.networks.append(network)

            for route_item in network_item['routes']:
                line_index = 0
                # Create a line for each itinerary.
                for line_item in route_item['itineraries']:
                    if 'name' in line_item:
                        line_name = '{0} ({1})'.format(route_item['name'], line_item[name])
                    else:
                        line_name = route_item['name']
                    line_stops = line_item['stops']
                    line_id = get_line_id(route_item['route_id'], line_index)
                    line = {'id': line_id,
                            'type': route_item['type'],
                            'network_id': network_id,
                            'title': line_name,
                            'number': route_item['ref'],
                            'stop_ids': line_stops}
                    if 'colour' in route_item:
                        line['color'] = self.palette.get_nearest_color(route_item['colour'])

                    # TODO: Add processing of line_item['shape'] when this data will be available.
                    # TODO: Add processing of line_item['trip_ids'] when this data will be available.

                    # Create an edge for each connection of stops.
                    for i in range(len(line_stops)):
                        self.stops[line_stops[i]]['line_ids'].append(line_id)
                        if i < len(line_stops) - 1:
                            edge = {'stop_1_id': line_stops[i],
                                    'stop_2_id': line_stops[i + 1],
                                    'transfer': False,
                                    'line_id': line_id,
                                    'shape_ids': []}
                            self.edges.append(edge)

                    self.lines.append(line)
                    line_index += 1

    def __generate_transfer_nodes(self):
        """Merges stops into transfer nodes."""
        for edge in self.edges:
            if edge['transfer']:
                node1 = self.__get_interchange_node(edge['stop_1_id'])
                node2 = self.__get_interchange_node(edge['stop_2_id'])
                merged_node = tuple(sorted(set(node1 + node2)))
                self.interchange_nodes.discard(node1)
                self.interchange_nodes.discard(node2)
                self.interchange_nodes.add(merged_node)

        for node_stop_ids in self.interchange_nodes:
            point = self.__get_average_stops_point(node_stop_ids)
            transfer = {'id': get_interchange_node_id(self.stops[node_stop_ids[0]]['id']),
                        'stop_ids': list(node_stop_ids),
                        'point': {'x': point[0], 'y': point[1]},
                        'title_anchors': []}

            for stop_id in node_stop_ids:
                self.stops[stop_id]['transfer_id'] = transfer['id']

            self.transfers[transfer['id']] = transfer

    def __collect_segments(self):
        """Prepares collection of segments for shapes generation."""
        # Each line divided on segments by its stops and transfer nodes.
        # Merge equal segments from different lines into a single one and collect adjacent stops of that segment.
        # Average positions of these stops will be used as guide points for a curve generation.
        for line in self.lines:
            prev_seg = None
            prev_id1 = None
            for i in range(len(line['stop_ids']) - 1):
                node1 = self.stops[line['stop_ids'][i]]
                node2 = self.stops[line['stop_ids'][i + 1]]
                id1 = node1.get('transfer_id', node1['id'])
                id2 = node2.get('transfer_id', node2['id'])
                seg = tuple(sorted([id1, id2]))
                if seg not in self.segments:
                    self.segments[seg] = {'guide_points': {id1: set(), id2: set()}}
                if prev_seg is not None:
                    self.segments[seg]['guide_points'][id1].add(prev_id1)
                    self.segments[prev_seg]['guide_points'][id1].add(id2)
                prev_seg = seg
                prev_id1 = id1

    def __generate_shapes_for_segments(self):
        """Generates a curve for each connection of two stops / transfer nodes."""
        shape_id = 0
        for (id1, id2), info in self.segments.items():
            info['shape_id'] = shape_id
            shape_id += 1

            point1 = [self.__get_stop(id1)['point']['x'], self.__get_stop(id1)['point']['y']]
            point2 = [self.__get_stop(id2)['point']['x'], self.__get_stop(id2)['point']['y']]

            if info['guide_points'][id1]:
                guide1 = self.__get_average_stops_point(info['guide_points'][id1])
            else:
                guide1 = [2 * point1[0] - point2[0], 2 * point1[1] - point2[1]]

            if info['guide_points'][id2]:
                guide2 = self.__get_average_stops_point(info['guide_points'][id2])
            else:
                guide2 = [2 * point2[0] - point1[0], 2 * point2[1] - point1[1]]

            curve_points = bezier_curves.segment_to_Catmull_Rom_curve(guide1, point1, point2, guide2,
                                                                      self.points_per_curve, self.alpha)
            info['curve'] = np.array(curve_points)

            polyline = []
            for point in curve_points:
                polyline.append({'x': point[0], 'y': point[1]})

            shape = {'id': shape_id,
                     'stop1_id': id1,
                     'stop2_id': id2,
                     'polyline': polyline}
            self.shapes.append(shape)

    def __assign_shapes_to_edges(self):
        """Assigns a shape to each non-transfer edge."""
        for edge in self.edges:
            if not edge['transfer']:
                stop1 = self.stops[edge['stop_1_id']]
                stop2 = self.stops[edge['stop_2_id']]
                id1 = stop1.get('transfer_id', stop1['id'])
                id2 = stop2.get('transfer_id', stop2['id'])
                seg = tuple(sorted([id1, id2]))
                if seg in self.segments:
                    edge['shape_ids'].append(self.segments[seg]['shape_id'])

    def __create_scheme_shapes(self):
        self.__collect_segments()
        self.__generate_shapes_for_segments()
        self.__assign_shapes_to_edges()

    def build(self):
        if self.transit_graph is not None:
            return self.transit_graph

        self.__read_stops()
        self.__read_transfers()
        self.__read_networks()
        self.__generate_transfer_nodes()
        self.__create_scheme_shapes()

        self.transit_graph = {'networks': self.networks,
                              'lines': self.lines,
                              'gates': self.gates.values(),
                              'stops': self.stops.values(),
                              'transfers': self.transfers.values(),
                              'shapes': self.shapes,
                              'edges': self.edges}
        return self.transit_graph

    def show_preview(self):
        for (s1, s2), info in self.segments.items():
            plt.plot(info['curve'][:, 0], info['curve'][:, 1], 'g')
        for stop in self.stops.values():
            if 'transfer_id' in stop:
                point = self.transfers[stop['transfer_id']]['point']
                size = 60
                color = 'r'
            else:
                point = stop['point']
                if len(stop['line_ids']) > 2:
                    size = 40
                    color = 'b'
                else:
                    size = 20
                    color = 'g'
            plt.scatter([point['x']], [point['y']], size, color)
        plt.show()


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('input_file', help='input file name of transit data')
    parser.add_argument('output_file', nargs='?', help='output file name of generated graph')
    default_colors_path = os.path.dirname(os.path.abspath(__file__)) + '/../../../data/transit_colors.txt'
    parser.add_argument('-c', '--colors', type=str, default=default_colors_path,
                        help='transit colors file COLORS_FILE_PATH', metavar='COLORS_FILE_PATH')
    parser.add_argument('-p', '--preview', action="store_true", default=False,
                        help="show preview of the transit scheme")

    parser.add_argument('-a', '--alpha', type=float, default=0.5, help='the curves generator parameter value ALPHA',
                        metavar='ALPHA')
    parser.add_argument('-n', '--num', type=int, default=100, help='the number NUM of points in a generated curve',
                        metavar='NUM')

    args = parser.parse_args()

    with open(args.input_file, 'r') as input_file:
        data = json.load(input_file)

    with open(args.colors, 'r') as colors_file:
        colors = json.load(colors_file)

    transit = TransitGraphBuilder(data, colors, args.num, args.alpha)
    result = transit.build()

    output_file = args.output_file
    if output_file is None:
        head, tail = os.path.split(os.path.abspath(args.input_file))
        name, extension = os.path.splitext(tail)
        output_file = os.path.join(head, name + '.transit' + extension)
    with io.open(output_file, 'w', encoding='utf8') as json_file:
        result_data = json.dumps(result, ensure_ascii=False, indent=4, sort_keys=True)
        json_file.write(unicode(result_data))
    print 'Transit graph generated:', output_file

    if args.preview:
        transit.show_preview()
