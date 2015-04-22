@routing @testbot @overlap
Feature: Testbot - overlapping ways
 
    Background:
        Given the profile "testbot"

    @bug @610
    Scenario: Testbot - multiple way between same nodes 
    Note that cb is connecting the same two nodes as bc
        Given the node map
            | a | b | c | d |

        And the ways
            | nodes | highway   |
            | ab    | primary   |
            | bc    | primary   |
            | cd    | primary   |
            | cb    | secondary |

        When I route I should get
            | from | to | route    |
            | a    | d  | ab,bc,cd |
            | d    | a  | cd,bc,ab |
    
    @bug @610
    Scenario: Testbot - area on top of way
        Given the node map
            | x | a | b | y |
            |   | d | c |   |

        And the ways
            | nodes | highway   | area |
            | xaby  | primary   |      |
            | abcda | secondary | yes  |

        When I route I should get
            | from | to | route |
            | x    | y  | xaby  |
            | y    | x  | xaby  |
