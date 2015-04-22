@locate
Feature: Locate - return nearest node

    Background:
        Given the profile "testbot"

    Scenario: Locate - two ways crossing
        Given the node map
            |   |  | 0 | c | 1 |  |   |
            |   |  |   |   |   |  |   |
            | 7 |  |   | n |   |  | 2 |
            | a |  | k | x | m |  | b |
            | 6 |  |   | l |   |  | 3 |
            |   |  |   |   |   |  |   |
            |   |  | 5 | d | 4 |  |   |

        And the ways
            | nodes |
            | axb   |
            | cxd   |

        When I request locate I should get
            | in | out |
            | 0  | c   |
            | 1  | c   |
            | 2  | b   |
            | 3  | b   |
            | 4  | d   |
            | 5  | d   |
            | 6  | a   |
            | 7  | a   |
            | a  | a   |
            | b  | b   |
            | c  | c   |
            | d  | d   |
            | k  | x   |
            | l  | x   |
            | m  | x   |
            | n  | x   |

    Scenario: Locate - inside a triangle
        Given the node map
            |   |  |   |   |   | c |   |   |   |  |   |
            |   |  |   |   |   | 7 |   |   |   |  |   |
            |   |  |   | y |   |   |   | z |   |  |   |
            |   |  | 5 |   | 0 |   | 1 |   | 8 |  |   |
            | 6 |  |   | 2 |   | 3 |   | 4 |   |  | 9 |
            | a |  |   | x |   | u |   | w |   |  | b |

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | ca    |

        When I request locate I should get
            | in | out |
            | 0  | c   |
            | 1  | c   |
            | 2  | a   |
            | 3  | c   |
            | 4  | b   |
            | 5  | a   |
            | 6  | a   |
            | 7  | c   |
            | 8  | b   |
            | 9  | b   |
            | x  | a   |
            | y  | c   |
            | z  | c   |
            | w  | b   |

    Scenario: Nearest - easy-west way
        Given the node map
            | 3 | 4 |   | 5 | 6 |
            | 2 | a | x | b | 7 |
            | 1 | 0 |   | 9 | 8 |

        And the ways
            | nodes |
            | ab    |

        When I request locate I should get
            | in | out |
            | 0  | a   |
            | 1  | a   |
            | 2  | a   |
            | 3  | a   |
            | 4  | a   |
            | 5  | b   |
            | 6  | b   |
            | 7  | b   |
            | 8  | b   |
            | 9  | b   |

    Scenario: Nearest - north-south way
        Given the node map
            | 1 | 2 | 3 |
            | 0 | a | 4 |
            |   | x |   |
            | 9 | b | 5 |
            | 8 | 7 | 6 |

        And the ways
            | nodes |
            | ab    |

        When I request locate I should get
            | in | out |
            | 0  | a   |
            | 1  | a   |
            | 2  | a   |
            | 3  | a   |
            | 4  | a   |
            | 5  | b   |
            | 6  | b   |
            | 7  | b   |
            | 8  | b   |
            | 9  | b   |

    Scenario: Nearest - diagonal 1
        Given the node map
            | 2 |   | 3 |   |   |   |
            |   | a |   | 4 |   |   |
            | 1 |   | x |   | 5 |   |
            |   | 0 |   | y |   | 6 |
            |   |   | 9 |   | b |   |
            |   |   |   | 8 |   | 7 |

        And the ways
            | nodes |
            | axyb  |

        When I request locate I should get
            | in | out |
            | 0  | x   |
            | 1  | a   |
            | 2  | a   |
            | 3  | a   |
            | 4  | x   |
            | 5  | y   |
            | 6  | b   |
            | 7  | b   |
            | 8  | b   |
            | 9  | y   |
            | a  | a   |
            | b  | b   |
            | x  | x   |
            | y  | y   |

    Scenario: Nearest - diagonal 2
        Given the node map
            |   |   |   | 6 |   | 7 |
            |   |   | 5 |   | b |   |
            |   | 4 |   | y |   | 8 |
            | 3 |   | x |   | 9 |   |
            |   | a |   | 0 |   |   |
            | 2 |   | 1 |   |   |   |

        And the ways
        | nodes |
        | ab    |

        When I request nearest I should get
            | in | out |
            | 0  | x   |
            | 1  | a   |
            | 2  | a   |
            | 3  | a   |
            | 4  | x   |
            | 5  | y   |
            | 6  | b   |
            | 7  | b   |
            | 8  | b   |
            | 9  | y   |
            | a  | a   |
            | b  | b   |
            | x  | x   |
            | y  | y   |

        Scenario: Locate - High lat/lon
           Given the node locations
            | node | lat | lon  |
            | a    | -85 | -180 |
            | b    | 0   | 0    |
            | c    | 85  | 180  |
            | x    | -84 | -180 |
            | y    | 84  | 180  |

           And the ways
            | nodes |
            | abc   |

           When I request locate I should get
            | in | out |
            | x  | a   |
            | y  | c   |
