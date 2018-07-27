#!/usr/bin/env python

'''
This script generates jUnit-style xml files from the log written by our tests.
This xml file is used in Jenkins to show the state of the test execution.

Created on May 13, 2015

@author: t.danshin
'''

from __future__ import print_function
import sys
import xml.etree.ElementTree as ElementTree
from optparse import OptionParser
import re

REPLACE_CHARS_RE = re.compile("[\x00-\x1f]")


class PrefixesInLog:
    OK = "OK"
    FAILED = "FAILED"
    BEGIN = "BEGIN: "
    END = "END: "
    RUNNING = "Running "
    TEST_TOOK = "Test took "
    RESULT = "result: "


class TestInfo:

    EXE = "UNKNOWN_COMPILED_FILE"
    NAME = "UNKNOWN_CPP_FILE"
    FAILED = "FAILED"
    PASSED = "PASSED"


    def __init__(self):
        self.test_name = TestInfo.NAME
        self.test_suite = TestInfo.EXE
        self.test_comment = None
        self.test_result = TestInfo.FAILED
        self.test_duration = 0.0

    
    def set_name(self, test_name):
        self.test_name = test_name.replace("::", ".")


    def set_exe_name(self, exe):
        self.test_suite = exe if exe else TestInfo.EXE


    def set_duration(self, milis):
        self.test_duration = float(milis) / 1000 

    
    def set_test_result(self, result_string):
        if result_string.startswith(PrefixesInLog.FAILED):
            self.test_result = TestInfo.FAILED
            self.append_comment(string_after_prefix(result_string, PrefixesInLog.FAILED))
        elif result_string.startswith(PrefixesInLog.OK):
            self.test_result = TestInfo.PASSED
            

    def append_comment(self, comment):
        if not self.test_comment:
            if comment.strip(): # if we don't have a comment to test yet, and the line we got is not an empty string
                self.test_comment = comment
        else:
            try:
                self.test_comment = u"{old_comment}\n{comment}".format(old_comment=self.test_comment, comment=comment)
            except Exception as ex:
                print(comment)
                print(type(ex))
                sys.exit(2)


    def is_empty(self):
        return self.test_name == TestInfo.NAME and self.test_suite == TestInfo.EXE and self.test_comment


    def __repr__(self):
        local_comment = self.test_comment if self.test_comment else str()
        return "{suite}::{name}: {comment} -> {result}\n".format(suite=self.test_suite,
                                                                 name=self.test_name,
                                                                 comment=local_comment,
                                                                 result=self.test_result)


    def xml(self):
        d = ElementTree.Element("testcase", {"name":self.test_name,
                                             "classname":self.test_suite,
                                             "time":str(self.test_duration)})
        
        if self.test_comment:
            b = ElementTree.SubElement(d, "system-err")
            b.text = self.test_comment

        if self.test_result == TestInfo.FAILED:
            fail = ElementTree.SubElement(d, "failure")
            if self.test_comment:
                fail.text = self.test_comment
        return d


class Parser:

    def __init__(self, logfile, xml_file):
        self.logfile = logfile
        self.xml_file = xml_file
        self.current_exe = None
        self.test_info = TestInfo()
        self.var_should_pass = False
        self.root = ElementTree.Element("testsuite")


    def write_xml_file(self):
        ElementTree.ElementTree(self.root).write(self.xml_file, encoding="UTF-8")


    def parse_log_file(self):
        with open(self.logfile) as f:

            PipeEach(f.readlines()).through_functions(
                    self.check_for_exe_boundaries, 
                    self.check_for_testcase_boundaries, 
                    self.check_test_result, 
                    self.should_pass, 
                    self.append_to_comment
            )
                        
                        
    def should_pass(self, line):
        return self.var_should_pass


    def check_for_exe_boundaries(self, line):
        if line.startswith(PrefixesInLog.BEGIN):
            if self.current_exe: #if we never had an End to a Beginning
                self.test_info = TestInfo()
                self.append_to_xml()
                self.var_should_pass = False
            
            self.current_exe = string_after_prefix(line, PrefixesInLog.BEGIN)
            return True 
            
        elif line.startswith(PrefixesInLog.END):
            self.var_should_pass = False
            parts = line.split(" | ")
            end_exe = string_after_prefix(parts[0], PrefixesInLog.END)
            result = int(string_after_prefix(parts[1], PrefixesInLog.RESULT))
            
            if result != 0:
                if not self.test_info:
                    self.test_info = TestInfo()
                    self.test_info.set_exe_name(end_exe)
                    self.test_info.set_name("SOME_TESTS_FAILED")
                self.test_info.set_test_result(TestInfo.FAILED)

            self.append_to_xml()
            
            self.current_exe = None
            return True
        
        return False
    

    def check_for_testcase_boundaries(self, line):
        if line.startswith(PrefixesInLog.RUNNING):
            
            if not self.test_info:
                self.test_info = TestInfo()
            
            self.test_info.set_name(string_after_prefix(line, PrefixesInLog.RUNNING))
            self.test_info.set_exe_name(self.current_exe)
            return True
            
        elif line.startswith(PrefixesInLog.TEST_TOOK):
            self.test_info.set_duration(string_after_prefix(line, PrefixesInLog.TEST_TOOK, end=-3))
            self.append_to_xml()
        
            self.test_info = None
            return True

        return False


    def check_test_result(self, line):
        if line == PrefixesInLog.OK or line.startswith(PrefixesInLog.FAILED):
            self.test_info.set_test_result(line)
            return True
        return False


    def append_to_xml(self):
        if self.test_info:
            self.test_info.set_exe_name(self.current_exe)
            self.root.append(self.test_info.xml())


    def append_to_comment(self, line):
        if self.test_info:
            if line == "All tests passed." or re.match("\d{1,} tests failed", line, re.IGNORECASE):
                self.var_should_pass = True
                return False
            line = REPLACE_CHARS_RE.sub("_", line)
            self.test_info.append_comment(line)
        return False


class PipeEach:
    def __init__(self, iterable_param):
        self.iterable_param = iterable_param
    

    def through_functions(self, *fns):
        for param in self.iterable_param:
            param = param.rstrip().decode('utf-8')

            for fn in fns:
                if fn(param):
                    break   


def string_after_prefix(line, prefix, end=None):
    return line[len(prefix):end] if end else line[len(prefix):]

def read_cl_options():
    
    parser = OptionParser()
    parser.add_option("-o", "--output", dest="output", default="test_results.xml", help="resulting log file. Default testlog.log")
    parser.add_option("-i", "--include", dest="input", default="testlog.log", help="The path to the original log file to parse")
        
    (options, args) = parser.parse_args()

    return options


def main():
    options = read_cl_options()
    parser = Parser(options.input, options.output)

    parser.parse_log_file()
    parser.write_xml_file()

    print("\nFinished writing the xUnit-style xml file\n")


if __name__ == '__main__':
    main()
