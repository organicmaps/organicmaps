@routing @testbot @opposite
Feature: Separate settings for forward/backward direction

    Background:
        Given the profile "testbot"

    Scenario: Testbot - Going against the flow
        Given the node map
            | a | b | c | d |

        And the ways
            | nodes | highway |
            | abcd  | river   |

        When I route I should get
            | from | to | route | distance  | speed        |
            | a    | d  | abcd  | 300 +- 1m | 36 km/h      |
            | d    | a  | abcd  | 300 +- 1m | 16 km/h +- 1 |
