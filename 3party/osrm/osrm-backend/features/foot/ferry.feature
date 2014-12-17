@routing @foot @ferry
Feature: Foot - Handle ferry routes

    Background:
        Given the profile "foot"

    Scenario: Foot - Ferry route
        Given the node map
            | a | b | c |   |   |
            |   |   | d |   |   |
            |   |   | e | f | g |

        And the ways
            | nodes | highway | route | foot |
            | abc   | primary |       |      |
            | cde   |         | ferry | yes  |
            | efg   | primary |       |      |

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

    Scenario: Foot - Ferry duration, single node
        Given the node map
            | a | b | c | d |
            |   |   | e | f |
            |   |   | g | h |
            |   |   | i | j |

        And the ways
            | nodes | highway | route | foot | duration |
            | ab    | primary |       |      |          |
            | cd    | primary |       |      |          |
            | ef    | primary |       |      |          |
            | gh    | primary |       |      |          |
            | ij    | primary |       |      |          |
            | bc    |         | ferry | yes  | 0:01     |
            | be    |         | ferry | yes  | 0:10     |
            | bg    |         | ferry | yes  | 1:00     |
            | bi    |         | ferry | yes  | 10:00    |

    Scenario: Foot - Ferry duration, multiple nodes
        Given the node map
            | x |   |   |   |   | y |
            |   | a | b | c | d |   |

        And the ways
            | nodes | highway | route | foot | duration |
            | xa    | primary |       |      |          |
            | yd    | primary |       |      |          |
            | abcd  |         | ferry | yes  | 1:00     |

        When I route I should get
            | from | to | route | time       |
            | a    | d  | abcd  | 3600s +-10 |
            | d    | a  | abcd  | 3600s +-10 |
