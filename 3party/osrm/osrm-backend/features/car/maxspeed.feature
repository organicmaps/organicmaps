@routing @maxspeed @car
Feature: Car - Max speed restrictions
OSRM will use 4/5 of the projected free-flow speed.

    Background: Use specific speeds
        Given the profile "car"
        Given a grid size of 1000 meters

    Scenario: Car - Respect maxspeeds when lower that way type speed
        Given the node map
            | a | b | c |

        And the ways
            | nodes | highway | maxspeed |
            | ab    | trunk   |          |
            | bc    | trunk   | 60       |

        When I route I should get
            | from | to | route | speed        |
            | a    | b  | ab    | 67 km/h      |
            | b    | c  | bc    | 48 km/h +- 1 |

    Scenario: Car - Do not ignore maxspeed when higher than way speed
        Given the node map
            | a | b | c |

        And the ways
            | nodes | highway     | maxspeed |
            | ab    | residential |          |
            | bc    | residential | 90       |

        When I route I should get
            | from | to | route | speed        |
            | a    | b  | ab    | 20 km/h      |
            | b    | c  | bc    | 72 km/h +- 1 |

    Scenario: Car - Forward/backward maxspeed
        Given a grid size of 100 meters

        Then routability should be
            | highway | maxspeed | maxspeed:forward | maxspeed:backward | forw        | backw        |
            | primary |          |                  |                   | 51 km/h     | 51 km/h      |
            | primary | 60       |                  |                   | 48 km/h     | 48 km/h      |
            | primary |          | 60               |                   | 48 km/h     | 65 km/h      |
            | primary |          |                  | 60                | 51 km/h     | 48 km/h      |
            | primary | 15       | 60               |                   | 48 km/h     | 15 km/h +- 1 |
            | primary | 15       |                  | 60                | 12 km/h +- 1| 48 km/h      |
            | primary | 15       | 30               | 60                | 24 km/h     | 48 km/h      |

    Scenario: Car - Maxspeed should not allow routing on unroutable ways
        Then routability should be
            | highway   | railway | access | maxspeed | maxspeed:forward | maxspeed:backward | bothw |
            | primary   |         |        |          |                  |                   | x     |
            | secondary |         | no     |          |                  |                   |       |
            | secondary |         | no     | 100      |                  |                   |       |
            | secondary |         | no     |          | 100              |                   |       |
            | secondary |         | no     |          |                  | 100               |       |
            | (nil)     | train   |        |          |                  |                   |       |
            | (nil)     | train   |        | 100      |                  |                   |       |
            | (nil)     | train   |        |          | 100              |                   |       |
            | (nil)     | train   |        |          |                  | 100               |       |
            | runway    |         |        |          |                  |                   |       |
            | runway    |         |        | 100      |                  |                   |       |
            | runway    |         |        |          | 100              |                   |       |
            | runway    |         |        |          |                  | 100               |       |
