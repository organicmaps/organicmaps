@routing @testbot @oneway
Feature: Testbot - oneways

    Background:
        Given the profile "testbot"

    Scenario: Routing on a oneway roundabout
        Given the node map
        | x |   |   | v |   |   |
        |   |   | d | c |   |   |
        |   | e |   |   | b |   |
        |   | f |   |   | a |   |
        |   |   | g | h |   |   |

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
