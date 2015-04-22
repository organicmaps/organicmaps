@timestamp
Feature: Timestamp

    Scenario: Request timestamp
        Given the node map
            | a | b |
        And the ways
            | nodes |
            | ab    |
        When I request /timestamp
        Then I should get a valid timestamp
