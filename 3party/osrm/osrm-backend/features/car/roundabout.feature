@routing @car @roundabout @instruction
Feature: Roundabout Instructions

    Background:
        Given the profile "car"

    Scenario: Car - Roundabout instructions
        Given the node map
            |   |   | v |   |   |
            |   |   | d |   |   |
            | s | a |   | c | u |
            |   |   | b |   |   |
            |   |   | t |   |   |

        And the ways
            | nodes | junction   |
            | sa    |            |
            | tb    |            |
            | uc    |            |
            | vd    |            |
            | abcda | roundabout |

        When I route I should get
            | from | to | route | turns                               |
            | s    | t  | sa,tb | head,enter_roundabout-1,destination |
            | s    | u  | sa,uc | head,enter_roundabout-2,destination |
            | s    | v  | sa,vd | head,enter_roundabout-3,destination |
            | u    | v  | uc,vd | head,enter_roundabout-1,destination |
            | u    | s  | uc,sa | head,enter_roundabout-2,destination |
            | u    | t  | uc,tb | head,enter_roundabout-3,destination |
