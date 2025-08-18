@routing @foot @oneway
Feature: Foot - Oneway streets
# Handle oneways streets, as defined at http://wiki.openstreetmap.org/wiki/OSM_tags_for_routing

    Background:
        Given the profile "foot"

    Scenario: Foot - Walking should not be affected by oneways
        Then routability should be
            | oneway   | bothw |
            |          | x     |
            | nonsense | x     |
            | no       | x     |
            | false    | x     |
            | 0        | x     |
            | yes      | x     |
            | true     | x     |
            | 1        | x     |
            | -1       | x     |

    Scenario: Foot - Walking and roundabouts
        Then routability should be
            | junction   | bothw |
            | roundarout | x     |

    Scenario: Foot - Oneway:foot tag should not cause walking on big roads
        Then routability should be
            | highway       | oneway:foot | bothw |
            | footway       |             | x     |
            | motorway      | yes         |       |
            | motorway_link | yes         |       |
            | trunk         | yes         |       |
            | trunk_link    | yes         |       |
            | motorway      | no          |       |
            | motorway_link | no          |       |
            | trunk         | no          |       |
            | trunk_link    | no          |       |
            | motorway      | -1          |       |
            | motorway_link | -1          |       |
            | trunk         | -1          |       |
            | trunk_link    | -1          |       |

    Scenario: Foot - Walking should respect oneway:foot
        Then routability should be
            | oneway:foot | oneway | junction   | forw | backw |
            | yes         |        |            | x    |       |
            | yes         | yes    |            | x    |       |
            | yes         | no     |            | x    |       |
            | yes         | -1     |            | x    |       |
            | yes         |        | roundabout | x    |       |
            | no          |        |            | x    | x     |
            | no          | yes    |            | x    | x     |
            | no          | no     |            | x    | x     |
            | no          | -1     |            | x    | x     |
            | no          |        | roundabout | x    | x     |
            | -1          |        |            |      | x     |
            | -1          | yes    |            |      | x     |
            | -1          | no     |            |      | x     |
            | -1          | -1     |            |      | x     |
            | -1          |        | roundabout |      | x     |
