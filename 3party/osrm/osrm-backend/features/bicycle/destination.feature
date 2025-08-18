@routing @bicycle @destination @todo
Feature: Bike - Destination only, no passing through

    Background:
        Given the profile "bicycle"

    Scenario: Bike - Destination only street
        Given the node map
            | a |   |   |   | e |
            |   | b | c | d |   |
            |   |   |   |   |   |
            | x |   |   |   | y |

        And the ways
            | nodes | access      |
            | ab    |             |
            | bcd   | destination |
            | de    |             |
            | axye  |             |

        When I route I should get
            | from | to | route  |
            | a    | b  | ab     |
            | a    | c  | ab,bcd |
            | a    | d  | ab,bcd |
            | a    | e  | axye   |
            | e    | d  | de     |
            | e    | c  | de,bcd |
            | e    | b  | de,bcd |
            | e    | a  | axye   |

    Scenario: Bike - Destination only street
        Given the node map
            | a |   |   |   | e |
            |   | b | c | d |   |
            |   |   |   |   |   |
            | x |   |   |   | y |

        And the ways
            | nodes | access      |
            | ab    |             |
            | bc    | destination |
            | cd    | destination |
            | de    |             |
            | axye  |             |

        When I route I should get
            | from | to | route    |
            | a    | b  | ab       |
            | a    | c  | ab,bc    |
            | a    | d  | ab,bc,cd |
            | a    | e  | axye     |
            | e    | d  | de       |
            | e    | c  | de,dc    |
            | e    | b  | de,dc,bc |
            | e    | a  | axye     |

    Scenario: Bike - Routing inside a destination only area
        Given the node map
            | a |   | c |   | e |
            |   | b |   | d |   |
            | x |   |   |   | y |

        And the ways
            | nodes | access      |
            | ab    | destination |
            | bc    | destination |
            | cd    | destination |
            | de    | destination |
            | axye  |             |

        When I route I should get
            | from | to | route       |
            | a    | e  | ab,bc,cd,de |
            | e    | a  | de,cd,bc,ab |
            | b    | d  | bc,cd       |
            | d    | b  | cd,bc       |
