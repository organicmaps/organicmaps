@routing @testbot @fixed
Feature: Fixed bugs, kept to check for regressions

    Background:
        Given the profile "testbot"

    @726
    Scenario: Weird looping, manual input
        Given the node locations
            | node | lat       | lon       |
            | a    | 55.660778 | 12.573909 |
            | b    | 55.660672 | 12.573693 |
            | c    | 55.660128 | 12.572546 |
            | d    | 55.660015 | 12.572476 |
            | e    | 55.660119 | 12.572325 |
            | x    | 55.660818 | 12.574051 |
            | y    | 55.660073 | 12.574067 |

        And the ways
            | nodes |
            | abc   |
            | cdec  |

        When I route I should get
            | from | to | route | turns            |
            | x    | y  | abc   | head,destination |
