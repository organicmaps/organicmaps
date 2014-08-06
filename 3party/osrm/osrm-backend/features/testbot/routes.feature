@routing @testbot @routes @todo
Feature: OSM Route Relation

    Background:
        Given the profile "testbot"

    Scenario: Prioritize ways that are part of route relations
    # This scenario assumes that the testbot uses an impedance of 0.5 for ways that are part of 'testbot' routes.

        Given the node map
            | s |  |  | t |  |  |   |
            | a |  |  | b |  |  | c |
            |   |  |  |   |  |  |   |
            |   |  |  | u |  |  | v |

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | as    |
            | stb   |
            | bu    |
            | uvc   |

        And the relations
            | type  | route   | way:route |
            | route | testbot | as,stb    |
            | route | testbot | bu,uvc    |

        When I route I should get
            | from | to | route  | distance | time    |
            | b    | c  | bc     | 300m +-1 | 30s +-1 |
            | c    | b  | bc     | 300m +-1 | 30s +-1 |
            | a    | b  | as,stb | 500m +-1 | 50s +-1 |
            | b    | a  | stb,as | 500m +-1 | 50s +-1 |
