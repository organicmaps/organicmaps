@routing @bearing @testbot
Feature: Compass bearing

    Background:
        Given the profile "testbot"

    Scenario: Bearing when going northwest
        Given the node map
            | b |   |
            |   | a |

        And the ways
            | nodes |
            | ab    |

        When I route I should get
            | from | to | route | compass | bearing |
            | a    | b  | ab    | NW      | 315     |

    Scenario: Bearing when going west
        Given the node map
            | b | a |

        And the ways
            | nodes |
            | ab    |

        When I route I should get
            | from | to | route | compass | bearing |
            | a    | b  | ab    | W       | 270     |

    Scenario: Bearing af 45 degree intervals
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

    Scenario: Bearing in a roundabout
        Given the node map
            |   | d | c |   |
            | e |   |   | b |
            | f |   |   | a |
            |   | g | h |   |

        And the ways
            | nodes | oneway |
            | ab    | yes    |
            | bc    | yes    |
            | cd    | yes    |
            | de    | yes    |
            | ef    | yes    |
            | fg    | yes    |
            | gh    | yes    |
            | ha    | yes    |

        When I route I should get
            | from | to | route                | compass          | bearing                 |
            | c    | b  | cd,de,ef,fg,gh,ha,ab | W,SW,S,SE,E,NE,N | 270,225,180,135,90,45,0 |
            | g    | f  | gh,ha,ab,bc,cd,de,ef | E,NE,N,NW,W,SW,S | 90,45,0,315,270,225,180 |

    Scenario: Bearing should stay constant when zig-zagging
        Given the node map
            | b | d | f | h |
            | a | c | e | g |

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | cd    |
            | de    |
            | ef    |
            | fg    |
            | gh    |

        When I route I should get
            | from | to | route                | compass          | bearing             |
            | a    | h  | ab,bc,cd,de,ef,fg,gh | N,SE,N,SE,N,SE,N | 0,135,0,135,0,135,0 |

    Scenario: Bearings on an east-west way.
        Given the node map
            | a | b | c | d | e | f |

        And the ways
            | nodes  |
            | abcdef |

        When I route I should get
            | from | to | route  | compass | bearing |
            | a    | b  | abcdef | E       | 90      |
            | a    | c  | abcdef | E       | 90      |
            | a    | d  | abcdef | E       | 90      |
            | a    | e  | abcdef | E       | 90      |
            | a    | f  | abcdef | E       | 90      |
            | b    | a  | abcdef | W       | 270     |
            | b    | c  | abcdef | E       | 90      |
            | b    | d  | abcdef | E       | 90      |
            | b    | e  | abcdef | E       | 90      |
            | b    | f  | abcdef | E       | 90      |
            | c    | a  | abcdef | W       | 270     |
            | c    | b  | abcdef | W       | 270     |
            | c    | d  | abcdef | E       | 90      |
            | c    | e  | abcdef | E       | 90      |
            | c    | f  | abcdef | E       | 90      |
            | d    | a  | abcdef | W       | 270     |
            | d    | b  | abcdef | W       | 270     |
            | d    | c  | abcdef | W       | 270     |
            | d    | e  | abcdef | E       | 90      |
            | d    | f  | abcdef | E       | 90      |
            | e    | a  | abcdef | W       | 270     |
            | e    | b  | abcdef | W       | 270     |
            | e    | c  | abcdef | W       | 270     |
            | e    | d  | abcdef | W       | 270     |
            | e    | f  | abcdef | E       | 90      |
            | f    | a  | abcdef | W       | 270     |
            | f    | b  | abcdef | W       | 270     |
            | f    | c  | abcdef | W       | 270     |
            | f    | d  | abcdef | W       | 270     |
            | f    | e  | abcdef | W       | 270     |

    Scenario: Bearings at high latitudes
    # The coordinas below was calculated using http://www.movable-type.co.uk/scripts/latlong.html,
    # to form square with sides of 1 km.

        Given the node locations
            | node | lat       | lon      |
            | a    | 80        | 0        |
            | b    | 80.006389 | 0        |
            | c    | 80.006389 | 0.036667 |
            | d    | 80        | 0.036667 |

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | cd    |
            | da    |
            | ac    |
            | bd    |

        When I route I should get
            | from | to | route | compass | bearing |
            | a    | b  | ab    | N       | 0       |
            | b    | c  | bc    | E       | 90      |
            | c    | d  | cd    | S       | 180     |
            | d    | a  | da    | W       | 270     |
            | b    | a  | ab    | S       | 180     |
            | c    | b  | bc    | W       | 270     |
            | d    | c  | cd    | N       | 0       |
            | a    | d  | da    | E       | 90      |
            | a    | c  | ac    | NE      | 45      |
            | c    | a  | ac    | SW      | 225     |
            | b    | d  | bd    | SE      | 135     |
            | d    | b  | bd    | NW      | 315     |

    Scenario: Bearings at high negative latitudes
    # The coordinas below was calculated using http://www.movable-type.co.uk/scripts/latlong.html,
    # to form square with sides of 1 km.

        Given the node locations
            | node | lat        | lon      |
            | a    | -80        | 0        |
            | b    | -80.006389 | 0        |
            | c    | -80.006389 | 0.036667 |
            | d    | -80        | 0.036667 |

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | cd    |
            | da    |
            | ac    |
            | bd    |

        When I route I should get
            | from | to | route | compass | bearing |
            | a    | b  | ab    | S       | 180     |
            | b    | c  | bc    | E       | 90      |
            | c    | d  | cd    | N       | 0       |
            | d    | a  | da    | W       | 270     |
            | b    | a  | ab    | N       | 0       |
            | c    | b  | bc    | W       | 270     |
            | d    | c  | cd    | S       | 180     |
            | a    | d  | da    | E       | 90      |
            | a    | c  | ac    | SE      | 135     |
            | c    | a  | ac    | NW      | 315     |
            | b    | d  | bd    | NE      | 45      |
            | d    | b  | bd    | SW      | 225     |
