@routing @foot @barrier
Feature: Barriers

    Background:
        Given the profile "foot"

    Scenario: Foot - Barriers
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

    Scenario: Foot - Access tag trumphs barriers
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
            | gate         | no           |       |
            | gate         | private      |       |
            | gate         | agricultural |       |
