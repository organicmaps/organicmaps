@routing @origin @testbot
Feature: Routing close to the [0,0] origin

    Background:
        Given the profile "testbot"

    Scenario: East-west oneways close to the origin
        Given the node locations
            | node | lat                   | lon |
            | a    | 0                     | 0   |
            | b    | 0.0008990679362704611 | 0   |
            | c    | 0.0017981358725409223 | 0   |
            | d    | 0.0026972038088113833 | 0   |

        And the ways
            | nodes | oneway |
            | abcd  | yes    |

        When I route I should get
            | from | to | route | distance |
            | b    | c  | abcd  | 100m +-1 |
            | c    | b  |       |          |

    Scenario: North-south oneways close to the origin
        Given the node locations
            | node | lat | lon                   |
            | a    | 0   | 0                     |
            | b    | 0   | 0.0008990679362704611 |
            | c    | 0   | 0.0017981358725409223 |
            | d    | 0   | 0.0026972038088113833 |

        And the ways
            | nodes | oneway |
            | abcd  | yes    |

        When I route I should get
            | from | to | route | distance |
            | b    | c  | abcd  | 100m +-1 |
            | c    | b  |       |          |

    Scenario: East-west oneways crossing the origin
        Given the node locations
            | node | lat                    | lon |
            | a    | -0.0017981358725409223 | 0   |
            | b    | -0.0008990679362704611 | 0   |
            | c    | 0                      | 0   |
            | d    | 0.0008990679362704611  | 0   |
            | e    | 0.0017981358725409223  | 0   |

        And the ways
            | nodes | oneway |
            | abcde | yes    |

        When I route I should get
            | from | to | route | distance |
            | b    | d  | abcde | 200m +-2 |
            | d    | b  |       |          |

    Scenario: North-south oneways crossing the origin
        Given the node locations
            | node | lat | lon                    |
            | a    | 0   | -0.0017981358725409223 |
            | b    | 0   | -0.0008990679362704611 |
            | c    | 0   | 0                      |
            | d    | 0   | 0.0008990679362704611  |
            | e    | 0   | 0.0017981358725409223  |

        And the ways
            | nodes | oneway |
            | abcde | yes    |

        When I route I should get
            | from | to | route | distance |
            | b    | d  | abcde | 200m +-2 |
            | d    | b  |       |          |
