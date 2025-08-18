@routing @snap @testbot
Feature: Snap start/end point to the nearest way

    Background:
        Given the profile "testbot"

    Scenario: Snap to nearest protruding oneway
        Given the node map
            |   | 1 |   | 2 |   |
            | 8 |   | n |   | 3 |
            |   | w | c | e |   |
            | 7 |   | s |   | 4 |
            |   | 6 |   | 5 |   |

        And the ways
            | nodes |
            | nc    |
            | ec    |
            | sc    |
            | wc    |

        When I route I should get
            | from | to | route |
            | 1    | c  | nc    |
            | 2    | c  | nc    |
            | 3    | c  | ec    |
            | 4    | c  | ec    |
            | 5    | c  | sc    |
            | 6    | c  | sc    |
            | 7    | c  | wc    |
            | 8    | c  | wc    |

    Scenario: Snap to nearest edge of a square
        Given the node map
            | 4 | 5 | 6 | 7 |
            | 3 | a |   | u |
            | 2 |   |   |   |
            | 1 | d |   | b |

        And the ways
            | nodes |
            | aub   |
            | adb   |

        When I route I should get
            | from | to | route |
            | 1    | b  | adb   |
            | 2    | b  | adb   |
            | 6    | b  | aub   |
            | 7    | b  | aub   |

    Scenario: Snap to edge right under start/end point
        Given the node map
            | d | e | f | g |
            | c |   |   | h |
            | b |   |   | i |
            | a | l | k | j |

        And the ways
            | nodes |
            | abcd  |
            | defg  |
            | ghij  |
            | jkla  |

        When I route I should get
            | from | to | route     |
            | a    | b  | abcd      |
            | a    | c  | abcd      |
            | a    | d  | abcd      |
            | a    | e  | abcd,defg |
            | a    | f  | abcd,defg |
            | a    | h  | jkla,ghij |
            | a    | i  | jkla,ghij |
            | a    | j  | jkla      |
            | a    | k  | jkla      |
            | a    | l  | jkla      |

    Scenario: Snap to correct way at large scales
        Given a grid size of 1000 meters
        Given the node map
            |   |  |  | a |
            | x |  |  | b |
            |   |  |  | c |

        And the ways
            | nodes |
            | xa    |
            | xb    |
            | xc    |

        When I route I should get
            | from | to | route |
            | x    | a  | xa    |
            | x    | b  | xb    |
            | x    | c  | xc    |
            | a    | x  | xa    |
            | b    | x  | xb    |
            | c    | x  | xc    |

    Scenario: Find edges within 1km, and the same from 10km
        Given a grid size of 1000 meters
        Given the node map
            | p |  |  |  |  |  |  |  |  |   |   | i |   |   |  |  |  |  |  |  |  |  | j |
            |   |  |  |  |  |  |  |  |  |   |   |   |   |   |  |  |  |  |  |  |  |  |   |
            |   |  |  |  |  |  |  |  |  |   |   |   |   |   |  |  |  |  |  |  |  |  |   |
            |   |  |  |  |  |  |  |  |  |   |   |   |   |   |  |  |  |  |  |  |  |  |   |
            |   |  |  |  |  |  |  |  |  |   |   |   |   |   |  |  |  |  |  |  |  |  |   |
            |   |  |  |  |  |  |  |  |  |   |   |   |   |   |  |  |  |  |  |  |  |  |   |
            |   |  |  |  |  |  |  |  |  |   |   |   |   |   |  |  |  |  |  |  |  |  |   |
            |   |  |  |  |  |  |  |  |  | 8 |   | 1 |   | 2 |  |  |  |  |  |  |  |  |   |
            |   |  |  |  |  |  |  |  |  |   | h | a | b |   |  |  |  |  |  |  |  |  |   |
            | o |  |  |  |  |  |  |  |  | 7 | g | x | c | 3 |  |  |  |  |  |  |  |  | k |
            |   |  |  |  |  |  |  |  |  |   | f | e | d |   |  |  |  |  |  |  |  |  |   |
            |   |  |  |  |  |  |  |  |  | 6 |   | 5 |   | 4 |  |  |  |  |  |  |  |  |   |
            |   |  |  |  |  |  |  |  |  |   |   |   |   |   |  |  |  |  |  |  |  |  |   |
            |   |  |  |  |  |  |  |  |  |   |   |   |   |   |  |  |  |  |  |  |  |  |   |
            |   |  |  |  |  |  |  |  |  |   |   |   |   |   |  |  |  |  |  |  |  |  |   |
            |   |  |  |  |  |  |  |  |  |   |   |   |   |   |  |  |  |  |  |  |  |  |   |
            |   |  |  |  |  |  |  |  |  |   |   |   |   |   |  |  |  |  |  |  |  |  |   |
            |   |  |  |  |  |  |  |  |  |   |   |   |   |   |  |  |  |  |  |  |  |  |   |
            |   |  |  |  |  |  |  |  |  |   |   |   |   |   |  |  |  |  |  |  |  |  |   |
            |   |  |  |  |  |  |  |  |  |   |   |   |   |   |  |  |  |  |  |  |  |  |   |
            | n |  |  |  |  |  |  |  |  |   |   | m |   |   |  |  |  |  |  |  |  |  | l |

        Given the ways
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
            | from | to | route |
            | x    | 1  | xa    |
            | x    | 2  | xb    |
            | x    | 3  | xc    |
            | x    | 4  | xd    |
            | x    | 5  | xe    |
            | x    | 6  | xf    |
            | x    | 7  | xg    |
            | x    | 8  | xh    |
            | x    | i  | xa    |
            | x    | j  | xb    |
            | x    | k  | xc    |
            | x    | l  | xd    |
            | x    | m  | xe    |
            | x    | n  | xf    |
            | x    | o  | xg    |
            | x    | p  | xh    |
