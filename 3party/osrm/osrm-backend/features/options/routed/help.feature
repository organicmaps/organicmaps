@routed @options @help
Feature: osrm-routed command line options: help

    Background:
        Given the profile "testbot"

    Scenario: osrm-routed - Help should be shown when no options are passed
        When I run "osrm-routed"
        Then stderr should be empty
        And stdout should contain "osrm-routed <base.osrm> [<options>]:"
        And stdout should contain "Options:"
        And stdout should contain "--version"
        And stdout should contain "--help"
        And stdout should contain "--config"
        And stdout should contain "--trial"
        And stdout should contain "Configuration:"
        And stdout should contain "--hsgrdata arg"
        And stdout should contain "--nodesdata arg"
        And stdout should contain "--edgesdata arg"
        And stdout should contain "--ramindex arg"
        And stdout should contain "--fileindex arg"
        And stdout should contain "--namesdata arg"
        And stdout should contain "--timestamp arg"
        And stdout should contain "--ip"
        And stdout should contain "--port"
        And stdout should contain "--threads"
        And stdout should contain "--shared-memory"
        And stdout should contain "--max-table-size"
        And stdout should contain "--max-matching-size"
        And stdout should contain 26 lines
        And it should exit with code 0

    Scenario: osrm-routed - Help, short
        When I run "osrm-routed -h"
        Then stderr should be empty
        And stdout should contain "osrm-routed <base.osrm> [<options>]:"
        And stdout should contain "Options:"
        And stdout should contain "--version"
        And stdout should contain "--help"
        And stdout should contain "--config"
        And stdout should contain "--trial"
        And stdout should contain "Configuration:"
        And stdout should contain "--hsgrdata arg"
        And stdout should contain "--nodesdata arg"
        And stdout should contain "--edgesdata arg"
        And stdout should contain "--ramindex arg"
        And stdout should contain "--fileindex arg"
        And stdout should contain "--namesdata arg"
        And stdout should contain "--timestamp arg"
        And stdout should contain "--ip"
        And stdout should contain "--port"
        And stdout should contain "--threads"
        And stdout should contain "--shared-memory"
        And stdout should contain "--max-table-size"
        And stdout should contain "--max-matching-size"
        And stdout should contain 26 lines
        And it should exit with code 0

    Scenario: osrm-routed - Help, long
        When I run "osrm-routed --help"
        Then stderr should be empty
        And stdout should contain "osrm-routed <base.osrm> [<options>]:"
        And stdout should contain "Options:"
        And stdout should contain "--version"
        And stdout should contain "--help"
        And stdout should contain "--config"
        And stdout should contain "--trial"
        And stdout should contain "Configuration:"
        And stdout should contain "--hsgrdata arg"
        And stdout should contain "--nodesdata arg"
        And stdout should contain "--edgesdata arg"
        And stdout should contain "--ramindex arg"
        And stdout should contain "--fileindex arg"
        And stdout should contain "--namesdata arg"
        And stdout should contain "--timestamp arg"
        And stdout should contain "--ip"
        And stdout should contain "--port"
        And stdout should contain "--threads"
        And stdout should contain "--shared-memory"
        And stdout should contain "--max-table-size"
        And stdout should contain "--max-matching-size"
        And stdout should contain 26 lines
        And it should exit with code 0
