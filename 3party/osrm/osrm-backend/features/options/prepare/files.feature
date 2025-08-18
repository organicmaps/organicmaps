@prepare @options @files
Feature: osrm-prepare command line options: files
# expansions:
# {extracted_base} => path to current extracted input file
# {profile} => path to current profile script

    Background:
        Given the profile "testbot"
        And the node map
            | a | b |
        And the ways
            | nodes |
            | ab    |
        And the data has been extracted

    Scenario: osrm-prepare - Passing base file
        When I run "osrm-prepare {extracted_base}.osrm --profile {profile}"
        Then stderr should be empty
        And it should exit with code 0

    Scenario: osrm-prepare - Order of options should not matter
        When I run "osrm-prepare --profile {profile} {extracted_base}.osrm"
        Then stderr should be empty
        And it should exit with code 0

    Scenario: osrm-prepare - Missing input file
        When I run "osrm-prepare over-the-rainbow.osrm --profile {profile}"
        And stderr should contain "over-the-rainbow.osrm"
        And stderr should contain "not found"
        And it should exit with code 1
