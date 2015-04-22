@prepare @options @invalid
Feature: osrm-prepare command line options: invalid options

    Background:
        Given the profile "testbot"

    Scenario: osrm-prepare - Non-existing option
        When I run "osrm-prepare --fly-me-to-the-moon"
        Then stdout should be empty
        And stderr should contain "option"
        And stderr should contain "fly-me-to-the-moon"
        And it should exit with code 1
