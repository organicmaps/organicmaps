@routing @testbot @mode
Feature: Testbot - Mode flag

    Background:
        Given the profile "testbot"

    @todo
    Scenario: Bike - Mode
        Given the node map
            | a | b |   |
            |   | c | d |

        And the ways
            | nodes | highway | route | duration |
            | ab    | primary |       |          |
            | bc    |         | ferry | 0:01     |
            | cd    | primary |       |          |

        When I route I should get
            | from | to | route    | turns                       | modes         |
            | a    | d  | ab,bc,cd | head,right,left,destination | bot,ferry,bot |
            | d    | a  | cd,bc,ab | head,right left,destination | bot,ferry,bot |
            | c    | a  | bc,ab    | head,left,destination       | ferry,bot     |
            | d    | b  | cd,bc    | head,right,destination      | bot,ferry     |
            | a    | c  | ab,bc    | head,right,destination      | bot,ferry     |
            | b    | d  | bc,cd    | head,left,destination       | ferry,bot     |
