@routing @bicycle @stop_area @todo
Feature: Bike - Stop areas for public transport
# Platforms and railway/bus lines are connected using a relation rather that a way, as specified in:
# http://wiki.openstreetmap.org/wiki/Tag:public_transport%3Dstop_area

    Background:
        Given the profile "bicycle"

    Scenario: Bike - Platforms tagged using public_transport
        Then routability should be
            | highway | public_transport | bicycle | bothw |
            | primary |                  |         | x     |
            | (nil)   | platform         |         | x     |

    Scenario: Bike - railway platforms
        Given the node map
            | a | b | c | d |
            |   | s | t |   |

        And the nodes
            | node | public_transport |
            | c    | stop_position    |

        And the ways
            | nodes | highway | railway | bicycle | public_transport |
            | abcd  | (nil)   | train   | yes     |                  |
            | st    | (nil)   | (nil)   |         | platform         |

        And the relations
            | type             | public_transport | node:stop | way:platform |
            | public_transport | stop_area        | c         | st           |

        When I route I should get
            | from | to | route        |
            | a    | d  | abcd         |
            | s    | t  | st           |
            | s    | d  | /st,.+,abcd/ |
