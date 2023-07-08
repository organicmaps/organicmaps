@extract @options @files
Feature: osrm-extract command line options: files
# expansions:
# {osm_base} => path to current input file
# {profile} => path to current profile script

    Background:
        Given the profile "testbot"
        And the node map
            | a | b |
        And the ways
            | nodes |
            | ab    |
        And the data has been saved to disk

    Scenario: osrm-extract - Passing base file
        When I run "osrm-extract {osm_base}.osm --profile {profile}"
        Then stderr should be empty
        And it should exit with code 0

    Scenario: osrm-extract - Order of options should not matter
        When I run "osrm-extract --profile {profile} {osm_base}.osm"
        Then stderr should be empty
        And it should exit with code 0

    Scenario: osrm-extract - Missing input file
        When I run "osrm-extract over-the-rainbow.osrm --profile {profile}"
        And stderr should contain "over-the-rainbow.osrm"
        And stderr should contain "not found"
        And it should exit with code 1
