@routing @bicycle @oneway
Feature: Bike - Oneway streets
# Handle oneways streets, as defined at http://wiki.openstreetmap.org/wiki/OSM_tags_for_routing
# Usually we can push bikes against oneways, but we use foot=no to prevent this in these tests

    Background:
        Given the profile "bicycle"

    Scenario: Bike - Simple oneway
        Then routability should be
            | highway | foot | oneway | forw | backw |
            | primary | no   | yes    | x    |       |

    Scenario: Simple reverse oneway
        Then routability should be
            | highway | foot | oneway | forw | backw |
            | primary | no   | -1     |      | x     |

    Scenario: Bike - Around the Block
        Given the node map
            | a | b |
            | d | c |

        And the ways
            | nodes | oneway | foot |
            | ab    | yes    | no   |
            | bc    |        | no   |
            | cd    |        | no   |
            | da    |        | no   |

        When I route I should get
            | from | to | route    |
            | a    | b  | ab       |
            | b    | a  | bc,cd,da |

    Scenario: Bike - Handle various oneway tag values
        Then routability should be
            | foot | oneway   | forw | backw |
            | no   |          | x    | x     |
            | no   | nonsense | x    | x     |
            | no   | no       | x    | x     |
            | no   | false    | x    | x     |
            | no   | 0        | x    | x     |
            | no   | yes      | x    |       |
            | no   | true     | x    |       |
            | no   | 1        | x    |       |
            | no   | -1       |      | x     |

    Scenario: Bike - Implied oneways
        Then routability should be
            | highway       | foot | bicycle | junction   | forw | backw |
            |               | no   |         |            | x    | x     |
            |               | no   |         | roundabout | x    |       |
            | motorway      | no   | yes     |            | x    |       |
            | motorway_link | no   | yes     |            | x    |       |
            | motorway      | no   | yes     | roundabout | x    |       |
            | motorway_link | no   | yes     | roundabout | x    |       |

    Scenario: Bike - Overriding implied oneways
        Then routability should be
            | highway       | foot | junction   | oneway | forw | backw |
            | primary       | no   | roundabout | no     | x    | x     |
            | primary       | no   | roundabout | yes    | x    |       |
            | motorway_link | no   |            | -1     |      |       |
            | trunk_link    | no   |            | -1     |      |       |
            | primary       | no   | roundabout | -1     |      | x     |

    Scenario: Bike - Oneway:bicycle should override normal oneways tags
        Then routability should be
            | foot | oneway:bicycle | oneway | junction   | forw | backw |
            | no   | yes            |        |            | x    |       |
            | no   | yes            | yes    |            | x    |       |
            | no   | yes            | no     |            | x    |       |
            | no   | yes            | -1     |            | x    |       |
            | no   | yes            |        | roundabout | x    |       |
            | no   | no             |        |            | x    | x     |
            | no   | no             | yes    |            | x    | x     |
            | no   | no             | no     |            | x    | x     |
            | no   | no             | -1     |            | x    | x     |
            | no   | no             |        | roundabout | x    | x     |
            | no   | -1             |        |            |      | x     |
            | no   | -1             | yes    |            |      | x     |
            | no   | -1             | no     |            |      | x     |
            | no   | -1             | -1     |            |      | x     |
            | no   | -1             |        | roundabout |      | x     |

    Scenario: Bike - Contra flow
        Then routability should be
            | foot | oneway | cycleway       | forw | backw |
            | no   | yes    | opposite       | x    | x     |
            | no   | yes    | opposite_track | x    | x     |
            | no   | yes    | opposite_lane  | x    | x     |
            | no   | -1     | opposite       | x    | x     |
            | no   | -1     | opposite_track | x    | x     |
            | no   | -1     | opposite_lane  | x    | x     |
            | no   | no     | opposite       | x    | x     |
            | no   | no     | opposite_track | x    | x     |
            | no   | no     | opposite_lane  | x    | x     |

    Scenario: Bike - Should not be affected by car tags
        Then routability should be
            | foot | junction   | oneway | oneway:car | forw | backw |
            | no   |            | yes    | yes        | x    |       |
            | no   |            | yes    | no         | x    |       |
            | no   |            | yes    | -1         | x    |       |
            | no   |            | no     | yes        | x    | x     |
            | no   |            | no     | no         | x    | x     |
            | no   |            | no     | -1         | x    | x     |
            | no   |            | -1     | yes        |      | x     |
            | no   |            | -1     | no         |      | x     |
            | no   |            | -1     | -1         |      | x     |
            | no   | roundabout |        | yes        | x    |       |
            | no   | roundabout |        | no         | x    |       |
            | no   | roundabout |        | -1         | x    |       |

    Scenario: Bike - Two consecutive oneways
        Given the node map
            | a | b | c |

        And the ways
            | nodes | oneway |
            | ab    | yes    |
            | bc    | yes    |


        When I route I should get
            | from | to | route |
            | a    | c  | ab,bc |
