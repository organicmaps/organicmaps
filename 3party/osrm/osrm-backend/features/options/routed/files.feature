@routed @options @files
Feature: osrm-routed command line options: files
# Normally when launching osrm-routed, it will keep running as a server until it's shut down.
# For testing program options, the --trial option is used, which causes osrm-routed to quit
# immediately after initialization. This makes testing easier and faster.
#
# The {base} part of the options to osrm-routed will be expanded to the actual base path of
# the preprocessed file.

    Background:
        Given the profile "testbot"
        And the node map
            | a | b |
        And the ways
            | nodes |
            | ab    |
        And the data has been prepared

    Scenario: osrm-routed - Passing base file
        When I run "osrm-routed {base}.osrm --trial"
        Then stdout should contain /^\[info\] starting up engines/
        And stdout should contain /\d{1,2}\.\d{1,2}\.\d{1,2}/
        And stdout should contain /compiled at/
        And stdout should contain /^\[info\] loaded plugin: viaroute/
        And stdout should contain /^\[info\] trial run/
        And stdout should contain /^\[info\] shutdown completed/
        And it should exit with code 0