@routing @testbot @example
Feature: Testbot - Walkthrough
# A complete walk-through of how this data is processed can be found at:
# https://github.com/DennisOSRM/Project-OSRM/wiki/Processing-Flow

    Background:
        Given the profile "testbot"

    Scenario: Testbot - Processing Flow
        Given the node map
            |   |   |   | d |
            | a | b | c |   |
            |   |   |   | e |

        And the ways
            | nodes | highway | oneway |
            | abc   | primary |        |
            | cd    | primary | yes    |
            | ce    | river   |        |
            | de    | primary |        |

        When I route I should get
            | from | to | route     |
            | a    | b  | abc       |
            | a    | c  | abc       |
            | a    | d  | abc,cd    |
            | a    | e  | abc,ce    |
            | b    | a  | abc       |
            | b    | c  | abc       |
            | b    | d  | abc,cd    |
            | b    | e  | abc,ce    |
            | d    | a  | de,ce,abc |
            | d    | b  | de,ce,abc |
            | d    | e  | de        |
            | e    | a  | ce,abc    |
            | e    | b  | ce,abc    |
            | e    | c  | ce        |
