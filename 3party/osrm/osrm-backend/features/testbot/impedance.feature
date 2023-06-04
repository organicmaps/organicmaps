@routing @testbot @impedance @todo
Feature: Setting impedance and speed separately
# These tests assume that the speed is not factored into the impedance by OSRM internally.
# Instead the speed can optionally be factored into the weiht in the lua profile.
# Note: With the default grid size of 100m, the diagonals has a length if 141.42m

    Background:
        Given the profile "testbot"

    Scenario: Use impedance to pick route, even when longer/slower
        Given the node map
            |   | s |   | t |   | u |   | v |   |
            | a |   | b |   | c |   | d |   | e |

        And the ways
            | nodes | impedance |
            | ab    | 1.3       |
            | asb   | 1         |
            | bc    | 1.5       |
            | btc   | 1         |
            | cd    | 0.015     |
            | cud   | 0.010     |
            | de    | 150000    |
            | dve   | 100000    |

        When I route I should get
            | from | to | route | distance |
            | a    | b  | ab    | 200m +-1 |
            | b    | a  | ab    | 200m +-1 |
            | b    | c  | btc   | 282m +-1 |
            | c    | b  | btc   | 282m +-1 |
            | c    | d  | cud   | 282m +-1 |
            | d    | c  | cud   | 282m +-1 |
            | d    | e  | dve   | 282m +-1 |
            | e    | d  | dve   | 282m +-1 |

    Scenario: Weight should default to 1
        Given the node map
            |   | s |   | t |   |
            | a |   | b |   | c |

        And the ways
            | nodes | impedance |
            | ab    | 1.40      |
            | asb   |           |
            | bc    | 1.42      |
            | btc   |           |

        When I route I should get
            | from | to | route |
            | a    | b  | ab    |
            | b    | a  | ab    |
            | b    | c  | btc   |
            | c    | b  | btc   |

    Scenario: Use both impedance and speed (multiplied) when picking route
    # OSRM should not factor speed into impedance internally. However, the profile can choose to do so,
    # and this test expect the testbot profile to do it.
        Given the node map
            |   | s |   | t |   |
            | a |   | b |   | c |

        And the ways
            | nodes | impedance | highway   |
            | ab    | 2.80      | primary   |
            | asb   | 1         | secondary |
            | bc    | 2.84      | primary   |
            | btc   | 1         | secondary |

        When I route I should get
            | from | to | route |
            | a    | b  | ab    |
            | b    | a  | ab    |
            | b    | c  | btc   |
            | c    | b  | btc   |

    Scenario: Weight should influence neither speed nor travel time.
        Given the node map
            | a | b | c |
            | t |   |   |

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | at    |

        When I route I should get
            | from | to | route | distance | time    |
            | a    | b  | ab    | 100m +-1 | 10s +-1 |
            | b    | a  | ab    | 100m +-1 | 10s +-1 |
            | b    | c  | bc    | 100m +-1 | 10s +-1 |
            | c    | b  | bc    | 100m +-1 | 10s +-1 |
            | a    | c  | ab,bc | 200m +-1 | 20s +-1 |
            | c    | a  | bc,ab | 200m +-1 | 20s +-1 |
