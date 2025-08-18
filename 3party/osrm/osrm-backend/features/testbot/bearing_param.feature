@routing @bearing_param @todo @testbot
Feature: Bearing parameter

    Background:
        Given the profile "testbot"
        And a grid size of 10 meters

    Scenario: Testbot - Intial bearing in simple case
        Given the node map
            | a |   |
            | 0 | c |
            | b |   |

        And the ways
            | nodes |
            | ac    |
            | bc    |

        When I route I should get
            | from | to | param:bearing | route | bearing |
            | 0    | c  | 0             | bc    | 45      |
            | 0    | c  | 45            | bc    | 45      |
            | 0    | c  | 85            | bc    | 45      |
            | 0    | c  | 95            | ac    | 135     |
            | 0    | c  | 135           | ac    | 135     |
            | 0    | c  | 180           | ac    | 135     |

    Scenario: Testbot - Initial bearing on split way
        Given the node map
        | d |  |  |  |  | 1 |  |  |  |  | c |
        | a |  |  |  |  | 0 |  |  |  |  | b |

        And the ways
            | nodes | oneway |
            | ab    | yes    |
            | bc    | yes    |
            | cd    | yes    |
            | da    | yes    |

        When I route I should get
            | from | to | param:bearing | route    | bearing |
            | 0    | b  | 10            | ab       | 90      |
            | 0    | b  | 90            | ab       | 90      |
            | 0    | b  | 170           | ab       | 90      |
            | 0    | b  | 190           | cd,da,ab | 270     |
            | 0    | b  | 270           | cd,da,ab | 270     |
            | 0    | b  | 350           | cd,da,ab | 270     |
            | 1    | d  | 10            | cd       | 90      |
            | 1    | d  | 90            | cd       | 90      |
            | 1    | d  | 170           | cd       | 90      |
            | 1    | d  | 190           | ab,bc,cd | 270     |
            | 1    | d  | 270           | ab,bc,cd | 270     |
            | 1    | d  | 350           | ab,bc,cd | 270     |

    Scenario: Testbot - Initial bearing in all direction
        Given the node map
            | h |  |   | a |   |  | b |
            |   |  |   |   |   |  |   |
            |   |  | p | i | j |  |   |
            | g |  | o | 0 | k |  | c |
            |   |  | n | m | l |  |   |
            |   |  |   |   |   |  |   |
            | f |  |   | e |   |  | d |

        And the ways
            | nodes | oneway |
            | ia    | yes    |
            | jb    | yes    |
            | kc    | yes    |
            | ld    | yes    |
            | me    | yes    |
            | nf    | yes    |
            | og    | yes    |
            | ph    | yes    |
            | ab    | yes    |
            | bc    | yes    |
            | cd    | yes    |
            | de    | yes    |
            | ef    | yes    |
            | fg    | yes    |
            | gh    | yes    |
            | ha    | yes    |

        When I route I should get
            | from | to | param:bearing | route                   | bearing |
            | 0    | a  | 0             | ia                      | 0       |
            | 0    | a  | 45            | jb,bc,cd,de,ef,fg,gh,ha | 45      |
            | 0    | a  | 90            | kc,cd,de,ef,fg,gh,ha    | 90      |
            | 0    | a  | 135           | ld,de,ef,fg,gh,ha       | 135     |
            | 0    | a  | 180           | me,de,ef,fg,gh,ha       | 180     |
            | 0    | a  | 225           | nf,ef,fg,gh,ha          | 225     |
            | 0    | a  | 270           | og,gh,ha                | 270     |
            | 0    | a  | 315           | pn,ha                   | 315     |
