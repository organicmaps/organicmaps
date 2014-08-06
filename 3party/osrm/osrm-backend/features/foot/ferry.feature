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
            | from | to | route       |
            | a    | g  | abc,cde,efg |
            | b    | f  | abc,cde,efg |
            | e    | c  | cde         |
            | e    | b  | cde,abc     |
            | e    | a  | cde,abc     |
            | c    | e  | cde         |
            | c    | f  | cde,efg     |
            | c    | g  | cde,efg     |

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
