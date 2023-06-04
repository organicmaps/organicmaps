@routing @graph @testbot
Feature: Basic Routing
#Test the input data descibed on https://github.com/DennisOSRM/Project-OSRM/wiki/Graph-representation

    Background:
        Given the profile "testbot"

    Scenario: Graph transformation
        Given the node map
            |   |   | d |
            | a | b | c |
            |   |   | e |

        And the ways
            | nodes |
            | abc   |
            | dce   |

        When I route I should get
            | from | to | route   |
            | a    | e  | abc,dce |

    Scenario: Turn instructions on compressed road network geometry
        Given the node map
            | x | a |   |   |
            |   | b |   |   |
            | f |   |   | e |
            |   |   |   |   |
            |   |   |   |   |
            | y | c |   | d |

        And the ways
            | nodes  | name  |
            | xa     | first |
            | abcdef | compr |
            | fy     | last  |

        When I route I should get
            | from | to | route            | turns                       |
            | x    | y  | first,compr,last | head,right,left,destination |
