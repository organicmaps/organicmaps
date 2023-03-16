@routing @foot @area
Feature: Foot - Squares and other areas

    Background:
        Given the profile "foot"

    @square
    Scenario: Foot - Route along edge of a squares
        Given the node map
            | x |   |
            | a | b |
            | d | c |

        And the ways
            | nodes | area | highway     |
            | xa    |      | primary     |
            | abcda | yes  | residential |

        When I route I should get
            | from | to | route |
            | a    | b  | abcda |
            | a    | d  | abcda |
            | b    | c  | abcda |
            | c    | b  | abcda |
            | c    | d  | abcda |
            | d    | c  | abcda |
            | d    | a  | abcda |
            | a    | d  | abcda |

    @building
    Scenario: Foot - Don't route on buildings
        Given the node map
            | x |   |
            | a | b |
            | d | c |

        And the ways
            | nodes | highway | area | building | access |
            | xa    | primary |      |          |        |
            | abcda | (nil)   | yes  | yes      | yes    |

        When I route I should get
            | from | to | route |
            | a    | b  | xa    |
            | a    | d  | xa    |
            | b    | c  | xa    |
            | c    | b  | xa    |
            | c    | d  | xa    |
            | d    | c  | xa    |
            | d    | a  | xa    |
            | a    | d  | xa    |

    @parking
    Scenario: Foot - parking areas
        Given the node map
            | e |   |   | f |
            | x | a | b | y |
            |   | d | c |   |

        And the ways
            | nodes | highway | amenity |
            | xa    | primary |         |
            | by    | primary |         |
            | xefy  | primary |         |
            | abcda | (nil)   | parking |

        When I route I should get
            | from | to | route       |
            | x    | y  | xa,abcda,by |
            | y    | x  | by,abcda,xa |
            | a    | b  | abcda       |
            | a    | d  | abcda       |
            | b    | c  | abcda       |
            | c    | b  | abcda       |
            | c    | d  | abcda       |
            | d    | c  | abcda       |
            | d    | a  | abcda       |
            | a    | d  | abcda       |

    @train @platform
    Scenario: Foot - railway platforms
        Given the node map
            | x | a | b | y |
            |   | d | c |   |

        And the ways
            | nodes | highway | railway  |
            | xa    | primary |          |
            | by    | primary |          |
            | abcda | (nil)   | platform |

        When I route I should get
            | from | to | route       |
            | x    | y  | xa,abcda,by |
            | y    | x  | by,abcda,xa |
            | a    | b  | abcda       |
            | a    | d  | abcda       |
            | b    | c  | abcda       |
            | c    | b  | abcda       |
            | c    | d  | abcda       |
            | d    | c  | abcda       |
            | d    | a  | abcda       |
            | a    | d  | abcda       |
