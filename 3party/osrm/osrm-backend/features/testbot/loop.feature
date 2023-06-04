@routing @726 @testbot
Feature: Avoid weird loops caused by rounding errors

    Background:
        Given the profile "testbot"

    Scenario: Avoid weird loops 1
        Given the node locations
            | node | lat        | lon        |
            | a    | 55.6602463 | 12.5717242 |
            | b    | 55.6600270 | 12.5723008 |
            | c    | 55.6601840 | 12.5725037 |
            | d    | 55.6604146 | 12.5719299 |
            | e    | 55.6599410 | 12.5727592 |
            | f    | 55.6606727 | 12.5736932 |
            | g    | 55.6603422 | 12.5732619 |
            | h    | 55.6607785 | 12.5739097 |
            | i    | 55.6600566 | 12.5725070 |
            | x    | 55.6608180 | 12.5740510 |
            | y    | 55.6600730 | 12.5740670 |

        And the ways
            | nodes |
            | ab    |
            | hfgd  |
            | icd   |
            | ad    |
            | ie    |

        When I route I should get
            | from | to | route | turns            |
            | x    | y  | hfgd  | head,destination |

    Scenario: Avoid weird loops 2
        Given the node locations
            | node | lat       | lon       |
            | a    | 55.660778 | 12.573909 |
            | b    | 55.660672 | 12.573693 |
            | c    | 55.660128 | 12.572546 |
            | d    | 55.660015 | 12.572476 |
            | e    | 55.660119 | 12.572325 |
            | x    | 55.660818 | 12.574051 |
            | y    | 55.660073 | 12.574067 |

        And the ways
            | nodes |
            | abc   |
            | cdec  |

        When I route I should get
            | from | to | route | turns            |
            | x    | y  | abc   | head,destination |

    @412
    Scenario: Avoid weird loops 3
        And the node map
            | a |   |   |
            | b | e |   |
            |   |   | 1 |
            |   |   |   |
            |   |   | 2 |
            |   |   |   |
            |   | c | f |
            |   | d |   |

        And the ways
            | nodes | highway     |
            | ab    | residential |
            | bc    | residential |
            | cd    | residential |
            | be    | primary     |
            | ef    | primary     |
            | cf    | primary     |

        When I route I should get
            | waypoints | route             | turns                                      |
            | a,2,d     | ab,be,ef,ef,cf,cd | head,left,right,via,right,left,destination |
            | a,1,d     | ab,be,ef,ef,cf,cd | head,left,right,via,right,left,destination |
