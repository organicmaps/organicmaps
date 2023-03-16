@routing @maxspeed @bicycle
Feature: Bike - Max speed restrictions

    Background: Use specific speeds
        Given the profile "bicycle"
        And a grid size of 1000 meters

    Scenario: Bicycle - Respect maxspeeds when lower that way type speed
        Then routability should be
            | highway     | maxspeed | bothw   |
            | residential |          | 15 km/h |
            | residential | 10       | 10 km/h |

    Scenario: Bicycle - Ignore maxspeed when higher than way speed
        Then routability should be
            | highway     | maxspeed | bothw   |
            | residential |          | 15 km/h |
            | residential | 80       | 15 km/h |

    @todo
    Scenario: Bicycle - Maxspeed formats
        Then routability should be
            | highway     | maxspeed  | bothw     |
            | residential |           | 49s ~10%  |
            | residential | 5         | 144s ~10% |
            | residential | 5mph      | 90s ~10%  |
            | residential | 5 mph     | 90s ~10%  |
            | residential | 5MPH      | 90s ~10%  |
            | residential | 5 MPH     | 90s ~10%  |
            | trunk       | 5unknown  | 49s ~10%  |
            | trunk       | 5 unknown | 49s ~10%  |

    @todo
    Scenario: Bicycle - Maxspeed special tags
        Then routability should be
            | highway     | maxspeed | bothw    |
            | residential |          | 49s ~10% |
            | residential | none     | 49s ~10% |
            | residential | signals  | 49s ~10% |

    Scenario: Bike - Do not use maxspeed when higher that way type speed
        Given the node map
            | a | b | c |

        And the ways
            | nodes | highway     | maxspeed |
            | ab    | residential |          |
            | bc    | residential | 80       |

        When I route I should get
            | from | to | route | speed   |
            | a    | b  | ab    | 15 km/h |
            | b    | c  | bc    | 15 km/h |

    Scenario: Bike - Forward/backward maxspeed
        Given the shortcuts
            | key   | value     |
            | bike  | 49s ~10%  |
            | run   | 73s ~10%  |
            | walk  | 145s ~10% |
            | snail | 720s ~10% |

        Then routability should be
            | maxspeed | maxspeed:forward | maxspeed:backward | forw    | backw   |
            |          |                  |                   | 15 km/h | 15 km/h |
            | 10       |                  |                   | 10 km/h | 10 km/h |
            |          | 10               |                   | 10 km/h | 15 km/h |
            |          |                  | 10                | 15 km/h | 10 km/h |
            | 2        | 10               |                   | 10 km/h | 2 km/h  |
            | 2        |                  | 10                | 2 km/h  | 10 km/h |
            | 2        | 5                | 10                | 5 km/h  | 10 km/h |

    Scenario: Bike - Maxspeed should not allow routing on unroutable ways
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
