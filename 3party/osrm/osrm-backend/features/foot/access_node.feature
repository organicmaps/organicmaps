@routing @foot @access
Feature: Foot - Access tags on nodes
# Reference: http://wiki.openstreetmap.org/wiki/Key:access

    Background:
        Given the profile "foot"

    Scenario: Foot - Access tag hierachy on nodes
        Then routability should be
            | node/access | node/foot | bothw |
            |             |           | x     |
            |             | yes       | x     |
            |             | no        |       |
            | yes         |           | x     |
            | yes         | yes       | x     |
            | yes         | no        |       |
            | no          |           |       |
            | no          | yes       | x     |
            | no          | no        |       |

    Scenario: Foot - Overwriting implied acccess on nodes doesn't overwrite way
        Then routability should be
            | highway  | node/access | node/foot | bothw |
            | footway  |             |           | x     |
            | footway  | no          |           |       |
            | footway  |             | no        |       |
            | motorway |             |           |       |
            | motorway | yes         |           |       |
            | motorway |             | yes       |       |

    Scenario: Foot - Access tags on nodes
        Then routability should be
            | node/access  | node/foot    | bothw |
            |              |              | x     |
            | yes          |              | x     |
            | permissive   |              | x     |
            | designated   |              | x     |
            | some_tag     |              | x     |
            | no           |              |       |
            | private      |              |       |
            | agricultural |              |       |
            | forestery    |              |       |
            | no           | yes          | x     |
            | no           | permissive   | x     |
            | no           | designated   | x     |
            | no           | some_tag     | x     |
            | yes          | no           |       |
            | yes          | private      |       |
            | yes          | agricultural |       |
            | yes          | forestery    |       |
