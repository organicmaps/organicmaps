@extract @options @invalid
Feature: osrm-extract command line options: invalid options

    Background:
        Given the profile "testbot"

    Scenario: osrm-extract - Non-existing option
        When I run "osrm-extract --fly-me-to-the-moon"
        Then stdout should be empty
        And stderr should contain "option"
        And stderr should contain "fly-me-to-the-moon"
        And it should exit with code 1
