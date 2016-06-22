from __future__ import print_function

import logging

import os
import re
import urllib2
import socket
from subprocess import Popen, PIPE
from time import sleep
import sys


import logging

class SiblingKiller:
    
    def __init__(self, port, ping_timeout):
        self.all_processes = self.ps_dash_w()
        self.all_pids = self.all_process_ids() 
        self.__allow_serving = False
        self.__my_pid = self.my_process_id()
        self.port = port
        self.ping_timeout = ping_timeout
        logging.debug("Sibling killer: my process id = {}".format(self.__my_pid))
        
        
    def allow_serving(self):
        return self.__allow_serving    

    def give_process_time_to_kill_port_user(self):
        sleep(5)

    
    def kill_siblings(self):
        """
        The idea is to get the list of all processes by the current user, check which one of them is using the port.
        If there is such a process, let's wait for 10 seconds for it to start serving, if it doesn't, kill it.
        
        If there is NO such process, let's see if there is a process with the same name as us but with a lower process id. 
        We shall wait for 10 seconds for it to start, if it doesn't, kill it.
        
        If we are the process with the same name as ours and with the lowest process id, let's start serving and kill everyone else. 
        """

        if self.wait_for_server():
            self.__allow_serving = False
            logging.debug("There is a server that is currently serving on our port... Disallowing to start a new one")
            return
        
        logging.debug("There are no servers that are currently serving. Will try to kill our siblings.")
        
        
        sibs = self.siblings()
        
        for sibling in sibs:
            logging.debug("Checking whether we should kill sibling id: {}".format(sibling))
            
            self.give_process_time_to_kill_port_user()
            
            if self.wait_for_server():
                serving_pid = self.serving_process_id()
                if serving_pid:
                    logging.debug("There is a serving sibling with process id: {}".format(serving_pid))
                    self.kill(pids=map(lambda x: x != serving_pid, sibs))
                    self.__allow_serving = False
                    return
            else:
                self.kill(pid=sibling)
                
        self.kill_process_on_port() # changes __allow_serving to True if the process was alive and serving
        


    def kill(self, pid=0, pids=list()):
        if not pid and not pids:
            logging.debug("There are no siblings to kill")
            return
        if pid and pids:
            raise Exception("Use either one pid or multiple pids")
        
        hitlist = ""
        if pid:
            hitlist = str(pid)
        if pids:
            hitlist = " ".join(map(str, pids))

        command = "kill -9 {hitlist}".format(hitlist=hitlist)
        self.exec_command(command)
        

    def siblings(self):
        my_name = self.my_process_name()
        return filter(lambda x: x < self.__my_pid,
                          map(lambda x: int(x.split(" ")[1]), 
                              filter(lambda x: my_name in x, self.all_processes)))


    def kill_process_on_port(self):
        process_on_port = self.process_using_port(self.port)
        
        if not self.wait_for_server():
            self.kill(pid=process_on_port)
            self.__allow_serving = True
    
    
    def all_process_ids(self):
        pid = lambda x: int(x.split(" ")[1])
        return map(pid, self.all_processes)


    def process_using_port(self, port):
        lsof = lambda x : (self.exec_command("lsof -a -p{process} -i4".format(process=str(x))), x) #ignore the return code
        listenning_on_port = lambda (info_line, id) : info_line and info_line.endswith("(LISTEN)") and str(port) in info_line
        ids = lambda (info_line, id) : id

        listening_process = map(ids, filter(listenning_on_port, map(lsof, self.all_pids)))
        
        if len(listening_process) > 1:
            pass
            # We should panic here
        
        if not listening_process:
            return None
        
        return listening_process[0]
    
    
    def my_process_id(self):
        return os.getpid()


    def my_process_name(self):
        return " ".join(sys.argv)


    def ps_dash_w(self):
        not_header = lambda x: x and not x.startswith("UID")
        return filter(not_header, self.gen_to_list(re.sub("\s{1,}", " ", x.strip()) for x in self.exec_command("ps -f").split("\n")))


    def wait_for_server(self):
        for i in range(0, 2):
            if self.ping(): # unsuccessful ping takes 5 seconds (look at PING_TIMEOUT) iff there is a dead server occupying the port
                return True
        return False
    
    
    def ping(self):
        html = str()
        try:
            response = urllib2.urlopen('http://localhost:{port}/ping'.format(port=self.port), timeout=self.ping_timeout);
            html = response.read()
        except (urllib2.URLError, socket.timeout):
            pass

        logging.debug("Pinging returned html: {}".format(html))

        return html == "pong"


    def serving_process_id(self):
        resp = str()
        try:
            response = urllib2.urlopen('http://localhost:{port}/id'.format(port=self.port), timeout=self.ping_timeout);
            resp = response.read()
            id = int(resp)
            return id
        
        except:
            logging.info("Couldn't get id of a serving process (the PID of the server that responded to pinging)")
            return None


    def gen_to_list(self, generator):
        l = list()
        for i in generator:
            l.append(i)
        return l
    
   
    def exec_command(self, command):
        logging.debug(">> {}".format(command))
        p = Popen(command, shell=True, stdout=PIPE, stderr=PIPE)
        output, err = p.communicate()
        p.wait()
        return output
