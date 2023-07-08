@routing @bad @testbot
Feature: Handle bad data in a graceful manner

    Background:
        Given the profile "testbot"

    Scenario: Empty dataset
        Given the node map
            |  |

        Given the ways
            | nodes |

        When the data has been prepared
        Then "osrm-extract" should return code 1

    Scenario: Only dead-end oneways
        Given the node map
            | a | b | c | d | e |

        Given the ways
            | nodes | oneway |
            | abcde | yes    |

        When I route I should get
            | from | to | route |
            | b    | d  | abcde |

    @todo
    Scenario: Start/end point at the same location
        Given the node map
            | a | b |
            | 1 | 2 |

        Given the ways
            | nodes |
            | ab    |

        When I route I should get
            | from | to | route |
            | a    | a  |       |
            | b    | b  |       |
            | 1    | 1  |       |
            | 2    | 2  |       |

    @poles
    Scenario: Routing close to the north/south pole
    # Mercator is undefined close to the poles.
    # All nodes and request with latitude to close to either of the poles should therefore be ignored.

        Given the node locations
            | node | lat | lon |
            | a    | 89  | 0   |
            | b    | 87  | 0   |
            | c    | 82  | 0   |
            | d    | 80  | 0   |
            | e    | 78  | 0   |
            | k    | -78 | 0   |
            | l    | -80 | 0   |
            | m    | -82 | 0   |
        # | n    | -87 | 0   |
        # | o    | -89 | 0   |

        And the ways
            | nodes |
        # | ab    |
            | bc    |
            | cd    |
            | de    |
            | kl    |
            | lm    |
        # | mn    |
        # | no    |

        When I route I should get
            | from | to | route |
        # | a    | b  | cd    |
        # | b    | c  | cd    |
        # | a    | d  | cd    |
        # | c    | d  | cd    |
            | d    | e  | de    |
        # | k    | l  | kl    |
        # | l    | m  | lm    |
        # | o    | l  | lm    |
        # | n    | m  | lm    |
        # | o    | n  | lm    |
