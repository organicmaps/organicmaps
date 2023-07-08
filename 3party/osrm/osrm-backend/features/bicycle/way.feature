@routing @bicycle @way
Feature: Bike - Accessability of different way types

    Background:
        Given the profile "bicycle"

    Scenario: Bike - Routability of way types
    # Bikes are allowed on footways etc because you can pull your bike at a lower speed.
    # Pier is not allowed, since it's tagged using man_made=pier.

        Then routability should be
            | highway        | bothw |
            | (nil)          |       |
            | motorway       |       |
            | motorway_link  |       |
            | trunk          |       |
            | trunk_link     |       |
            | primary        | x     |
            | primary_link   | x     |
            | secondary      | x     |
            | secondary_link | x     |
            | tertiary       | x     |
            | tertiary_link  | x     |
            | residential    | x     |
            | service        | x     |
            | unclassified   | x     |
            | living_street  | x     |
            | road           | x     |
            | track          | x     |
            | path           | x     |
            | footway        | x     |
            | pedestrian     | x     |
            | steps          | x     |
            | cycleway       | x     |
            | bridleway      |       |
            | pier           |       |

    Scenario: Bike - Routability of man_made structures
        Then routability should be
            | highway | man_made | bothw |
            | (nil)   | (nil)    |       |
            | (nil)   | pier     | x     |
