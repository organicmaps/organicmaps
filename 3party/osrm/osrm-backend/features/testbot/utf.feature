@routing @utf @testbot
Feature: Handling of UTF characters

    Background:
        Given the profile "testbot"

    Scenario: Streetnames with UTF characters
        Given the node map
            | a | b | c | d |

        And the ways
            | nodes | name                   |
            | ab    | Scandinavian København |
            | bc    | Japanese 東京            |
            | cd    | Cyrillic Москва        |

        When I route I should get
            | from | to | route                  |
            | a    | b  | Scandinavian København |
            | b    | c  | Japanese 東京            |
            | c    | d  | Cyrillic Москва        |
