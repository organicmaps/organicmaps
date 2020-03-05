"""
This is a simple web-server that does very few things. It is necessary for 
the downloader tests.  

Here is the logic behind the initialization:
Because several instances of the test can run simultaneously on the Build 
machine, we have to take this into account and not start another server if 
one is already running. However, there is a chance that a server will not
terminate correctly, and will still hold the port, so we will not be able 
to initialize another server. 

So before initializing the server, we check if any processes are using the port
that we want to use. If we find such a process, we assume that it might be
working, and wait for about 10 seconds for it to start serving. If it does not, 
we kill it. 

Next, we check the name of our process and see if there are other processes
with the same name. If there are, we assume that they might start serving any 
moment. So we iterate over the ones that have PID lower than ours, and wait
for them to start serving. If a process doesn't serve, we kill it.

If we have killed (or someone has) all the processes with PIDs lower than ours,
we try to start serving. If we succeed, we kill all other processes with the
same name as ours. If we don't someone else will kill us.
  
"""



from __future__ import print_function

from BaseHTTPServer import BaseHTTPRequestHandler
from BaseHTTPServer import HTTPServer
from ResponseProvider import Payload
from ResponseProvider import ResponseProvider
from ResponseProvider import ResponseProviderMixin
from SiblingKiller import SiblingKiller
from threading import Timer
import os
import socket
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

PORT = 34568
LIFESPAN = 180.0  # timeout for the self destruction timer - how much time 
                  # passes between the last request and the server killing 
                  # itself
PING_TIMEOUT = 5  # Nubmer of seconds to wait for ping response


class InternalServer(HTTPServer):

    def kill_me(self):
        self.shutdown()
        logging.info("The server's life has come to an end, pid: {}".format(os.getpid()))


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
        
        pid = os.getpid()
        logging.info("Init server. Pid: {}".format(pid))
        
        self.server = None
        
        killer = SiblingKiller(PORT, PING_TIMEOUT)
        killer.kill_siblings()
        if killer.allow_serving():
            try:
                self.init_server()
                logging.info("Started server with pid: {}".format(pid))
                self.may_serve = True

            except socket.error:
                logging.info("Failed to start the server: Port is in use")
            except Exception as e:
                logging.debug(e)
                logging.info("Failed to start serving for unknown reason")
                traceback.print_exc()
        else:
            logging.info("Not allowed to start serving for process: {}".format(pid))

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


    def do_POST(self):
        self.init_vars()
        self.server.reset_selfdestruct_timer()
        headers = self.prepare_headers()
        payload = self.response_provider.response_for_url_and_headers(self.path, headers)
        if payload.response_code() >= 300:
            length = int(self.headers.getheader('content-length'))
            self.dispatch_response(Payload(self.rfile.read(length)))
        else:
            self.dispatch_response(payload)


    def do_GET(self):
        headers = self.prepare_headers()
        self.init_vars()
        self.dispatch_response(self.response_provider.response_for_url_and_headers(self.path, headers))


    def prepare_headers(self):
        ret = dict()
        for h in self.headers:
            ret[h] = self.headers.get(h)
        return ret
        

    def got_pinged(self):
        self.server.clients += 1


    def kill(self):
        logging.debug("Kill called in testserver")
        self.server.suicide()


if __name__ == '__main__':
    
    server = TestServer()
    server.start_serving()
