@routing @car @ferry
Feature: Car - Handle ferry routes

    Background:
        Given the profile "car"

    Scenario: Car - Use a ferry route
        Given the node map
            | a | b | c |   |   |
            |   |   | d |   |   |
            |   |   | e | f | g |

        And the ways
            | nodes | highway | route | bicycle |
            | abc   | primary |       |         |
            | cde   |         | ferry | yes     |
            | efg   | primary |       |         |

        When I route I should get
            | from | to | route       | modes |
            | a    | g  | abc,cde,efg | 1,2,1 |
            | b    | f  | abc,cde,efg | 1,2,1 |
            | e    | c  | cde         | 2     |
            | e    | b  | cde,abc     | 2,1   |
            | e    | a  | cde,abc     | 2,1   |
            | c    | e  | cde         | 2     |
            | c    | f  | cde,efg     | 2,1   |
            | c    | g  | cde,efg     | 2,1   |

    Scenario: Car - Properly handle simple durations
        Given the node map
            | a | b | c |   |   |
            |   |   | d |   |   |
            |   |   | e | f | g |

        And the ways
            | nodes | highway | route | duration |
            | abc   | primary |       |          |
            | cde   |         | ferry | 00:01:00 |
            | efg   | primary |       |          |

        When I route I should get
            | from | to | route       | modes | speed   |
            | a    | g  | abc,cde,efg | 1,2,1 | 26 km/h |
            | b    | f  | abc,cde,efg | 1,2,1 | 20 km/h |
            | c    | e  | cde         | 2     | 12 km/h |
            | e    | c  | cde         | 2     | 12 km/h |

    Scenario: Car - Properly handle ISO 8601 durations
        Given the node map
            | a | b | c |   |   |
            |   |   | d |   |   |
            |   |   | e | f | g |

        And the ways
            | nodes | highway | route | duration |
            | abc   | primary |       |          |
            | cde   |         | ferry | PT1M     |
            | efg   | primary |       |          |

        When I route I should get
            | from | to | route       | modes | speed   |
            | a    | g  | abc,cde,efg | 1,2,1 | 26 km/h |
            | b    | f  | abc,cde,efg | 1,2,1 | 20 km/h |
            | c    | e  | cde         | 2     | 12 km/h |
            | e    | c  | cde         | 2     | 12 km/h |
