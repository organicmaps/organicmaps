@routing @speed @testbot
Feature: Testbot - speeds

    Background: Use specific speeds
        Given the profile "testbot"

    Scenario: Testbot - Speed on roads
        Then routability should be
            | highway   | bothw   |
            | primary   | 36 km/h |
            | unknown   | 24 km/h |
            | secondary | 18 km/h |
            | tertiary  | 12 km/h |

    Scenario: Testbot - Speed on rivers, table
        Then routability should be
            | highway | forw    | backw   |
            | river   | 36 km/h | 16 km/h |

    Scenario: Testbot - Speed on rivers, map
        Given the node map
            | a | b |

        And the ways
            | nodes | highway |
            | ab    | river   |

        When I route I should get
            | from | to | route | speed        | time | distance |
            | a    | b  | ab    | 36 km/h      | 10s  | 100m     |
            | b    | a  | ab    | 16 km/h +- 1 | 23s  | 100m     |
