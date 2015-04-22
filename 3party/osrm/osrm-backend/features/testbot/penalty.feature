@routing @penalty @signal @testbot
Feature: Penalties
# Testbot uses a signal penalty of 7s.

    Background:
        Given the profile "testbot"

    Scenario: Traffic signals should incur a delay, without changing distance
        Given the node map
            | a | b | c |
            | d | e | f |

        And the nodes
            | node | highway         |
            | e    | traffic_signals |

        And the ways
            | nodes |
            | abc   |
            | def   |

        When I route I should get
            | from | to | route | time    | distance |
            | a    | c  | abc   | 20s +-1 | 200m +-1 |
            | d    | f  | def   | 27s +-1 | 200m +-1 |

    Scenario: Signal penalty should not depend on way type
        Given the node map
            | a | b | c |
            | d | e | f |
            | g | h | i |

        And the nodes
            | node | highway         |
            | b    | traffic_signals |
            | e    | traffic_signals |
            | h    | traffic_signals |

        And the ways
            | nodes | highway   |
            | abc   | primary   |
            | def   | secondary |
            | ghi   | tertiary  |

        When I route I should get
            | from | to | route | time    |
            | a    | c  | abc   | 27s +-1 |
            | d    | f  | def   | 47s +-1 |
            | g    | i  | ghi   | 67s +-1 |

    Scenario: Passing multiple traffic signals should incur a accumulated delay
        Given the node map
            | a | b | c | d | e |

        And the nodes
            | node | highway         |
            | b    | traffic_signals |
            | c    | traffic_signals |
            | d    | traffic_signals |

        And the ways
            | nodes |
            | abcde |

        When I route I should get
            | from | to | route | time    |
            | a    | e  | abcde | 61s +-1 |

        @todo
        Scenario: Signal penalty should not depend on way type
            Given the node map
                | a | b | c |
                | d | e | f |
                | g | h | i |

            And the nodes
                | node | highway         |
                | b    | traffic_signals |
                | e    | traffic_signals |
                | h    | traffic_signals |

            And the ways
                | nodes | highway   |
                | abc   | primary   |
                | def   | secondary |
                | ghi   | tertiary  |

            When I route I should get
                | from | to | route | time    |
                | a    | b  | abc   | 10s +-1 |
                | a    | c  | abc   | 27s +-1 |
                | d    | e  | def   | 20s +-1 |
                | d    | f  | def   | 47s +-1 |
                | g    | h  | ghi   | 30s +-1 |
                | g    | i  | ghi   | 67s +-1 |

        Scenario: Passing multiple traffic signals should incur a accumulated delay
            Given the node map
                | a | b | c | d | e |

            And the nodes
                | node | highway         |
                | b    | traffic_signals |
                | c    | traffic_signals |
                | d    | traffic_signals |

            And the ways
                | nodes |
                | abcde |

            When I route I should get
                | from | to | route | time    |
                | a    | e  | abcde | 61s +-1 |

    @todo
    Scenario: Starting or ending at a traffic signal should not incur a delay
        Given the node map
            | a | b | c |

        And the nodes
            | node | highway         |
            | b    | traffic_signals |

        And the ways
            | nodes |
            | abc   |

        When I route I should get
            | from | to | route | time    |
            | a    | b  | abc   | 10s +-1 |
            | b    | a  | abc   | 10s +-1 |

    Scenario: Routing between signals on the same way should not incur a delay
        Given the node map
            | a | b | c | d |

        And the nodes
            | node | highway         |
            | a    | traffic_signals |
            | d    | traffic_signals |

        And the ways
            | nodes | highway |
            | abcd  | primary |

        When I route I should get
            | from | to | route | time    |
            | b    | c  | abcd  | 10s +-1 |
            | c    | b  | abcd  | 10s +-1 |

    Scenario: Prefer faster route without traffic signals
        Given a grid size of 50 meters
        And the node map
            | a |  | b |  | c |
            |   |  | d |  |   |

        And the nodes
            | node | highway         |
            | b    | traffic_signals |

        And the ways
            | nodes | highway |
            | abc   | primary |
            | adc   | primary |

        When I route I should get
            | from | to | route |
            | a    | c  | adc   |
