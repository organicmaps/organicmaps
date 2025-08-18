@nearest
Feature: Locating Nearest node on a Way - basic projection onto way

    Background:
        Given the profile "testbot"

    Scenario: Nearest - easy-west way
        Given the node map
            | 0 | 1 | 2 | 3 | 4 |
            |   | a | x | b |   |
            | 5 | 6 | 7 | 8 | 9 |

        And the ways
            | nodes |
            | ab    |

        When I request nearest I should get
            | in | out |
            | 0  | a   |
            | 1  | a   |
            | 2  | x   |
            | 3  | b   |
            | 4  | b   |
            | 5  | a   |
            | 6  | a   |
            | 7  | x   |
            | 8  | b   |
            | 9  | b   |

    Scenario: Nearest - north-south way
        Given the node map
            | 0 |   | 5 |
            | 1 | a | 6 |
            | 2 | x | 7 |
            | 3 | b | 8 |
            | 4 |   | 9 |

        And the ways
            | nodes |
            | ab    |

        When I request nearest I should get
            | in | out |
            | 0  | a   |
            | 1  | a   |
            | 2  | x   |
            | 3  | b   |
            | 4  | b   |
            | 5  | a   |
            | 6  | a   |
            | 7  | x   |
            | 8  | b   |
            | 9  | b   |

    Scenario: Nearest - diagonal 1
        Given the node map
            | 8 |   | 4 |   |   |   |
            |   | a |   | 5 |   |   |
            | 0 |   | x |   | 6 |   |
            |   | 1 |   | y |   | 7 |
            |   |   | 2 |   | b |   |
            |   |   |   | 3 |   | 9 |

        And the ways
            | nodes |
            | ab    |

        When I request nearest I should get
            | in | out |
            | 0  | a   |
            | 1  | x   |
            | 2  | y   |
            | 3  | b   |
            | 4  | a   |
            | 5  | x   |
            | 6  | y   |
            | 7  | b   |
            | 8  | a   |
            | 9  | b   |

    Scenario: Nearest - diagonal 2
        Given the node map
            |   |   |   | 3 |   | 9 |
            |   |   | 2 |   | b |   |
            |   | 1 |   | y |   | 7 |
            | 0 |   | x |   | 6 |   |
            |   | a |   | 5 |   |   |
            | 8 |   | 4 |   |   |   |

        And the ways
            | nodes |
            | ab    |

        When I request nearest I should get
            | in | out |
            | 0  | a   |
            | 1  | x   |
            | 2  | y   |
            | 3  | b   |
            | 4  | a   |
            | 5  | x   |
            | 6  | y   |
            | 7  | b   |
            | 8  | a   |
            | 9  | b   |
