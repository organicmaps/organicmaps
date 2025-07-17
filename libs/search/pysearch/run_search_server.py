#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from __future__ import print_function
from http.server import BaseHTTPRequestHandler,HTTPServer
import argparse
import json
import os
import pysearch
import urllib.parse

DIR = os.path.dirname(__file__)
RESOURCE_PATH = os.path.realpath(os.path.join(DIR, '..', '..', 'data'))
MWM_PATH = os.path.realpath(os.path.join(DIR, '..', '..', 'data'))
PORT=8080


class HTTPHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        result = urllib.parse.urlparse(self.path)
        query = urllib.parse.parse_qs(result.query)

        def sparam(name):
            return query[name][-1]

        def fparam(name):
            return float(sparam(name))

        params = pysearch.Params()
        try:
            params.query = sparam('query')
            params.locale = sparam('locale')
            params.position = pysearch.Mercator(fparam('posx'), fparam('posy'))
            params.viewport = pysearch.Viewport(
                pysearch.Mercator(fparam('minx'), fparam('miny')),
                pysearch.Mercator(fparam('maxx'), fparam('maxy')))
        except KeyError:
            self.send_response(400)
            return

        results = HTTPHandler.engine.query(params)

        responses = [{'name': result.name,
                      'address': result.address,
                      'has_center': result.has_center,
                      'center': {'x': result.center.x,
                                 'y': result.center.y
                                }
                     }
                     for result in results]

        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        self.wfile.write(json.dumps(responses).encode())


def main(args):
    pysearch.init(args.r, args.m)
    engine = pysearch.SearchEngine()
    HTTPHandler.engine = pysearch.SearchEngine()

    print('Starting HTTP server on port', PORT)
    server = HTTPServer(('', args.p), HTTPHandler)
    server.serve_forever()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-r', metavar='RESOURCE_PATH', default=RESOURCE_PATH,
                        help='Path to resources directory.')
    parser.add_argument('-m', metavar='MWM_PATH', default=MWM_PATH,
                        help='Path to mwm files.')
    parser.add_argument('-p', metavar='PORT', default=PORT,
                        help='Port for the server to listen')
    args = parser.parse_args()

    main(args)
