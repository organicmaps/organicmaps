@prepare @options @help
Feature: osrm-prepare command line options: help

    Background:
        Given the profile "testbot"

    Scenario: osrm-prepare - Help should be shown when no options are passed
        When I run "osrm-prepare"
        Then stderr should be empty
        And stdout should contain "osrm-prepare <input.osrm> [options]:"
        And stdout should contain "Options:"
        And stdout should contain "--version"
        And stdout should contain "--help"
        And stdout should contain "--config"
        And stdout should contain "Configuration:"
        And stdout should contain "--restrictions"
        And stdout should contain "--profile"
        And stdout should contain "--threads"
        And stdout should contain 15 lines
        And it should exit with code 0

    Scenario: osrm-prepare - Help, short
        When I run "osrm-prepare -h"
        Then stderr should be empty
        And stdout should contain "osrm-prepare <input.osrm> [options]:"
        And stdout should contain "Options:"
        And stdout should contain "--version"
        And stdout should contain "--help"
        And stdout should contain "--config"
        And stdout should contain "Configuration:"
        And stdout should contain "--restrictions"
        And stdout should contain "--profile"
        And stdout should contain "--threads"
        And stdout should contain 15 lines
        And it should exit with code 0

    Scenario: osrm-prepare - Help, long
        When I run "osrm-prepare --help"
        Then stderr should be empty
        And stdout should contain "osrm-prepare <input.osrm> [options]:"
        And stdout should contain "Options:"
        And stdout should contain "--version"
        And stdout should contain "--help"
        And stdout should contain "--config"
        And stdout should contain "Configuration:"
        And stdout should contain "--restrictions"
        And stdout should contain "--profile"
        And stdout should contain "--threads"
        And stdout should contain 15 lines
        And it should exit with code 0
