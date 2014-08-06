@routing @surface @bicycle
Feature: Bike - Surfaces

    Background:
        Given the profile "bicycle"

    Scenario: Bicycle - Slow surfaces
        Then routability should be
            | highway  | surface               | bothw |
            | cycleway |                       | 48s   |
            | cycleway | asphalt               | 48s   |
            | cycleway | cobblestone:flattened | 72s   |
            | cycleway | paving_stones         | 72s   |
            | cycleway | compacted             | 72s   |
            | cycleway | cobblestone           | 120s  |
            | cycleway | unpaved               | 120s  |
            | cycleway | fine_gravel           | 120s  |
            | cycleway | gravel                | 120s  |
            | cycleway | pebbelstone           | 120s  |
            | cycleway | dirt                  | 120s  |
            | cycleway | earth                 | 120s  |
            | cycleway | grass                 | 120s  |
            | cycleway | mud                   | 240s  |
            | cycleway | sand                  | 240s  |

    Scenario: Bicycle - Good surfaces on small paths
        Then routability should be
        | highway  | surface | bothw |
        | cycleway |         | 48s   |
        | path     |         | 60s   |
        | track    |         | 60s   |
        | track    | asphalt | 48s   |
        | path     | asphalt | 48s   |

    Scenario: Bicycle - Surfaces should not make unknown ways routable
        Then routability should be
        | highway  | surface | bothw |
        | cycleway |         | 48s   |
        | nosense  |         |       |
        | nosense  | asphalt |       |
