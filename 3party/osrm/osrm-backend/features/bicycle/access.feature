@routing @bicycle @access
Feature: Bike - Access tags on ways
# Reference: http://wiki.openstreetmap.org/wiki/Key:access

    Background:
        Given the profile "bicycle"

    Scenario: Bike - Access tag hierachy on ways
        Then routability should be
            | highway | access | vehicle | bicycle | bothw |
            |         |        |         |         | x     |
            |         | yes    |         |         | x     |
            |         | no     |         |         |       |
            |         |        | yes     |         | x     |
            |         |        | no      |         |       |
            |         | no     | yes     |         | x     |
            |         | yes    | no      |         |       |
            |         |        |         | yes     | x     |
            |         |        |         | no      |       |
            |         | no     |         | yes     | x     |
            |         | yes    |         | no      |       |
            |         |        | no      | yes     | x     |
            |         |        | yes     | no      |       |
            | runway  |        |         |         |       |
            | runway  | yes    |         |         | x     |
            | runway  | no     |         |         |       |
            | runway  |        | yes     |         | x     |
            | runway  |        | no      |         |       |
            | runway  | no     | yes     |         | x     |
            | runway  | yes    | no      |         |       |
            | runway  |        |         | yes     | x     |
            | runway  |        |         | no      |       |
            | runway  | no     |         | yes     | x     |
            | runway  | yes    |         | no      |       |
            | runway  |        | no      | yes     | x     |
            | runway  |        | yes     | no      |       |

    @todo
    Scenario: Bike - Access tag in forward direction
        Then routability should be
            | highway | access:forward | vehicle:forward | bicycle:forward | forw | backw |
            |         |                |                 |                 | x    |       |
            |         | yes            |                 |                 | x    |       |
            |         | no             |                 |                 |      |       |
            |         |                | yes             |                 | x    |       |
            |         |                | no              |                 |      |       |
            |         | no             | yes             |                 | x    |       |
            |         | yes            | no              |                 |      |       |
            |         |                |                 | yes             | x    |       |
            |         |                |                 | no              |      |       |
            |         | no             |                 | yes             | x    |       |
            |         | yes            |                 | no              |      |       |
            |         |                | no              | yes             | x    |       |
            |         |                | yes             | no              |      |       |
            | runway  |                |                 |                 | x    |       |
            | runway  | yes            |                 |                 | x    |       |
            | runway  | no             |                 |                 |      |       |
            | runway  |                | yes             |                 | x    |       |
            | runway  |                | no              |                 |      |       |
            | runway  | no             | yes             |                 | x    |       |
            | runway  | yes            | no              |                 |      |       |
            | runway  |                |                 | yes             | x    |       |
            | runway  |                |                 | no              |      |       |
            | runway  | no             |                 | yes             | x    |       |
            | runway  | yes            |                 | no              |      |       |
            | runway  |                | no              | yes             | x    |       |
            | runway  |                | yes             | no              |      |       |

    @todo
    Scenario: Bike - Access tag in backward direction
        Then routability should be
            | highway | access:forward | vehicle:forward | bicycle:forward | forw | backw |
            |         |                |                 |                 |      | x     |
            |         | yes            |                 |                 |      | x     |
            |         | no             |                 |                 |      |       |
            |         |                | yes             |                 |      | x     |
            |         |                | no              |                 |      |       |
            |         | no             | yes             |                 |      | x     |
            |         | yes            | no              |                 |      |       |
            |         |                |                 | yes             |      | x     |
            |         |                |                 | no              |      |       |
            |         | no             |                 | yes             |      | x     |
            |         | yes            |                 | no              |      |       |
            |         |                | no              | yes             |      | x     |
            |         |                | yes             | no              |      |       |
            | runway  |                |                 |                 |      | x     |
            | runway  | yes            |                 |                 |      | x     |
            | runway  | no             |                 |                 |      |       |
            | runway  |                | yes             |                 |      | x     |
            | runway  |                | no              |                 |      |       |
            | runway  | no             | yes             |                 |      | x     |
            | runway  | yes            | no              |                 |      |       |
            | runway  |                |                 | yes             |      | x     |
            | runway  |                |                 | no              |      |       |
            | runway  | no             |                 | yes             |      | x     |
            | runway  | yes            |                 | no              |      |       |
            | runway  |                | no              | yes             |      | x     |
            | runway  |                | yes             | no              |      |       |

    Scenario: Bike - Overwriting implied acccess on ways
        Then routability should be
            | highway  | access | vehicle | bicycle | bothw |
            | cycleway |        |         |         | x     |
            | runway   |        |         |         |       |
            | cycleway | no     |         |         |       |
            | cycleway |        | no      |         |       |
            | cycleway |        |         | no      |       |
            | runway   | yes    |         |         | x     |
            | runway   |        | yes     |         | x     |
            | runway   |        |         | yes     | x     |

    Scenario: Bike - Access tags on ways
        Then routability should be
            | access       | vehicle      | bicycle      | bothw |
            |              |              |              | x     |
            | yes          |              |              | x     |
            | permissive   |              |              | x     |
            | designated   |              |              | x     |
            | some_tag     |              |              | x     |
            | no           |              |              |       |
            | private      |              |              |       |
            | agricultural |              |              |       |
            | forestery    |              |              |       |
            |              | yes          |              | x     |
            |              | permissive   |              | x     |
            |              | designated   |              | x     |
            |              | some_tag     |              | x     |
            |              | no           |              |       |
            |              | private      |              |       |
            |              | agricultural |              |       |
            |              | forestery    |              |       |
            |              |              | yes          | x     |
            |              |              | permissive   | x     |
            |              |              | designated   | x     |
            |              |              | some_tag     | x     |
            |              |              | no           |       |
            |              |              | private      |       |
            |              |              | agricultural |       |
            |              |              | forestery    |       |

    Scenario: Bike - Access tags on both node and way
        Then routability should be
            | access   | node/access | bothw |
            | yes      | yes         | x     |
            | yes      | no          |       |
            | yes      | some_tag    | x     |
            | no       | yes         |       |
            | no       | no          |       |
            | no       | some_tag    |       |
            | some_tag | yes         | x     |
            | some_tag | no          |       |
            | some_tag | some_tag    | x     |

    Scenario: Bike - Access combinations
        Then routability should be
            | highway     | access     | vehicle    | bicycle    | forw | backw |
            | runway      | private    |            | yes        | x    | x     |
            | footway     |            | no         | permissive | x    | x     |
            | motorway    |            |            | yes        | x    |       |
            | track       | forestry   |            | permissive | x    | x     |
            | cycleway    | yes        | designated | no         |      |       |
            | primary     |            | yes        | private    |      |       |
            | residential | permissive |            | no         |      |       |

    Scenario: Bike - Ignore access tags for other modes
        Then routability should be
            | highway  | boat | motor_vehicle | moped | bothw |
            | river    | yes  |               |       |       |
            | cycleway | no   |               |       | x     |
            | runway   |      | yes           |       |       |
            | cycleway |      | no            |       | x     |
            | runway   |      |               | yes   |       |
            | cycleway |      |               | no    | x     |
