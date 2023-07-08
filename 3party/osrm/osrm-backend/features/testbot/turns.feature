@routing @turns @testbot
Feature: Turn directions/codes

    Background:
        Given the profile "testbot"

    Scenario: Turn directions
        Given the node map
            | o | p | a | b | c |
            | n |   |   |   | d |
            | m |   | x |   | e |
            | l |   |   |   | f |
            | k | j | i | h | g |

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
            | xi    |
            | xj    |
            | xk    |
            | xl    |
            | xm    |
            | xn    |
            | xo    |
            | xp    |

        When I route I should get
            | from | to | route | turns                         |
            | i    | k  | xi,xk | head,sharp_left,destination   |
            | i    | m  | xi,xm | head,left,destination         |
            | i    | o  | xi,xo | head,slight_left,destination  |
            | i    | a  | xi,xa | head,straight,destination     |
            | i    | c  | xi,xc | head,slight_right,destination |
            | i    | e  | xi,xe | head,right,destination        |
            | i    | g  | xi,xg | head,sharp_right,destination  |

            | k | m | xk,xm | head,sharp_left,destination   |
            | k | o | xk,xo | head,left,destination         |
            | k | a | xk,xa | head,slight_left,destination  |
            | k | c | xk,xc | head,straight,destination     |
            | k | e | xk,xe | head,slight_right,destination |
            | k | g | xk,xg | head,right,destination        |
            | k | i | xk,xi | head,sharp_right,destination  |

            | m | o | xm,xo | head,sharp_left,destination   |
            | m | a | xm,xa | head,left,destination         |
            | m | c | xm,xc | head,slight_left,destination  |
            | m | e | xm,xe | head,straight,destination     |
            | m | g | xm,xg | head,slight_right,destination |
            | m | i | xm,xi | head,right,destination        |
            | m | k | xm,xk | head,sharp_right,destination  |

            | o | a | xo,xa | head,sharp_left,destination   |
            | o | c | xo,xc | head,left,destination         |
            | o | e | xo,xe | head,slight_left,destination  |
            | o | g | xo,xg | head,straight,destination     |
            | o | i | xo,xi | head,slight_right,destination |
            | o | k | xo,xk | head,right,destination        |
            | o | m | xo,xm | head,sharp_right,destination  |

            | a | c | xa,xc | head,sharp_left,destination   |
            | a | e | xa,xe | head,left,destination         |
            | a | g | xa,xg | head,slight_left,destination  |
            | a | i | xa,xi | head,straight,destination     |
            | a | k | xa,xk | head,slight_right,destination |
            | a | m | xa,xm | head,right,destination        |
            | a | o | xa,xo | head,sharp_right,destination  |

            | c | e | xc,xe | head,sharp_left,destination   |
            | c | g | xc,xg | head,left,destination         |
            | c | i | xc,xi | head,slight_left,destination  |
            | c | k | xc,xk | head,straight,destination     |
            | c | m | xc,xm | head,slight_right,destination |
            | c | o | xc,xo | head,right,destination        |
            | c | a | xc,xa | head,sharp_right,destination  |

            | e | g | xe,xg | head,sharp_left,destination   |
            | e | i | xe,xi | head,left,destination         |
            | e | k | xe,xk | head,slight_left,destination  |
            | e | m | xe,xm | head,straight,destination     |
            | e | o | xe,xo | head,slight_right,destination |
            | e | a | xe,xa | head,right,destination        |
            | e | c | xe,xc | head,sharp_right,destination  |

            | g | i | xg,xi | head,sharp_left,destination   |
            | g | k | xg,xk | head,left,destination         |
            | g | m | xg,xm | head,slight_left,destination  |
            | g | o | xg,xo | head,straight,destination     |
            | g | a | xg,xa | head,slight_right,destination |
            | g | c | xg,xc | head,right,destination        |
            | g | e | xg,xe | head,sharp_right,destination  |

    Scenario: Turn instructions at high latitude
    # https://github.com/DennisOSRM/Project-OSRM/issues/532
        Given the node locations
            | node | lat       | lon      |
            | a    | 55.68740  | 12.52430 |
            | b    | 55.68745  | 12.52409 |
            | c    | 55.68711  | 12.52383 |
            | x    | -55.68740 | 12.52430 |
            | y    | -55.68745 | 12.52409 |
            | z    | -55.68711 | 12.52383 |

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | xy    |
            | yz    |

        When I route I should get
            | from | to | route | turns                  |
            | a    | c  | ab,bc | head,left,destination  |
            | c    | a  | bc,ab | head,right,destination |
            | x    | z  | xy,yz | head,right,destination |
            | z    | x  | yz,xy | head,left,destination  |
