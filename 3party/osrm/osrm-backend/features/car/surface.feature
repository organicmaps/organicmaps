@routing @car @surface
Feature: Car - Surfaces

    Background:
        Given the profile "car"

    Scenario: Car - Routeability of tracktype tags
        Then routability should be
            | highway | tracktype | bothw |
            | trunk   | grade1    | x     |
            | trunk   | grade2    | x     |
            | trunk   | grade3    | x     |
            | trunk   | grade4    | x     |
            | trunk   | grade5    | x     |
            | trunk   | nonsense  | x     |

    Scenario: Car - Routability of smoothness tags
        Then routability should be
            | highway | smoothness    | bothw |
            | trunk   | excellent     | x     |
            | trunk   | good          | x     |
            | trunk   | intermediate  | x     |
            | trunk   | bad           | x     |
            | trunk   | very_bad      | x     |
            | trunk   | horrible      | x     |
            | trunk   | very_horrible | x     |
            | trunk   | impassable    |       |
            | trunk   | nonsense      | x     |
 
    Scenario: Car - Routabiliy of surface tags
        Then routability should be
            | highway | surface  | bothw |
            | trunk   | asphalt  | x     |
            | trunk   | sett     | x     |
            | trunk   | gravel   | x     |
            | trunk   | nonsense | x     |

    Scenario: Car - Good surfaces should not grant access
        Then routability should be
            | highway  | access       | tracktype | smoothness | surface | forw | backw |
            | motorway |              |           |            |         | x    |       |
            | motorway | no           | grade1    | excellent  | asphalt |      |       |
            | motorway | private      | grade1    | excellent  | asphalt |      |       |
            | motorway | agricultural | grade1    | excellent  | asphalt |      |       |
            | motorway | forestry     | grade1    | excellent  | asphalt |      |       |
            | motorway | emergency    | grade1    | excellent  | asphalt |      |       |
            | primary  |              |           |            |         | x    | x     |
            | primary  | no           | grade1    | excellent  | asphalt |      |       |
            | primary  | private      | grade1    | excellent  | asphalt |      |       |
            | primary  | agricultural | grade1    | excellent  | asphalt |      |       |
            | primary  | forestry     | grade1    | excellent  | asphalt |      |       |
            | primary  | emergency    | grade1    | excellent  | asphalt |      |       |

    Scenario: Car - Impassable surfaces should deny access
        Then routability should be
            | highway  | access | smoothness | forw | backw |
            | motorway |        | impassable |      |       |
            | motorway | yes    |            | x    |       |
            | motorway | yes    | impassable |      |       |
            | primary  |        | impassable |      |       |
            | primary  | yes    |            | x    | x     |
            | primary  | yes    | impassable |      |       |

    Scenario: Car - Surface should reduce speed
        Then routability should be
            | highway  | oneway | surface         | forw        | backw       |
            | motorway | no     |                 | 80 km/h +-1 | 80 km/h +-1 |
            | motorway | no     | asphalt         | 80 km/h +-1 | 80 km/h +-1 |
            | motorway | no     | concrete        | 80 km/h +-1 | 80 km/h +-1 |
            | motorway | no     | concrete:plates | 80 km/h +-1 | 80 km/h +-1 |
            | motorway | no     | concrete:lanes  | 80 km/h +-1 | 80 km/h +-1 |
            | motorway | no     | paved           | 80 km/h +-1 | 80 km/h +-1 |
            | motorway | no     | cement          | 72 km/h +-1 | 72 km/h +-1 |
            | motorway | no     | compacted       | 72 km/h +-1 | 72 km/h +-1 |
            | motorway | no     | fine_gravel     | 72 km/h +-1 | 72 km/h +-1 |
            | motorway | no     | paving_stones   | 60 km/h +-1 | 60 km/h +-1 |
            | motorway | no     | metal           | 60 km/h +-1 | 60 km/h +-1 |
            | motorway | no     | bricks          | 60 km/h +-1 | 60 km/h +-1 |
            | motorway | no     | grass           | 42 km/h +-1 | 42 km/h +-1 |
            | motorway | no     | wood            | 42 km/h +-1 | 42 km/h +-1 |
            | motorway | no     | sett            | 42 km/h +-1 | 42 km/h +-1 |
            | motorway | no     | grass_paver     | 42 km/h +-1 | 42 km/h +-1 |
            | motorway | no     | gravel          | 42 km/h +-1 | 42 km/h +-1 |
            | motorway | no     | unpaved         | 42 km/h +-1 | 42 km/h +-1 |
            | motorway | no     | ground          | 42 km/h +-1 | 42 km/h +-1 |
            | motorway | no     | dirt            | 42 km/h +-1 | 42 km/h +-1 |
            | motorway | no     | pebblestone     | 42 km/h +-1 | 42 km/h +-1 |
            | motorway | no     | tartan          | 42 km/h +-1 | 42 km/h +-1 |
            | motorway | no     | cobblestone     | 34 km/h +-1 | 34 km/h +-1 |
            | motorway | no     | clay            | 34 km/h +-1 | 34 km/h +-1 |
            | motorway | no     | earth           | 26 km/h +-1 | 26 km/h +-1 |
            | motorway | no     | stone           | 26 km/h +-1 | 26 km/h +-1 |
            | motorway | no     | rocky           | 26 km/h +-1 | 26 km/h +-1 |
            | motorway | no     | sand            | 26 km/h +-1 | 26 km/h +-1 |

    Scenario: Car - Tracktypes should reduce speed
        Then routability should be
            | highway  | oneway | tracktype | forw        | backw       |
            | motorway | no     |           | 80 km/h +-1 | 80 km/h +-1 |
            | motorway | no     | grade1    | 60 km/h +-1 | 60 km/h +-1 |
            | motorway | no     | grade2    | 42 km/h +-1 | 42 km/h +-1 |
            | motorway | no     | grade3    | 34 km/h +-1 | 34 km/h +-1 |
            | motorway | no     | grade4    | 31 km/h +-1 | 31 km/h +-1 |
            | motorway | no     | grade5    | 26 km/h +-1 | 26 km/h +-1 |

    Scenario: Car - Smoothness should reduce speed
        Then routability should be
            | highway  | oneway | smoothness    | forw        | backw       |
            | motorway | no     |               | 80 km/h +-1 | 80 km/h +-1 |
            | motorway | no     | intermediate  | 72 km/h +-1 | 72 km/h +-1 |
            | motorway | no     | bad           | 42 km/h +-1 | 42 km/h +-1 |
            | motorway | no     | very_bad      | 26 km/h +-1 | 26 km/h +-1 |
            | motorway | no     | horrible      | 18 km/h +-1 | 18 km/h +-1 |
            | motorway | no     | very_horrible | 15 km/h +-1 | 15 km/h +-1 |

    Scenario: Car - Combination of surface tags should use lowest speed
        Then routability should be
            | highway  | oneway | tracktype | surface | smoothness    | backw   | forw    |
            | motorway | no     |           |         |               | 80 km/h | 80 km/h |
            | service  | no     | grade1    | asphalt | excellent     | 23 km/h | 23 km/h |
            | motorway | no     | grade5    | asphalt | excellent     | 26 km/h | 26 km/h |
            | motorway | no     | grade1    | mud     | excellent     | 18 km/h | 18 km/h |
            | motorway | no     | grade1    | asphalt | very_horrible | 15 km/h | 15 km/h |
            | service  | no     | grade5    | mud     | very_horrible | 15 km/h | 15 km/h |

    Scenario: Car - Surfaces should not affect oneway direction
        Then routability should be
            | highway | oneway | tracktype | smoothness | surface  | forw | backw |
            | primary |        | grade1    | excellent  | asphalt  | x    | x     |
            | primary |        | grade5    | very_bad   | mud      | x    | x     |
            | primary |        | nonsense  | nonsense   | nonsense | x    | x     |
            | primary | no     | grade1    | excellent  | asphalt  | x    | x     |
            | primary | no     | grade5    | very_bad   | mud      | x    | x     |
            | primary | no     | nonsense  | nonsense   | nonsense | x    | x     |
            | primary | yes    | grade1    | excellent  | asphalt  | x    |       |
            | primary | yes    | grade5    | very_bad   | mud      | x    |       |
            | primary | yes    | nonsense  | nonsense   | nonsense | x    |       |
            | primary | -1     | grade1    | excellent  | asphalt  |      | x     |
            | primary | -1     | grade5    | very_bad   | mud      |      | x     |
            | primary | -1     | nonsense  | nonsense   | nonsense |      | x     |
    
