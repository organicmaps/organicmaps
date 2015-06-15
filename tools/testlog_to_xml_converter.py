#!/usr/bin/env python

'''
This script generates jUnit-style xml files from the log written by our tests.
This xml file is used in Jenkins to show the state of the test execution.

Created on May 13, 2015

@author: t.danshin
'''

from __future__ import print_function
import getopt
import sys
import xml.etree.ElementTree as ElementTree



class TestInfo:


    def __init__(self):
        self.obj_is_valid = False
        self.test_name = None
        self.test_comment = None
        self.test_result = None
        self.test_duration = 0.0

    def set_name(self, test_name):
        self.obj_is_valid = True
        self.test_suite, name = test_name.split("::", 1)
        
        self.test_suite = self.test_suite[0: -4]
        name = name.replace("::", ".")
        
        self.test_name = name
        
    def is_valid(self):
        return self.obj_is_valid


    def append_comment(self, comment):
        if self.test_comment is None:
            self.test_comment = comment
        else:
            self.test_comment += "\n" + comment


    def __repr__(self):
        local_comment = ""
        if self.test_comment is not None:
            local_comment = self.test_comment
        return "{suite}::{name}: {comment} -> {result}\n".format(suite=self.test_suite,
                                                                 name=self.test_name,
                                                                 comment=local_comment,
                                                                 result=self.test_result)


    def xml(self):
        d = ElementTree.Element("testcase", {"name":self.test_name,
                                             "classname":self.test_suite,
                                             "time":str(self.test_duration)})
        if self.test_comment is not None:
            b = ElementTree.SubElement(d, "system-err")
            b.text = self.test_comment

        if self.test_result == "FAILED":
            ElementTree.SubElement(d, "failure")
        return d

    def set_duration(self, milis):
        self.test_duration = float(milis) 


class Parser:

    def __init__(self, logfile, xml_file):
        self.logfile = logfile if logfile else "testlog.log"
        self.xml_file = xml_file if xml_file else "test_results.xml"

        self.root = ElementTree.Element("testsuite")


    def parse_log_file(self):

        with open(self.logfile) as f:
            test_info = None

            for line in f.readlines():
                
                if test_info == None:
                    test_info = TestInfo()
                
                line = line.rstrip().decode('utf-8')

                if line.startswith("Running"):
                    test_info.set_name(line[len("Running "):])
                    
                elif line.startswith("Test took"):
                    test_info.set_duration(line[len("Test took "):-3])
                    if test_info.is_valid():
                        self.root.append(test_info.xml())

                    test_info = None

                elif line == "OK" or line.startswith("FAILED"):
                    test_info.test_result = line
                    if line.startswith("FAILED"):
                        test_info.append_comment(line[len("FAILED"):])

                else:
                    test_info.append_comment(line)
                        

    def write_xml_file(self):
        ElementTree.ElementTree(self.root).write(self.xml_file, encoding="UTF-8")
                        
                        
def usage():
    print("""
Possbile options:

-h --help   : print this help

-i --input : the path of the original log file to parse

-o --output: the path of the output xml file 


Example

./testlog_to_xml_converter.py -i testlog.log -o /Users/t.danshin/Desktop/testxml.xml

""")


def read_cl_options():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hi:o:",
                                   ["help", "input=", "output="])
    except getopt.GetoptError as err:
        print(str(err))
        usage()
        sys.exit(2)

    ret = {}
    
    for option, argument in opts:
        if option in ("-h", "--help"):
            usage()
            sys.exit()
        elif option in ("-i", "--input"):
            ret["logfile"] = argument
        elif option in ("-o", "--output"):
            ret["xml_file"] = argument
        else:
            assert False, "unhandled option"
            
    return ret


def main():
    options = read_cl_options()
    try:
        parser = Parser(options["logfile"], options["xml_file"])
    except:
        usage()
        exit(2)
    parser.parse_log_file()
    parser.write_xml_file()

    print("\nFinished writing the xUnit-style xml file\n")


if __name__ == '__main__':
    main()
