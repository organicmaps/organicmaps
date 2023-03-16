@routing @bicycle @ref @name
Feature: Bike - Way ref

    Background:
        Given the profile "bicycle"

    Scenario: Bike - Way with both name and ref
        Given the node map
            | a | b |

        And the ways
            | nodes | name         | ref |
            | ab    | Utopia Drive | E7  |

        When I route I should get
            | from | to | route             |
            | a    | b  | Utopia Drive / E7 |

    Scenario: Bike - Way with only ref
        Given the node map
            | a | b |

        And the ways
            | nodes | name | ref |
            | ab    |      | E7  |

        When I route I should get
            | from | to | route |
            | a    | b  | E7    |

    Scenario: Bike - Way with only name
        Given the node map
            | a | b |

        And the ways
            | nodes | name         |
            | ab    | Utopia Drive |

        When I route I should get
            | from | to | route        |
            | a    | b  | Utopia Drive |
