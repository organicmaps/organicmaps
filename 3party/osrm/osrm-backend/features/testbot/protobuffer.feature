@routing @pbf @testbot
Feature: Importing protobuffer (.pbf) format
# Test normally read .osm, which is faster than .pbf files,
# since we don't need to use osmosis to first convert to .pbf
# The scenarios in this file test the ability to import .pbf files,
# including nodes, way, restictions, and a various special situations.

    Background:
        Given the profile "testbot"
        And the import format "pbf"

    Scenario: Testbot - Protobuffer import, nodes and ways
        Given the node map
            |   |   |   | d |
            | a | b | c |   |
            |   |   |   | e |

        And the ways
            | nodes | highway | oneway |
            | abc   | primary |        |
            | cd    | primary | yes    |
            | ce    | river   |        |
            | de    | primary |        |

        When I route I should get
            | from | to | route |
            | d    | c  | de,ce |
            | e    | d  | de    |


    Scenario: Testbot - Protobuffer import, turn restiction relations
        Given the node map
            |   | n |   |
            | w | j | e |
            |   | s |   |

        And the ways
            | nodes | oneway |
            | sj    | yes    |
            | nj    | -1     |
            | wj    | -1     |
            | ej    | -1     |

        And the relations
            | type        | way:from | way:to | node:via | restriction  |
            | restriction | sj       | wj     | j        | no_left_turn |

        When I route I should get
            | from | to | route |
            | s    | w  |       |
            | s    | n  | sj,nj |
            | s    | e  | sj,ej |


    Scenario: Testbot - Protobuffer import, distances at longitude 45
        Given the node locations
            | node | lat | lon |
            | a    | 80  | 45  |
            | b    | 0   | 45  |

        And the ways
            | nodes |
            | ab    |

        When I route I should get
            | from | to | route | distance       |
            | a    | b  | ab    | 8905559m ~0.1% |

    Scenario: Testbot - Protobuffer import, distances at longitude 80
        Given the node locations
            | node | lat | lon |
            | a    | 80  | 80  |
            | b    | 0   | 80  |

        And the ways
            | nodes |
            | ab    |

        When I route I should get
            | from | to | route | distance       |
            | a    | b  | ab    | 8905559m ~0.1% |

    Scenario: Testbot - Protobuffer import, empty dataset
        Given the node map
            |  |

        Given the ways
            | nodes |

        When the data has been prepared
        Then "osrm-extract" should return code 1


    Scenario: Testbot - Protobuffer import, streetnames with UTF characters
        Given the node map
            | a | b | c | d |

        And the ways
            | nodes | name                   |
            | ab    | Scandinavian København |
            | bc    | Japanese 東京            |
            | cd    | Cyrillic Москва        |

        When I route I should get
            | from | to | route                  |
            | a    | b  | Scandinavian København |
            | b    | c  | Japanese 東京            |
            | c    | d  | Cyrillic Москва        |

    Scenario: Testbot - Protobuffer import, bearing af 45 degree intervals
        Given the node map
            | b | a | h |
            | c | x | g |
            | d | e | f |

        And the ways
            | nodes |
            | xa    |
            | xb    |
            | xc    |
            | xd    |
            | xe    |
            | xf    |
            | xg    |
            | xh    |

        When I route I should get
            | from | to | route | compass | bearing |
            | x    | a  | xa    | N       | 0       |
            | x    | b  | xb    | NW      | 315     |
            | x    | c  | xc    | W       | 270     |
            | x    | d  | xd    | SW      | 225     |
            | x    | e  | xe    | S       | 180     |
            | x    | f  | xf    | SE      | 135     |
            | x    | g  | xg    | E       | 90      |
            | x    | h  | xh    | NE      | 45      |


    Scenario: Testbot - Protobuffer import, rraffic signals should incur a delay
        Given the node map
            | a | b | c |
            | d | e | f |

        And the nodes
            | node | highway         |
            | e    | traffic_signals |

        And the ways
            | nodes |
            | abc   |
            | def   |

        When I route I should get
            | from | to | route | time    | distance |
            | a    | c  | abc   | 20s +-1 | 200m +-1 |
            | d    | f  | def   | 27s +-1 | 200m +-1 |
