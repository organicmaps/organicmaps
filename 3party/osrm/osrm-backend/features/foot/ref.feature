@routing @foot @ref @name
Feature: Foot - Way ref

    Background:
        Given the profile "foot"

    Scenario: Foot - Way with both name and ref
        Given the node map
            | a | b |

        And the ways
            | nodes | name         | ref |
            | ab    | Utopia Drive | E7  |

        When I route I should get
            | from | to | route             |
            | a    | b  | Utopia Drive / E7 |

    Scenario: Foot - Way with only ref
        Given the node map
            | a | b |

        And the ways
            | nodes | name | ref |
            | ab    |      | E7  |

        When I route I should get
            | from | to | route |
            | a    | b  | E7    |

    Scenario: Foot - Way with only name
        Given the node map
            | a | b |

        And the ways
            | nodes | name         |
            | ab    | Utopia Drive |

        When I route I should get
            | from | to | route        |
            | a    | b  | Utopia Drive |
