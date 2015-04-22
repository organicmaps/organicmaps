@routing @testbot @via
Feature: Via points

    Background:
        Given the profile "testbot"

    Scenario: Simple via point
        Given the node map
            | a | b | c |

        And the ways
            | nodes |
            | abc   |

        When I route I should get
            | waypoints | route   |
            | a,b,c     | abc,abc |
            | c,b,a     | abc,abc |

    Scenario: Via point at a dead end
        Given the node map
            | a | b | c |
            |   | d |   |

        And the ways
            | nodes |
            | abc   |
            | bd    |

        When I route I should get
            | waypoints | route            |
            | a,d,c     | abc,bd,bd,bd,abc |
            | c,d,a     | abc,bd,bd,bd,abc |

    Scenario: Multiple via points
        Given the node map
            | a |   |   |   | e | f | g |   |
            |   | b | c | d |   |   |   | h |

        And the ways
            | nodes |
            | ae    |
            | ab    |
            | bcd   |
            | de    |
            | efg   |
            | gh    |
            | dh    |

        When I route I should get
            | waypoints | route                    |
            | a,c,f     | ab,bcd,bcd,de,efg        |
            | a,c,f,h   | ab,bcd,bcd,de,efg,efg,gh |

    Scenario: Via points on ring of oneways
    # xa it to avoid only having a single ring, which cna trigger edge cases
        Given the node map
            | x |   |   |   |   |   |   |
            | a | 1 | b | 2 | c | 3 | d |
            | f |   |   |   |   |   | e |

        And the ways
            | nodes | oneway |
            | xa    |        |
            | ab    | yes    |
            | bc    | yes    |
            | cd    | yes    |
            | de    | yes    |
            | ef    | yes    |
            | fa    | yes    |

        When I route I should get
            | waypoints | route                      | distance  | turns                                                               |
            | 1,3       | ab,bc,cd                   |  400m +-1 | head,straight,straight,destination                                  |
            | 3,1       | cd,de,ef,fa,ab             | 1000m +-1 | head,right,right,right,right,destination                            |
            | 1,2,3     | ab,bc,bc,cd                |  400m +-1 | head,straight,via,straight,destination                              |
            | 1,3,2     | ab,bc,cd,cd,de,ef,fa,ab,bc | 1600m +-1 | head,straight,straight,via,right,right,right,right,straight,destination |
            | 3,2,1     | cd,de,ef,fa,ab,bc,bc,cd,de,ef,fa,ab | 2400m +-1 | head,right,right,right,right,straight,via,straight,right,right,right,right,destination |

    @bug
    Scenario: Via points on ring on the same oneway
    # xa it to avoid only having a single ring, which cna trigger edge cases
        Given the node map
            | x |   |   |   |   |
            | a | 1 | 2 | 3 | b |
            | d |   |   |   | c |

        And the ways
            | nodes | oneway |
            | xa    |        |
            | ab    | yes    |
            | bc    | yes    |
            | cd    | yes    |
            | da    | yes    |

        When I route I should get
            | waypoints | route                      | distance  | turns                                                            |
            | 1,3       | ab                         | 200m +-1  | head,destination                                                 |
            | 3,1       | ab,bc,cd,da,ab             | 800m +-1  | head,right,right,right,right,destination                         |
            | 1,2,3     | ab                         | 200m +-1  | head,destination                                                 |
            | 1,3,2     | ab,bc,cd,da,ab             | 1100m +-1 | head,right,right,right,right,destination                         |
            | 3,2,1     | ab,bc,cd,da,ab,bc,cd,da,ab | 1600m +-1 | head,right,right,right,right,right,right,right,right,destination |
