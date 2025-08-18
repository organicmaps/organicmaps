@routing @bicycle @bridge
Feature: Bicycle - Handle movable bridge

    Background:
        Given the profile "bicycle"

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
            | a    | g  | abc,cde,efg | 1,5,1 |
            | b    | f  | abc,cde,efg | 1,5,1 |
            | e    | c  | cde         | 5     |
            | e    | b  | cde,abc     | 5,1   |
            | e    | a  | cde,abc     | 5,1   |
            | c    | e  | cde         | 5     |
            | c    | f  | cde,efg     | 5,1   |
            | c    | g  | cde,efg     | 5,1   |

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
            | a    | g  | abc,cde,efg | 1,5,1 | 5 km/h |
            | b    | f  | abc,cde,efg | 1,5,1 | 3 km/h |
            | c    | e  | cde         | 5     | 2 km/h |
            | e    | c  | cde         | 5     | 2 km/h |
