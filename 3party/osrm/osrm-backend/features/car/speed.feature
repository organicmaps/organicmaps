@routing @car @speed
Feature: Car - speeds

    Background:
        Given the profile "car"
        And a grid size of 1000 meters

    Scenario: Car - speed of various way types
        Then routability should be
            | highway        | oneway | bothw        |
            | motorway       | no     | 72 km/h      |
            | motorway_link  | no     | 60 km/h      |
            | trunk          | no     | 67 km/h +- 1 |
            | trunk_link     | no     | 55 km/h +- 1 |
            | primary        | no     | 52 km/h +- 1 |
            | primary_link   | no     | 48 km/h      |
            | secondary      | no     | 43 km/h +- 1 |
            | secondary_link | no     | 40 km/h      |
            | tertiary       | no     | 32 km/h      |
            | tertiary_link  | no     | 24 km/h      |
            | unclassified   | no     | 20 km/h      |
            | residential    | no     | 20 km/h      |
            | living_street  | no     |  8 km/h      |
            | service        | no     | 12 km/h      |
