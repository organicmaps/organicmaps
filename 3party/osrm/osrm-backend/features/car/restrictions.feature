@routing @car @restrictions
Feature: Car - Turn restrictions
# Handle turn restrictions as defined by http://wiki.openstreetmap.org/wiki/Relation:restriction
# Note that if u-turns are allowed, turn restrictions can lead to suprising, but correct, routes.

    Background: Use car routing
        Given the profile "car"

    @no_turning
    Scenario: Car - No left turn
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

    @no_turning
    Scenario: Car - No straight on
        Given the node map
            | a | b | j | d | e |
            | v |   |   |   | z |
            |   | w | x | y |   |

        And the ways
            | nodes | oneway |
            | ab    | no     |
            | bj    | no     |
            | jd    | no     |
            | de    | no     |
            | av    | yes    |
            | vw    | yes    |
            | wx    | yes    |
            | xy    | yes    |
            | yz    | yes    |
            | ze    | yes    |

        And the relations
            | type        | way:from | way:to | node:via | restriction    |
            | restriction | bj       | jd     | j        | no_straight_on |

        When I route I should get
            | from | to | route             |
            | a    | e  | av,vw,wx,xy,yz,ze |

    @no_turning
    Scenario: Car - No right turn
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
            | type        | way:from | way:to | node:via | restriction   |
            | restriction | sj       | ej     | j        | no_right_turn |

        When I route I should get
            | from | to | route |
            | s    | w  | sj,wj |
            | s    | n  | sj,nj |
            | s    | e  |       |

    @no_turning
    Scenario: Car - No u-turn
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
            | type        | way:from | way:to | node:via | restriction |
            | restriction | sj       | wj     | j        | no_u_turn   |

        When I route I should get
            | from | to | route |
            | s    | w  |       |
            | s    | n  | sj,nj |
            | s    | e  | sj,ej |

    @no_turning
    Scenario: Car - Handle any no_* relation
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
            | type        | way:from | way:to | node:via | restriction      |
            | restriction | sj       | wj     | j        | no_weird_zigzags |

        When I route I should get
            | from | to | route |
            | s    | w  |       |
            | s    | n  | sj,nj |
            | s    | e  | sj,ej |

    @only_turning
    Scenario: Car - Only left turn
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
            | type        | way:from | way:to | node:via | restriction    |
            | restriction | sj       | wj     | j        | only_left_turn |

        When I route I should get
            | from | to | route |
            | s    | w  | sj,wj |
            | s    | n  |       |
            | s    | e  |       |

    @only_turning
    Scenario: Car - Only right turn
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
            | type        | way:from | way:to | node:via | restriction     |
            | restriction | sj       | ej     | j        | only_right_turn |

        When I route I should get
            | from | to | route |
            | s    | w  |       |
            | s    | n  |       |
            | s    | e  | sj,ej |

    @only_turning
    Scenario: Car - Only straight on
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
            | type        | way:from | way:to | node:via | restriction      |
            | restriction | sj       | nj     | j        | only_straight_on |

        When I route I should get
            | from | to | route |
            | s    | w  |       |
            | s    | n  | sj,nj |
            | s    | e  |       |

    @no_turning
    Scenario: Car - Handle any only_* restriction
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
            | type        | way:from | way:to | node:via | restriction        |
            | restriction | sj       | nj     | j        | only_weird_zigzags |

        When I route I should get
            | from | to | route |
            | s    | w  |       |
            | s    | n  | sj,nj |
            | s    | e  |       |

    @except
    Scenario: Car - Except tag and on no_ restrictions
        Given the node map
            | b | x | c |
            | a | j | d |
            |   | s |   |

        And the ways
            | nodes | oneway |
            | sj    | no     |
            | xj    | -1     |
            | aj    | -1     |
            | bj    | no     |
            | cj    | no     |
            | dj    | -1     |

        And the relations
            | type        | way:from | way:to | node:via | restriction   | except   |
            | restriction | sj       | aj     | j        | no_left_turn  | motorcar |
            | restriction | sj       | bj     | j        | no_left_turn  |          |
            | restriction | sj       | cj     | j        | no_right_turn |          |
            | restriction | sj       | dj     | j        | no_right_turn | motorcar |

        When I route I should get
            | from | to | route |
            | s    | a  | sj,aj |
            | s    | b  |       |
            | s    | c  |       |
            | s    | d  | sj,dj |

    @except
    Scenario: Car - Except tag and on only_ restrictions
        Given the node map
            | a |   | b |
            |   | j |   |
            |   | s |   |

        And the ways
            | nodes | oneway |
            | sj    | yes    |
            | aj    | no     |
            | bj    | no     |

        And the relations
            | type        | way:from | way:to | node:via | restriction      | except   |
            | restriction | sj       | aj     | j        | only_straight_on | motorcar |

        When I route I should get
            | from | to | route |
            | s    | a  | sj,aj |
            | s    | b  | sj,bj |

    @except
    Scenario: Car - Several only_ restrictions at the same segment
        Given the node map
            |   |   |   |   | y |   |   |   |   |
            | i | j | f | b | x | a | e | g | h |
            |   |   |   |   |   |   |   |   |   |
            |   |   |   | c |   | d |   |   |   |

        And the ways
            | nodes | oneway |
            | fb    | no     |
            | bx    | -1     |
            | xa    | no     |
            | ae    | no     |
            | cb    | no     |
            | dc    | -1     |
            | da    | no     |
            | fj    | no     |
            | jf    | no     |
            | ge    | no     |
            | hg    | no     |

        And the relations
            | type        | way:from | way:to | node:via | restriction      |
            | restriction | ae       | xa     | a        | only_straight_on |
            | restriction | xb       | fb     | b        | only_straight_on |
            | restriction | cb       | bx     | b        | only_right_turn  |
            | restriction | da       | ae     | a        | only_right_turn  |

        When I route I should get
            | from | to | route                            |
            | e    | f  | ae,xa,bx,fb                      |
            | c    | f  | dc,da,ae,ge,hg,hg,ge,ae,xa,bx,fb |
            | d    | f  | da,ae,ge,hg,hg,ge,ae,xa,bx,fb    |

    @except
    Scenario: Car - two only_ restrictions share same to-way
        Given the node map
            |   |   | e |   |   |   | f |   |   |
            |   |   |   |   | a |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |
            |   |   | c |   | x |   | d |   |   |
            |   |   |   |   | y |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |
            |   |   |   |   | b |   |   |   |   |

        And the ways
            | nodes | oneway |
            | ef    | no     |
            | ce    | no     |
            | fd    | no     |
            | ca    | no     |
            | ad    | no     |
            | ax    | no     |
            | xy    | no     |
            | yb    | no     |
            | cb    | no     |
            | db    | no     |

        And the relations
            | type        | way:from | way:to | node:via | restriction      |
            | restriction | ax       | xy     | x        | only_straight_on |
            | restriction | by       | xy     | y        | only_straight_on |

        When I route I should get
            | from | to | route    |
            | a    | b  | ax,xy,yb |
            | b    | a  | yb,xy,ax |

    @except
    Scenario: Car - two only_ restrictions share same from-way
        Given the node map
            |   |   | e |   |   |   | f |   |   |
            |   |   |   |   | a |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |
            |   |   | c |   | x |   | d |   |   |
            |   |   |   |   | y |   |   |   |   |
            |   |   |   |   |   |   |   |   |   |
            |   |   |   |   | b |   |   |   |   |

        And the ways
            | nodes | oneway |
            | ef    | no     |
            | ce    | no     |
            | fd    | no     |
            | ca    | no     |
            | ad    | no     |
            | ax    | no     |
            | xy    | no     |
            | yb    | no     |
            | cb    | no     |
            | db    | no     |

        And the relations
            | type        | way:from | way:to | node:via | restriction      |
            | restriction | xy       | xa     | x        | only_straight_on |
            | restriction | xy       | yb     | y        | only_straight_on |

        When I route I should get
            | from | to | route    |
            | a    | b  | ax,xy,yb |
            | b    | a  | yb,xy,ax |

