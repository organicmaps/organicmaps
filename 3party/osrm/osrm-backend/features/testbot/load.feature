@routing @load @testbot
Feature: Ways of loading data
# Several scenarios that change between direct/datastore makes
# it easier to check that the test framework behaves as expected.

    Background:
        Given the profile "testbot"

    Scenario: Load data with datastore - ab
        Given data is loaded with datastore
        Given the node map
            | a | b |

        And the ways
            | nodes |
            | ab    |

        When I route I should get
            | from | to | route |
            | a    | b  | ab    |
            | b    | a  | ab    |

    Scenario: Load data directly - st
        Given data is loaded directly
        Given the node map
            | s | t |

        And the ways
            | nodes |
            | st    |

        When I route I should get
            | from | to | route |
            | s    | t  | st    |
            | t    | s  | st    |

    Scenario: Load data datstore - xy
        Given data is loaded with datastore
        Given the node map
            | x | y |

        And the ways
            | nodes |
            | xy    |

        When I route I should get
            | from | to | route |
            | x    | y  | xy    |
            | y    | x  | xy    |

    Scenario: Load data directly - cd
        Given data is loaded directly
        Given the node map
            | c | d |

        And the ways
            | nodes |
            | cd    |

        When I route I should get
            | from | to | route |
            | c    | d  | cd    |
            | d    | c  | cd    |
