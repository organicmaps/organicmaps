@routing @testbot @mode
Feature: Testbot - Travel mode

# testbot modes:
# 1 normal
# 2 route
# 3 river downstream
# 4 river upstream
# 5 steps down
# 6 steps up

    Background:
       Given the profile "testbot"
        
    Scenario: Testbot - Modes in each direction, different forward/backward speeds
        Given the node map
            |   | 0 | 1 |   |
            | a |   |   | b |

        And the ways
            | nodes | highway | oneway |
            | ab    | river   |        |

        When I route I should get
            | from | to | route | modes |
            | a    | 0  | ab    | 3     |
            | a    | b  | ab    | 3     |
            | 0    | 1  | ab    | 3     |
            | 0    | b  | ab    | 3     |
            | b    | 1  | ab    | 4     |
            | b    | a  | ab    | 4     |
            | 1    | 0  | ab    | 4     |
            | 1    | a  | ab    | 4     |

    Scenario: Testbot - Modes in each direction, same forward/backward speeds
        Given the node map
            |   | 0 | 1 |   |
            | a |   |   | b |

        And the ways
            | nodes | highway |
            | ab    | steps   |

        When I route I should get
            | from | to | route | modes | time    |
            | 0    | 1  | ab    | 5     | 60s +-1 |
            | 1    | 0  | ab    | 6     | 60s +-1 |

    @oneway
    Scenario: Testbot - Modes for oneway, different forward/backward speeds
        Given the node map
            | a | b |

        And the ways
            | nodes | highway | oneway |
            | ab    | river   | yes    |

        When I route I should get
            | from | to | route | modes |
            | a    | b  | ab    | 3     |
            | b    | a  |       |       |

    @oneway
    Scenario: Testbot - Modes for oneway, same forward/backward speeds
        Given the node map
            | a | b |

        And the ways
            | nodes | highway | oneway |
            | ab    | steps   | yes    |

        When I route I should get
            | from | to | route | modes |
            | a    | b  | ab    | 5     |
            | b    | a  |       |       |

    @oneway
    Scenario: Testbot - Modes for reverse oneway, different forward/backward speeds
        Given the node map
            | a | b |

        And the ways
            | nodes | highway | oneway |
            | ab    | river   | -1     |

        When I route I should get
            | from | to | route | modes |
            | a    | b  |       |       |
            | b    | a  | ab    | 4     |

    @oneway
    Scenario: Testbot - Modes for reverse oneway, same forward/backward speeds
        Given the node map
            | a | b |

        And the ways
            | nodes | highway | oneway |
            | ab    | steps   | -1     |

        When I route I should get
            | from | to | route | modes |
            | a    | b  |       |       |
            | b    | a  | ab    | 6     |

    @via
    Scenario: Testbot - Mode should be set at via points
        Given the node map
            | a | 1 | b |

        And the ways
            | nodes | highway |
            | ab    | river   |

        When I route I should get
            | waypoints | route | modes | turns                |
            | a,1,b     | ab,ab | 3,3   | head,via,destination |
            | b,1,a     | ab,ab | 4,4   | head,via,destination |

    Scenario: Testbot - Starting at a tricky node
       Given the node map
            |  | a |  |   |   |
            |  |   |  | b | c |

       And the ways
            | nodes | highway |
            | ab    | river   |
            | bc    | primary |

       When I route I should get
            | from | to | route | modes |
            | b    | a  | ab    | 4     |

    Scenario: Testbot - Mode changes on straight way without name change
       Given the node map
            | a | 1 | b | 2 | c |

       And the ways
            | nodes | highway | name   |
            | ab    | primary | Avenue |
            | bc    | river   | Avenue |

       When I route I should get
            | from | to | route         | modes | turns                     |
            | a    | c  | Avenue,Avenue | 1,3   | head,straight,destination |
            | c    | a  | Avenue,Avenue | 4,1   | head,straight,destination |
            | 1    | 2  | Avenue,Avenue | 1,3   | head,straight,destination |
            | 2    | 1  | Avenue,Avenue | 4,1   | head,straight,destination |

    Scenario: Testbot - Mode for routes
       Given the node map
            | a | b |   |   |   |
            |   | c | d | e | f |

       And the ways
            | nodes | highway | route | duration |
            | ab    | primary |       |          |
            | bc    |         | ferry | 0:01     |
            | cd    | primary |       |          |
            | de    | primary |       |          |
            | ef    | primary |       |          |

       When I route I should get
            | from | to | route          | turns                                         | modes     |
            | a    | d  | ab,bc,cd       | head,right,left,destination                   | 1,2,1     |
            | d    | a  | cd,bc,ab       | head,right,left,destination                   | 1,2,1     |
            | c    | a  | bc,ab          | head,left,destination                         | 2,1       |
            | d    | b  | cd,bc          | head,right,destination                        | 1,2       |
            | a    | c  | ab,bc          | head,right,destination                        | 1,2       |
            | b    | d  | bc,cd          | head,left,destination                         | 2,1       |
            | a    | f  | ab,bc,cd,de,ef | head,right,left,straight,straight,destination | 1,2,1,1,1 |

    Scenario: Testbot - Modes, triangle map
        Given the node map
            |   |   |   |   |   |   | d |
            |   |   |   |   |   | 2 |   |
            |   |   |   |   | 6 |   | 5 |
            | a | 0 | b | c |   |   |   |
            |   |   |   |   | 4 |   | 1 |
            |   |   |   |   |   | 3 |   |
            |   |   |   |   |   |   | e |

       And the ways
            | nodes | highway | oneway |
            | abc   | primary |        |
            | cd    | primary | yes    |
            | ce    | river   |        |
            | de    | primary |        |

       When I route I should get
            | from | to | route        | modes   |
            | 0    | 1  | abc,ce,de    | 1,3,1   |
            | 1    | 0  | de,ce,abc    | 1,4,1   |
            | 0    | 2  | abc,cd       | 1,1     |
            | 2    | 0  | cd,de,ce,abc | 1,1,4,1 |
            | 0    | 3  | abc,ce       | 1,3     |
            | 3    | 0  | ce,abc       | 4,1     |
            | 4    | 3  | ce           | 3       |
            | 3    | 4  | ce           | 4       |
            | 3    | 1  | ce,de        | 3,1     |
            | 1    | 3  | de,ce        | 1,4     |
            | a    | e  | abc,ce       | 1,3     |
            | e    | a  | ce,abc       | 4,1     |
            | a    | d  | abc,cd       | 1,1     |
            | d    | a  | de,ce,abc    | 1,4,1   |

    Scenario: Testbot - River in the middle
        Given the node map
            | a | b | c |   |   |
            |   |   | d |   |   |
            |   |   | e | f | g |

        And the ways
            | nodes | highway |
            | abc   | primary |
            | cde   | river   |
            | efg   | primary |

        When I route I should get
            | from | to | route       | modes |
            | a    | g  | abc,cde,efg | 1,3,1 |
            | b    | f  | abc,cde,efg | 1,3,1 |
            | e    | c  | cde         | 4     |
            | e    | b  | cde,abc     | 4,1   |
            | e    | a  | cde,abc     | 4,1   |
            | c    | e  | cde         | 3     |
            | c    | f  | cde,efg     | 3,1   |
            | c    | g  | cde,efg     | 3,1   |
