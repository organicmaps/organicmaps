@routing @foot @names
Feature: Foot - Street names in instructions

    Background:
        Given the profile "foot"

    Scenario: Foot - A named street
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
    Scenario: Foot - Use way type to describe unnamed ways
        Given the node map
            | a | b | c | d |

        And the ways
            | nodes | highway | name |
            | ab    | footway |      |
            | bcd   | track   |      |

        When I route I should get
            | from | to | route                             |
            | a    | d  | {highway:footway},{highway:track} |
