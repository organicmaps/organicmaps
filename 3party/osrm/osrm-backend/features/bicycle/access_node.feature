@routing @bicycle @access
Feature: Bike - Access tags on nodes
# Reference: http://wiki.openstreetmap.org/wiki/Key:access

    Background:
        Given the profile "bicycle"

    Scenario: Bike - Access tag hierachy on nodes
        Then routability should be
            | node/access | node/vehicle | node/bicycle | bothw |
            |             |              |              | x     |
            | yes         |              |              | x     |
            | no          |              |              |       |
            |             | yes          |              | x     |
            |             | no           |              |       |
            | no          | yes          |              | x     |
            | yes         | no           |              |       |
            |             |              | yes          | x     |
            |             |              | no           |       |
            | no          |              | yes          | x     |
            | yes         |              | no           |       |
            |             | no           | yes          | x     |
            |             | yes          | no           |       |

    Scenario: Bike - Overwriting implied acccess on nodes doesn't overwrite way
        Then routability should be
            | highway  | node/access | node/vehicle | node/bicycle | bothw |
            | cycleway |             |              |              | x     |
            | runway   |             |              |              |       |
            | cycleway | no          |              |              |       |
            | cycleway |             | no           |              |       |
            | cycleway |             |              | no           |       |
            | runway   | yes         |              |              |       |
            | runway   |             | yes          |              |       |
            | runway   |             |              | yes          |       |

    Scenario: Bike - Access tags on nodes
        Then routability should be
            | node/access  | node/vehicle | node/bicycle | bothw |
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
