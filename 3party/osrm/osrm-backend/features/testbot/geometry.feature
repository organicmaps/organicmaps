@routing @testbot
Feature: Retrieve geometry

    Background: Use some profile
        Given the profile "testbot"


    @geometry
    Scenario: Route retrieving geometry
       Given the node locations
            | node | lat | lon |
            | a    | 1.0 | 1.5 |
            | b    | 2.0 | 2.5 |
            | c    | 3.0 | 3.5 |
            | d    | 4.0 | 4.5 |

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | cd    |

        When I route I should get
            | from | to | route | geometry                             |
            | a    | c  | ab,bc | _c`\|@_upzA_c`\|@_c`\|@_c`\|@_c`\|@  |
            | b    | d  | bc,cd | _gayB_yqwC_c`\|@_c`\|@_c`\|@_c`\|@   |

# Mind the \ before the pipes
# polycodec.rb decode2 '_c`|@_upzA_c`|@_c`|@_c`|@_c`|@' [[1.0, 1.5], [2.0, 2.5], [3.0, 3.5]]
# polycodec.rb decode2 '_gayB_yqwC_c`|@_c`|@_c`|@_c`|@' [[2.0, 2.5], [3.0, 3.5], [4.0, 4.5]]
