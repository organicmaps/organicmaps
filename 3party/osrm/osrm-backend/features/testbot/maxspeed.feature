@routing @maxspeed @testbot
Feature: Car - Max speed restrictions

    Background: Use specific speeds
        Given the profile "testbot"

    Scenario: Testbot - Respect maxspeeds when lower that way type speed
        Then routability should be
            | maxspeed | bothw   |
            |          | 36 km/h |
            | 18       | 18 km/h |

    Scenario: Testbot - Ignore maxspeed when higher than way speed
        Then routability should be
            | maxspeed | bothw   |
            |          | 36 km/h |
            | 100 km/h | 36 km/h |

    @opposite
    Scenario: Testbot - Forward/backward maxspeed
        Then routability should be
            | maxspeed | maxspeed:forward | maxspeed:backward | forw    | backw   |
            |          |                  |                   | 36 km/h | 36 km/h |
            | 18       |                  |                   | 18 km/h | 18 km/h |
            |          | 18               |                   | 18 km/h | 36 km/h |
            |          |                  | 18                | 36 km/h | 18 km/h |
            | 9        | 18               |                   | 18 km/h | 9 km/h  |
            | 9        |                  | 18                | 9 km/h  | 18 km/h |
            | 9        | 24               | 18                | 24 km/h | 18 km/h |