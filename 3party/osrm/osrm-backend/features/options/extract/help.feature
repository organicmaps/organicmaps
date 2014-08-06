@extract @options @help
Feature: osrm-extract command line options: help

    Background:
        Given the profile "testbot"

    Scenario: osrm-extract - Help should be shown when no options are passed
        When I run "osrm-extract"
        Then stderr should be empty
        And stdout should contain "osrm-extract <input.osm/.osm.bz2/.osm.pbf> [options]:"
        And stdout should contain "Options:"
        And stdout should contain "--version"
        And stdout should contain "--help"
        And stdout should contain "--config"
        And stdout should contain "Configuration:"
        And stdout should contain "--profile"
        And stdout should contain "--threads"
        And stdout should contain 12 lines
        And it should exit with code 0

    Scenario: osrm-extract - Help, short
        When I run "osrm-extract -h"
        Then stderr should be empty
        And stdout should contain "osrm-extract <input.osm/.osm.bz2/.osm.pbf> [options]:"
        And stdout should contain "Options:"
        And stdout should contain "--version"
        And stdout should contain "--help"
        And stdout should contain "--config"
        And stdout should contain "Configuration:"
        And stdout should contain "--profile"
        And stdout should contain "--threads"
        And stdout should contain 12 lines
        And it should exit with code 0

    Scenario: osrm-extract - Help, long
        When I run "osrm-extract --help"
        Then stderr should be empty
        And stdout should contain "osrm-extract <input.osm/.osm.bz2/.osm.pbf> [options]:"
        And stdout should contain "Options:"
        And stdout should contain "--version"
        And stdout should contain "--help"
        And stdout should contain "--config"
        And stdout should contain "Configuration:"
        And stdout should contain "--profile"
        And stdout should contain "--threads"
        And stdout should contain 12 lines
        And it should exit with code 0
