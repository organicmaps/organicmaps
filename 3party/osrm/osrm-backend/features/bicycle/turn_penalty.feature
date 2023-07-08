@routing @bicycle @turn_penalty
Feature: Turn Penalties

    Background:
        Given the profile "turnbot"

    Scenario: Bike - turns should incur a delay that depend on the angle

        Given the node map
            | c | d | e |
            | b | j | f |
            | a | s | g |

        And the ways
            | nodes |
            | sj    |
            | ja    |
            | jb    |
            | jc    |
            | jd    |
            | je    |
            | jf    |
            | jg    |

        When I route I should get
            | from | to | route | time    | distance |
            | s    | a  | sj,ja | 39s +-1 | 242m +-1 |
            | s    | b  | sj,jb | 30s +-1 | 200m +-1 |
            | s    | c  | sj,jc | 29s +-1 | 242m +-1 |
            | s    | d  | sj,jd | 20s +-1 | 200m +-1 |
            | s    | e  | sj,je | 29s +-1 | 242m +-1 |
            | s    | f  | sj,jf | 30s +-1 | 200m +-1 |
            | s    | g  | sj,jg | 39s +-1 | 242m +-1 |