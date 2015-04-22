@routing @car @names
Feature: Car - Street names in instructions

    Background:
        Given the profile "car"

    Scenario: Car - A named street
        Given the node map
            | a | b |
            |   | c |

        And the ways
            | nodes | name     |
            | ab    | My Way   |
            | bc    | Your Way |

        When I route I should get
            | from | to | route           |
            | a    | c  | My Way,Your Way |

    @todo
    Scenario: Car - Use way type to describe unnamed ways
        Given the node map
            | a | b | c | d |

        And the ways
            | nodes | highway     | name |
            | ab    | tertiary    |      |
            | bcd   | residential |      |

        When I route I should get
            | from | to | route                |
            | a    | c  | tertiary,residential |
