@routing @foot @surface
Feature: Foot - Surfaces

    Background:
        Given the profile "foot"

    Scenario: Foot - Slow surfaces
        Then routability should be
            | highway | surface     | bothw     |
            | footway |             | 145s ~10% |
            | footway | fine_gravel | 193s ~10% |
            | footway | gravel      | 193s ~10% |
            | footway | pebbelstone | 193s ~10% |
            | footway | mud         | 289s ~10% |
            | footway | sand        | 289s ~10% |
