@routing @bicycle @train
Feature: Bike - Handle ferry routes
# Bringing bikes on trains and subways

    Background:
        Given the profile "bicycle"

    Scenario: Bike - Bringing bikes on trains
        Then routability should be
            | highway | railway    | bicycle | bothw |
            | primary |            |         | x     |
            | (nil)   | train      |         |       |
            | (nil)   | train      | no      |       |
            | (nil)   | train      | yes     | x     |
            | (nil)   | railway    |         |       |
            | (nil)   | railway    | no      |       |
            | (nil)   | railway    | yes     | x     |
            | (nil)   | subway     |         |       |
            | (nil)   | subway     | no      |       |
            | (nil)   | subway     | yes     | x     |
            | (nil)   | tram       |         |       |
            | (nil)   | tram       | no      |       |
            | (nil)   | tram       | yes     | x     |
            | (nil)   | light_rail |         |       |
            | (nil)   | light_rail | no      |       |
            | (nil)   | light_rail | yes     | x     |
            | (nil)   | monorail   |         |       |
            | (nil)   | monorail   | no      |       |
            | (nil)   | monorail   | yes     | x     |
            | (nil)   | some_tag   |         |       |
            | (nil)   | some_tag   | no      |       |
            | (nil)   | some_tag   | yes     | x     |

    @construction
    Scenario: Bike - Don't route on railways under construction
        Then routability should be
            | highway | railway      | bicycle | bothw |
            | primary |              |         | x     |
            | (nil)   | construction | yes     |       |
