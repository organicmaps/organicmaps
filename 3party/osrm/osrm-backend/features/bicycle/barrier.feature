@routing @bicycle @barrier
Feature: Barriers

    Background:
        Given the profile "bicycle"

    Scenario: Bike - Barriers
        Then routability should be
            | node/barrier   | bothw |
            |                | x     |
            | bollard        | x     |
            | gate           | x     |
            | cycle_barrier  | x     |
            | cattle_grid    | x     |
            | border_control | x     |
            | toll_booth     | x     |
            | sally_port     | x     |
            | entrance       | x     |
            | wall           |       |
            | fence          |       |
            | some_tag       |       |

    Scenario: Bike - Access tag trumphs barriers
        Then routability should be
            | node/barrier | node/access  | bothw |
            | bollard      |              | x     |
            | bollard      | yes          | x     |
            | bollard      | permissive   | x     |
            | bollard      | designated   | x     |
            | bollard      | no           |       |
            | bollard      | private      |       |
            | bollard      | agricultural |       |
            | wall         |              |       |
            | wall         | yes          | x     |
            | wall         | permissive   | x     |
            | wall         | designated   | x     |
            | wall         | no           |       |
            | wall         | private      |       |
            | wall         | agricultural |       |
