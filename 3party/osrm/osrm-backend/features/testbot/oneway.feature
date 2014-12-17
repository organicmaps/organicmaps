@routing @testbot @oneway
Feature: Testbot - oneways

    Background:
        Given the profile "testbot"

    Scenario: Routing on a oneway roundabout
        Given the node map
        |   |   |   |   | v |   |
        | x |   | d | c |   |   |
        |   | e |   |   | b |   |
        |   | f |   |   | a |   |
        |   |   | g | h |   | y |
        |   | z |   |   |   |   |

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
            | vx    | yes    |
            | vy    | yes    |
            | yz    | yes    |
            | xe    | yes    |

        When I route I should get
            | from | to | route                |
            | a    | b  | ab                   |
            | b    | c  | bc                   |
            | c    | d  | cd                   |
            | d    | e  | de                   |
            | e    | f  | ef                   |
            | f    | g  | fg                   |
            | g    | h  | gh                   |
            | h    | a  | ha                   |
            | b    | a  | bc,cd,de,ef,fg,gh,ha |
            | c    | b  | cd,de,ef,fg,gh,ha,ab |
            | d    | c  | de,ef,fg,gh,ha,ab,bc |
            | e    | d  | ef,fg,gh,ha,ab,bc,cd |
            | f    | e  | fg,gh,ha,ab,bc,cd,de |
            | g    | f  | gh,ha,ab,bc,cd,de,ef |
            | h    | g  | ha,ab,bc,cd,de,ef,fg |
            | a    | h  | ab,bc,cd,de,ef,fg,gh |

    Scenario: Testbot - Simple oneway
        Then routability should be
            | highway | foot | oneway | forw | backw |
            | primary | no   | yes    | x    |       |

    Scenario: Simple reverse oneway
        Then routability should be
            | highway | foot | oneway | forw | backw |
            | primary | no   | -1     |      | x     |

    Scenario: Testbot - Around the Block
        Given the node map
            | a | b |
            | d | c |

        And the ways
            | nodes | oneway | foot |
            | ab    | yes    | no   |
            | bc    |        | no   |
            | cd    |        | no   |
            | da    |        | no   |

        When I route I should get
            | from | to | route    |
            | a    | b  | ab       |
            | b    | a  | bc,cd,da |

    Scenario: Testbot - Handle various oneway tag values
        Then routability should be
            | foot | oneway   | forw | backw |
            | no   |          | x    | x     |
            | no   | nonsense | x    | x     |
            | no   | no       | x    | x     |
            | no   | false    | x    | x     |
            | no   | 0        | x    | x     |
            | no   | yes      | x    |       |
            | no   | true     | x    |       |
            | no   | 1        | x    |       |
            | no   | -1       |      | x     |

    Scenario: Testbot - Two consecutive oneways
        Given the node map
            | a | b | c |

        And the ways
            | nodes | oneway |
            | ab    | yes    |
            | bc    | yes    |


        When I route I should get
            | from | to | route |
            | a    | c  | ab,bc |
