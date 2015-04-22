@routing @car @bridge
Feature: Car - Handle movable bridge

    Background:
        Given the profile "car"

    Scenario: Car - Use a ferry route
        Given the node map
            | a | b | c |   |   |
            |   |   | d |   |   |
            |   |   | e | f | g |

        And the ways
            | nodes | highway | bridge  | bicycle |
            | abc   | primary |         |         |
            | cde   |         | movable | yes     |
            | efg   | primary |         |         |

        When I route I should get
            | from | to | route       | modes |
            | a    | g  | abc,cde,efg | 1,3,1 |
            | b    | f  | abc,cde,efg | 1,3,1 |
            | e    | c  | cde         | 3     |
            | e    | b  | cde,abc     | 3,1   |
            | e    | a  | cde,abc     | 3,1   |
            | c    | e  | cde         | 3     |
            | c    | f  | cde,efg     | 3,1   |
            | c    | g  | cde,efg     | 3,1   |

    Scenario: Car - Properly handle durations
        Given the node map
            | a | b | c |   |   |
            |   |   | d |   |   |
            |   |   | e | f | g |

        And the ways
            | nodes | highway | bridge  | duration |
            | abc   | primary |         |          |
            | cde   |         | movable | 00:05:00 |
            | efg   | primary |         |          |

        When I route I should get
            | from | to | route       | modes | speed   |
            | a    | g  | abc,cde,efg | 1,3,1 | 6 km/h |
            | b    | f  | abc,cde,efg | 1,3,1 | 4 km/h |
            | c    | e  | cde         | 3     | 2 km/h |
            | e    | c  | cde         | 3     | 2 km/h |
