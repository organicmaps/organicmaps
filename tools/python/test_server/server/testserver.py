"""
A simple HTTP server used by the C++ downloader/http_client tests.

Started by start_server.py (which waits for a real HTTP probe to succeed)
and stopped by stop_server.py (which issues /kill). The server self-destructs
LIFESPAN seconds after the last request as a safety net in case the test run
is aborted before stop_server.py runs.

Only one instance may run at a time on PORT. If the port is already bound
(e.g. a leftover server from a previous run), startup fails fast and
start_server.py surfaces the error with the captured log for diagnosis.
"""


from __future__ import print_function

from http.server import BaseHTTPRequestHandler
from http.server import HTTPServer
from socketserver import ThreadingMixIn
from ResponseProvider import Payload
from ResponseProvider import ResponseProvider
from ResponseProvider import ResponseProviderMixin
from threading import Timer
from config import LIFESPAN, PORT
import os
import socket
import sys
import threading
import traceback


import logging
import logging.config

try:
    from tornado_handler import MainHandler
    USE_TORNADO = True
except:
    USE_TORNADO = False


logging.basicConfig(format='%(asctime)s %(message)s', level=logging.DEBUG)


class InternalServer(ThreadingMixIn, HTTPServer):
    daemon_threads = True

    def kill_me(self):
        self.shutdown()
        logging.info(f"The server's life has come to an end, pid: {os.getpid()}")


    def reset_selfdestruct_timer(self):
        if self.self_destruct_timer:
            self.self_destruct_timer.cancel()

        self.self_destruct_timer = Timer(LIFESPAN, self.kill_me)
        self.self_destruct_timer.start()


    def __init__(self, server_address, RequestHandlerClass,
                 bind_and_activate=True):

        HTTPServer.__init__(self, server_address, RequestHandlerClass,
                            bind_and_activate=bind_and_activate)

        self.self_destruct_timer = None
        self.clients = 1
        self.reset_selfdestruct_timer()


    def suicide(self):
        self.clients -= 1
        if self.clients == 0:
            if self.self_destruct_timer is not None:
                self.self_destruct_timer.cancel()

            quick_and_painless_timer = Timer(0.1, self.kill_me)
            quick_and_painless_timer.start()


class TestServer:

    def __init__(self):

        self.may_serve = False
        self.server = None

        pid = os.getpid()
        logging.info(f"Init server. Pid: {pid}")

        try:
            self.init_server()
            logging.info(f"Started server with pid: {pid}")
            self.may_serve = True
        except socket.error as e:
            logging.error(f"Failed to bind port {PORT}: {e}")
        except Exception as e:
            logging.error(f"Failed to start serving: {e}")
            traceback.print_exc()

    def init_server(self):

        if USE_TORNADO:
            MainHandler.init_server(PORT, LIFESPAN)
        else:
            print("""
*************
WARNING: Using the python's built-in BaseHTTPServer!
It is all right if you run the tests on your local machine, but if you are running tests on a server,
please consider installing Tornado. It is a much more powerful web-server. Otherwise you will find
that some of your downloader tests either fail or hang.

do

sudo pip install tornado

or go to http://www.tornadoweb.org/en/stable/ for more detail.
*************
""")

            self.server = InternalServer(('localhost', PORT), PostHandler)



    def start_serving(self):
        if not self.may_serve:
            return

        if USE_TORNADO:
            MainHandler.start_serving()

        else:
            thread = threading.Thread(target=self.server.serve_forever)
            thread.deamon = True
            thread.start()


class PostHandler(BaseHTTPRequestHandler, ResponseProviderMixin):

    def dispatch_response(self, payload):

        self.send_response(payload.response_code())
        for h in payload.headers():
            self.send_header(h, payload.headers()[h])
        self.send_header("Content-Length", payload.length())
        self.end_headers()
        self.wfile.write(payload.message())


    def init_vars(self):
        self.response_provider = ResponseProvider(self)
        # Every request must extend the server's lifespan. Previously only
        # do_POST reset the timer, so long GET-only test runs could die at
        # the LIFESPAN boundary on slow CI.
        self.server.reset_selfdestruct_timer()


    def do_POST(self):
        self.init_vars()
        headers = self.prepare_headers()
        payload = self.response_provider.response_for_url_and_headers(self.path, headers)
        if payload.response_code() >= 300:
            length = int(self.headers.get('content-length'))
            self.dispatch_response(Payload(self.rfile.read(length)))
        else:
            self.dispatch_response(payload)

    do_PUT = do_POST

    def do_HEAD(self):
        headers = self.prepare_headers()
        self.init_vars()
        payload = self.response_provider.response_for_url_and_headers(self.path, headers)
        self.send_response(payload.response_code())
        for h in payload.headers():
            self.send_header(h, payload.headers()[h])
        self.send_header("Content-Length", payload.length())
        self.end_headers()

    def do_GET(self):
        headers = self.prepare_headers()
        self.init_vars()
        self.dispatch_response(self.response_provider.response_for_url_and_headers(self.path, headers))


    def prepare_headers(self):
        ret = dict()
        for h in self.headers:
            ret[h.lower()] = self.headers.get(h)
        return ret


    def got_pinged(self):
        self.server.clients += 1


    def kill(self):
        logging.debug("Kill called in testserver")
        self.server.suicide()


if __name__ == '__main__':

    server = TestServer()
    if not server.may_serve:
        sys.exit(1)
    server.start_serving()
