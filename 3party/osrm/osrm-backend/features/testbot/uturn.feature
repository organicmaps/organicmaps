@routing @uturn @via @testbot
Feature: U-turns at via points

    Background:
        Given the profile "testbot"

    Scenario: U-turns at via points disabled by default
        Given the node map
            | a | b | c | d |
            |   | e | f | g |

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | cd    |
            | be    |
            | dg    |
            | ef    |
            | fg    |

        When I route I should get
            | waypoints | route             | turns                                          |
            | a,e,c     | ab,be,be,ef,fg,dg,cd | head,right,via,left,straight,left,left,destination |

    Scenario: Query param to allow U-turns at all via points
        Given the node map
            | a | b | c | d |
            |   | e | f | g |

        And the query options
            | uturns | true |

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | cd    |
            | be    |
            | dg    |
            | ef    |
            | fg    |

        When I route I should get
            | waypoints | route       |
            | a,e,c     | ab,be,be,bc |

    @todo
    Scenario: Instructions at via points at u-turns
        Given the node map
            | a | b | c | d |
            |   | e | f | g |

        And the query options
            | uturns | true |

        And the ways
            | nodes |
            | ab    |
            | bc    |
            | cd    |
            | be    |
            | dg    |
            | ef    |
            | fg    |

        When I route I should get
            | waypoints | route       | turns                              |
            | a,e,c     | ab,be,be,bc | head,right,uturn,right,destination |
