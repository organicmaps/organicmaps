@routing @car @barrier
Feature: Car - Barriers

    Background:
        Given the profile "car"

    Scenario: Car - Barriers
        Then routability should be
            | node/barrier   | bothw |
            |                | x     |
            | bollard        |       |
            | gate           | x     |
            | lift_gate      | x     |
            | cattle_grid    | x     |
            | border_control | x     |
            | toll_booth     | x     |
            | sally_port     | x     |
            | entrance       | x     |
            | wall           |       |
            | fence          |       |
            | some_tag       |       |

    Scenario: Car - Access tag trumphs barriers
        Then routability should be
            | node/barrier | node/access   | bothw |
            | gate         |               | x     |
            | gate         | yes           | x     |
            | gate         | permissive    | x     |
            | gate         | designated    | x     |
            | gate         | no            |       |
            | gate         | private       |       |
            | gate         | agricultural  |       |
            | wall         |               |       |
            | wall         | yes           | x     |
            | wall         | permissive    | x     |
            | wall         | designated    | x     |
            | wall         | no            |       |
            | wall         | private       |       |
            | wall         | agricultural  |       |
