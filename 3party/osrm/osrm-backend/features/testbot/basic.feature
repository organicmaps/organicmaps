@routing @basic @testbot
Feature: Basic Routing

    Background:
        Given the profile "testbot"

    @smallest
    Scenario: A single way with two nodes
        Given the node map
            | a | b |

        And the ways
            | nodes |
            | ab    |

        When I route I should get
            | from | to | route |
            | a    | b  | ab    |
            | b    | a  | ab    |

    Scenario: Routing in between two nodes of way
        Given the node map
            | a | b | 1 | 2 | c | d |

        And the ways
            | nodes |
            | abcd  |

        When I route I should get
            | from | to | route |
            | 1    | 2  | abcd  |
            | 2    | 1  | abcd  |

    Scenario: Routing between the middle nodes of way
        Given the node map
            | a | b | c | d | e | f |

        And the ways
            | nodes  |
            | abcdef |

        When I route I should get
            | from | to | route  |
            | b    | c  | abcdef |
            | b    | d  | abcdef |
            | b    | e  | abcdef |
            | c    | b  | abcdef |
            | c    | d  | abcdef |
            | c    | e  | abcdef |
            | d    | b  | abcdef |
            | d    | c  | abcdef |
            | d    | e  | abcdef |
            | e    | b  | abcdef |
            | e    | c  | abcdef |
            | e    | d  | abcdef |

    Scenario: Two ways connected in a straight line
        Given the node map
            | a | b | c |

        And the ways
            | nodes |
            | ab    |
            | bc    |

        When I route I should get
            | from | to | route |
            | a    | c  | ab,bc |
            | c    | a  | bc,ab |
            | a    | b  | ab    |
            | b    | a  | ab    |
            | b    | c  | bc    |
            | c    | b  | bc    |

    Scenario: 2 unconnected parallel ways
        Given the node map
            | a | b | c |
            | d | e | f |

        And the ways
            | nodes |
            | abc   |
            | def   |

        When I route I should get
            | from | to | route |
            | a    | b  | abc   |
            | b    | a  | abc   |
            | b    | c  | abc   |
            | c    | b  | abc   |
            | d    | e  | def   |
            | e    | d  | def   |
            | e    | f  | def   |
            | f    | e  | def   |
            | a    | d  |       |
            | d    | a  |       |
            | b    | d  |       |
            | d    | b  |       |
            | c    | d  |       |
            | d    | c  |       |
            | a    | e  |       |
            | e    | a  |       |
            | b    | e  |       |
            | e    | b  |       |
            | c    | e  |       |
            | e    | c  |       |
            | a    | f  |       |
            | f    | a  |       |
            | b    | f  |       |
            | f    | b  |       |
            | c    | f  |       |
            | f    | c  |       |

    Scenario: 3 ways connected in a triangle
        Given the node map
            | a |   | b |
            |   |   |   |
            |   | c |   |

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | ca    |

        When I route I should get
            | from | to | route |
            | a    | b  | ab    |
            | a    | c  | ca    |
            | b    | c  | bc    |
            | b    | a  | ab    |
            | c    | a  | ca    |
            | c    | b  | bc    |

    Scenario: 3 connected triangles
        Given a grid size of 100 meters
        Given the node map
            | x | a |   | b | s |
            | y |   |   |   | t |
            |   |   | c |   |   |
            |   | v |   | w |   |

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | ca    |
            | ax    |
            | xy    |
            | ya    |
            | bs    |
            | st    |
            | tb    |
            | cv    |
            | vw    |
            | wc    |

        When I route I should get
            | from | to | route |
            | a    | b  | ab    |
            | a    | c  | ca    |
            | b    | c  | bc    |
            | b    | a  | ab    |
            | c    | a  | ca    |
            | c    | b  | bc    |

    Scenario: To ways connected at a 45 degree angle
        Given the node map
            | a |   |   |
            | b |   |   |
            | c | d | e |

        And the ways
            | nodes |
            | abc   |
            | cde   |

        When I route I should get
            | from | to | route   |
            | b    | d  | abc,cde |
            | a    | e  | abc,cde |
            | a    | c  | abc     |
            | c    | a  | abc     |
            | c    | e  | cde     |
            | e    | c  | cde     |

    Scenario: Grid city center
        Given the node map
            | a | b | c | d |
            | e | f | g | h |
            | i | j | k | l |
            | m | n | o | p |

        And the ways
            | nodes |
            | abcd  |
            | efgh  |
            | ijkl  |
            | mnop  |
            | aeim  |
            | bfjn  |
            | cgko  |
            | dhlp  |

        When I route I should get
            | from | to | route |
            | f    | g  | efgh  |
            | g    | f  | efgh  |
            | f    | j  | bfjn  |
            | j    | f  | bfjn  |

    Scenario: Grid city periphery
        Given the node map
            | a | b | c | d |
            | e | f | g | h |
            | i | j | k | l |
            | m | n | o | p |

        And the ways
            | nodes |
            | abcd  |
            | efgh  |
            | ijkl  |
            | mnop  |
            | aeim  |
            | bfjn  |
            | cgko  |
            | dhlp  |

        When I route I should get
            | from | to | route |
            | a    | d  | abcd  |
            | d    | a  | abcd  |
            | a    | m  | aeim  |
            | m    | a  | aeim  |
    
    Scenario: Testbot - Triangle challenge
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
