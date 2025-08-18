@routing @link @car
Feature: Car - Speed on links
# Check that there's a reasonable ratio between the
# speed of a way and it's corresponding link type.

    Background: Use specific speeds
        Given the profile "car"
        Given a grid size of 1000 meters

    Scenario: Car - Use motorway_link when reasonable
        Given the node map
            |   |   | e |   |   |   | f |   |   |
            | x | a | b |   |   |   | c | d | y |

        And the ways
            | nodes | highway       |
            | xa    | unclassified  |
            | ab    | motorway_link |
            | bc    | motorway_link |
            | cd    | motorway_link |
            | ae    | motorway      |
            | ef    | motorway      |
            | fd    | motorway      |
            | dy    | unclassified  |

        When I route I should get
            | from | to | route          |
            | x    | y  | xa,ae,ef,fd,dy |
            | b    | c  | bc             |

    Scenario: Car - Use trunk_link when reasonable
        Given the node map
            |   |   | e |   |   |   | f |   |   |
            | x | a | b |   |   |   | c | d | y |

        And the ways
            | nodes | highway      |
            | xa    | unclassified |
            | ab    | trunk_link   |
            | bc    | trunk_link   |
            | cd    | trunk_link   |
            | ae    | trunk        |
            | ef    | trunk        |
            | fd    | trunk        |
            | dy    | unclassified |
        When I route I should get
            | from | to | route          |
            | x    | y  | xa,ae,ef,fd,dy |
            | b    | c  | bc             |

    Scenario: Car - Use primary_link when reasonable
        Given the node map
            |   |   | e |   |   |   | f |   |   |
            | x | a | b |   |   |   | c | d | y |

        And the ways
            | nodes | highway        |
            | xa    | unclassified   |
            | ab    | primary_link   |
            | bc    | primary_link   |
            | cd    | primary_link   |
            | ae    | primary        |
            | ef    | primary        |
            | fd    | primary        |
            | dy    | unclassified |
        When I route I should get
            | from | to | route          |
            | x    | y  | xa,ae,ef,fd,dy |
            | b    | c  | bc             |

    Scenario: Car - Use secondary_link when reasonable
        Given the node map
            |   |   | e |   |   |   | f |   |   |
            | x | a | b |   |   |   | c | d | y |

        And the ways
            | nodes | highway          |
            | xa    | unclassified     |
            | ab    | secondary_link   |
            | bc    | secondary_link   |
            | cd    | secondary_link   |
            | ae    | secondary        |
            | ef    | secondary        |
            | fd    | secondary        |
            | dy    | unclassified     |

        When I route I should get
            | from | to | route          |
            | x    | y  | xa,ae,ef,fd,dy |
            | b    | c  | bc             |

    Scenario: Car - Use tertiary_link when reasonable
        Given the node map
            |   |   | e |   |   |   | f |   |   |
            | x | a | b |   |   |   | c | d | y |

        And the ways
            | nodes | highway         |
            | xa    | unclassified    |
            | ab    | tertiary_link   |
            | bc    | tertiary_link   |
            | cd    | tertiary_link   |
            | ae    | tertiary        |
            | ef    | tertiary        |
            | fd    | tertiary        |
            | dy    | unclassified    |

        When I route I should get
            | from | to | route          |
            | x    | y  | xa,ae,ef,fd,dy |
            | b    | c  | bc             |
