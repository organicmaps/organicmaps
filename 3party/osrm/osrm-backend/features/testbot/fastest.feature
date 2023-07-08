@routing @fastest @testbot
Feature: Choosing fastest route

    Background:
        Given the profile "testbot"

    Scenario: Pick the geometrically shortest route, way types being equal
        Given the node map
            |   |   | s |   |   |
            |   |   | t |   |   |
            | x | a |   | b | y |

        And the ways
            | nodes | highway |
            | xa    | primary |
            | by    | primary |
            | atb   | primary |
            | asb   | primary |

        When I route I should get
            | from | to | route     |
            | x    | y  | xa,atb,by |
            | y    | x  | by,atb,xa |

    Scenario: Pick the fastest route, even when it's longer
        Given the node map
            |   | p |   |
            | a | s | b |

        And the ways
            | nodes | highway   |
            | apb   | primary   |
            | asb   | secondary |

        When I route I should get
            | from | to | route |
            | a    | b  | apb   |
            | b    | a  | apb   |
