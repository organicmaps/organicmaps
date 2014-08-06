@routing @graph
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
