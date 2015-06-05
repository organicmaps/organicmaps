"""
This is a simple web-server that does very few things. It is necessary for 
the downloader tests. Currently it works only for a subset of tests, it
doesn't yet work for chunked downloads. 

Here is the logic behind the initialization:
Because several instances of the test can run simultaneously on the Build 
machine, we have to take this into account and not start another server if 
one is already running. However, there is a chance that a server will not
terminate correctly, and will still hold the port, so we will not be able 
to initialize another server. 

So, before initalizing a new server we "ping" it. If it replies with "pong", 
we know that a working instance is already running, and we do not start a new 
one.

If it doesn't reply with pong, it might mean that it is either not running 
at all, or it is running, but dead. In this case, we try to init a server, 
and if we catch an exception, we kill all other processes that have the 
word "testserver.py" in the name, but whose ids are not our id, and then 
we init the server once again. If that fails, there is nothing we can do 
about it, so we don't catch any exceptions there.

Another thing to note is that you cannot stop the server from the same thread
as it was started from, and the only way I found possible to kill it is from a
timer. 
  
"""

from __future__ import print_function

from BaseHTTPServer import BaseHTTPRequestHandler
from BaseHTTPServer import HTTPServer
import cgi
from numpy.distutils.exec_command import exec_command
import os
import re
import socket
from subprocess import Popen, PIPE
import sys
import thread
from threading import Timer
import threading
import time
import types
import urllib2
from scipy.stats.stats import trim_mean


PORT = 34568
LIFESPAN = 18000.0  # timeout for the self destruction timer - how much time 
                  # passes between the last request and the server killing 
                  # itself
PING_TIMEOUT = 5  # Nubmer of seconds to wait for ping response


class InternalServer(HTTPServer):



    def kill_me(self):
        print("The server's life has come to an end")
        self.shutdown()

    
    def reset_selfdestruct_timer(self):
        if self.self_destruct_timer is not None:
            self.self_destruct_timer.cancel()
               
        self.self_destruct_timer = Timer(LIFESPAN, self.kill_me)
        self.self_destruct_timer.start()


    def __init__(self, server_address, RequestHandlerClass, 
                 bind_and_activate=True):
        self.self_destruct_timer = None
        self.clients = 1
        HTTPServer.__init__(self, server_address, RequestHandlerClass, 
                            bind_and_activate=bind_and_activate)
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
        self.server = None
        html = str()
        try:
            print("Pinging the server...")
            response = urllib2.urlopen('http://localhost:{port}/ping'.format(port=PORT), timeout=PING_TIMEOUT);
            html = response.read()
        except (urllib2.URLError, socket.timeout):
            print("The server does not currently serve...")
            
        if html != "pong":
            print("html != pong")
            try:
                self.init_server()
            except socket.error:
                print("Killing siblings")
                self.kill_siblings()
                time.sleep(1)
                self.init_server()


    def init_server(self):
        self.server = InternalServer(('localhost', PORT), PostHandler)

        
    def start_serving(self):
        if self.server is not None:
            thread = threading.Thread(target=self.server.serve_forever)
            thread.deamon = True
            thread.start()


    def exec_command(self, command):
        print(command)
        p = Popen(command.split(" "), shell=True, stdout=PIPE, stderr=PIPE)
        output, err = p.communicate()
        p.wait()
        return output[0]


    def kill_siblings(self):
        output = gen_to_list(re.sub("\s{1,}", " ", x.strip()) for x in exec_command("ps -w")[1].split("\n"))
        
        my_pid = str(os.getpid())
        
        my_name =  map(lambda x: x.split(" ")[4], # the name of python script
                       filter(lambda x: x.startswith(my_pid), output))[0]

        siblings = filter(lambda x: x != my_pid, 
                          map(lambda x: x.split(" ")[0], 
                              filter(lambda x: my_name in x, output)))
        
        if len(siblings) > 0:
            command = "kill {siblings}".format(siblings=" ".join(siblings)) 
            exec_command(command)
        else:
            print("The process has no siblings")


def gen_to_list(generator):
    l = []
    for i in generator:
        l.append(i)
    return l
            

BIG_FILE_SIZE = 47684
            
class PostHandler(BaseHTTPRequestHandler):
    
    def init_vars(self):
        self.byterange = None
        self.is_chunked = False
        self.response_code = 200
    

    def do_POST(self):
        self.init_vars()
        self.server.reset_selfdestruct_timer()
        print("URL is: " + self.path)
        ctype, pdict = cgi.parse_header(self.headers.getheader('content-type'))
        
#         print ("My ctype == {}".format(ctype))
        if ctype == 'multipart/form-data':
            self.send_response(500)
            self.end_headers()

        elif ctype == 'application/json':

            length = int(self.headers.getheader('content-length'))
            data = self.rfile.read(length)

            self.send_response(200)
            self.send_header("Content-Length", length)
            self.end_headers()

            self.wfile.write(data)
            
#         print("\n\n~~~\n\n")

    
    def do_GET(self):
        self.init_vars()
        self.chunk_requested()
        
        self.server.reset_selfdestruct_timer()
        switch = {"/unit_tests/1.txt": self.test1,
                  "/unit_tests/notexisting_unittest": self.test_404,
                  "/unit_tests/permanent" : self.test_301,
                  "/unit_tests/47kb.file" : self.test_47_kb,
                  "/ping" : self.pong,
                  "/kill": self.kill,
        }
        switch[self.path]()
#         print("\n\n~~~\n\n")
#         return


    def chunk_requested(self):
        
        the_range = self.headers.get('Range')
        if the_range is not None:
            self.is_chunked = True
            self.response_code = 206
            meaningful_string = the_range[6:]
            first, last = meaningful_string.split("-")
            self.byterange = (int(first), int(last))
        
#         print("My headers are: {0}".format( self.headers))

    def trim_message(self, message):
        if not self.is_chunked:
            return message
        m = message[self.byterange[0]: self.byterange[1] + 1]
        return m

    def pong(self):
        self.server.clients += 1
        self.send_response(200)
        self.end_headers()
        self.wfile.write("pong")


    def message_for_47kb_file(self):
        message = []
        for i in range(0, BIG_FILE_SIZE + 1):
            message.append(chr(i / 256))
            message.append(chr(i % 256))

#            print("{}{}\n".format(i/256, i%256))

        return "".join(message)
            
        
        

    def test1(self):
        message = "Test1"
        message = self.trim_message(message)
        
        
#         print("This is what TEST1 is about to write: {}; {};".format(message, self.response_code))
        
        self.send_response(self.response_code)
        self.send_header("Content-Length", len(message))
        self.end_headers()
        self.wfile.write(message)

        
    def test_404(self):
        self.send_response(404)
        self.end_headers()

        
    def test_301(self):
        self.send_response(301)
        self.send_header("Location", "google.com")
        self.end_headers()


    def test_47_kb(self):
        self.send_response(self.response_code)
        length = BIG_FILE_SIZE
        if self.byterange is None:
            self.byterange = (0, BIG_FILE_SIZE)
        else:
            length = min([length, self.byterange[1] - self.byterange[0] + 1])
            
#         print ("The length of message to return is: {0}".format(length))
        
        self.send_header("Content-Length", length)
        if self.byterange is not None:
            self.send_header("Content-Range", "bytes {start}-{end}/{out_of}".format(start=self.byterange[0], end=self.byterange[1], out_of=BIG_FILE_SIZE))
        self.end_headers()

        message = self.message_for_47kb_file()

        if message is None:
            print("The message is none for some reason")
        self.wfile.write(self.message_for_47kb_file()[self.byterange[0]: self.byterange[1] + 1])
#        for i in range(0, length + 1):
#             try:
#            self.wfile.write(chr(i%256))
#             except IOError, e:
#                 if e.errno == errno.EPIPE:
#                     print("This is indeed a broken pipe")
                    
        # EPIPE error
            
            
    def kill(self):
        message = "Bye..."
        self.send_response(200)
        self.send_header("Content-Length", len(message))
        self.end_headers()
        self.wfile.write(message)
        self.server.suicide()
    
    
if __name__ == '__main__':
    server = TestServer()
    server.start_serving()
