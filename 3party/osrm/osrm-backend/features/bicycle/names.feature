@routing @bicycle @names
Feature: Bike - Street names in instructions

    Background:
        Given the profile "bicycle"

    Scenario: Bike - A named street
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

    @unnamed
    Scenario: Bike - Use way type to describe unnamed ways
        Given the node map
            | a | b | c | d |

        And the ways
            | nodes | highway  | name |
            | ab    | cycleway |      |
            | bcd   | track    |      |

        When I route I should get
            | from | to | route                              |
            | a    | d  | {highway:cycleway},{highway:track} |
