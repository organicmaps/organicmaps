@routing @foot @maxspeed
Feature: Foot - Ignore max speed restrictions

Background: Use specific speeds
    Given the profile "foot"

    Scenario: Foot - Ignore maxspeed
        Then routability should be
            | highway     | maxspeed  | bothw     |
            | residential |           | 145s ~10% |
            | residential | 1         | 145s ~10% |
            | residential | 100       | 145s ~10% |
            | residential | 1         | 145s ~10% |
            | residential | 1mph      | 145s ~10% |
            | residential | 1 mph     | 145s ~10% |
            | residential | 1unknown  | 145s ~10% |
            | residential | 1 unknown | 145s ~10% |
            | residential | none      | 145s ~10% |
            | residential | signals   | 145s ~10% |
