@routing @testbot @roundabout @instruction
Feature: Roundabout Instructions

    Background:
        Given the profile "testbot"

    Scenario: Testbot - Roundabout
        Given the node map
            |   |   | v |   |   |
            |   |   | d |   |   |
            | s | a |   | c | u |
            |   |   | b |   |   |
            |   |   | t |   |   |

        And the ways
            | nodes | junction   |
            | sa    |            |
            | tb    |            |
            | uc    |            |
            | vd    |            |
            | abcda | roundabout |

        When I route I should get
            | from | to | route | turns                               |
            | s    | t  | sa,tb | head,enter_roundabout-1,destination |
            | s    | u  | sa,uc | head,enter_roundabout-2,destination |
            | s    | v  | sa,vd | head,enter_roundabout-3,destination |
            | t    | u  | tb,uc | head,enter_roundabout-1,destination |
            | t    | v  | tb,vd | head,enter_roundabout-2,destination |
            | t    | s  | tb,sa | head,enter_roundabout-3,destination |
            | u    | v  | uc,vd | head,enter_roundabout-1,destination |
            | u    | s  | uc,sa | head,enter_roundabout-2,destination |
            | u    | t  | uc,tb | head,enter_roundabout-3,destination |
            | v    | s  | vd,sa | head,enter_roundabout-1,destination |
            | v    | t  | vd,tb | head,enter_roundabout-2,destination |
            | v    | u  | vd,uc | head,enter_roundabout-3,destination |

    Scenario: Testbot - Roundabout with oneway links
        Given the node map
            |   |   | p | o |   |   |
            |   |   | h | g |   |   |
            | i | a |   |   | f | n |
            | j | b |   |   | e | m |
            |   |   | c | d |   |   |
            |   |   | k | l |   |   |

        And the ways
            | nodes     | junction   | oneway |
            | ai        |            | yes    |
            | jb        |            | yes    |
            | ck        |            | yes    |
            | ld        |            | yes    |
            | em        |            | yes    |
            | nf        |            | yes    |
            | go        |            | yes    |
            | ph        |            | yes    |
            | abcdefgha | roundabout |        |

        When I route I should get
            | from | to | route | turns                               |
            | j    | k  | jb,ck | head,enter_roundabout-1,destination |
            | j    | m  | jb,em | head,enter_roundabout-2,destination |
            | j    | o  | jb,go | head,enter_roundabout-3,destination |
            | j    | i  | jb,ai | head,enter_roundabout-4,destination |
            | l    | m  | ld,em | head,enter_roundabout-1,destination |
            | l    | o  | ld,go | head,enter_roundabout-2,destination |
            | l    | i  | ld,ai | head,enter_roundabout-3,destination |
            | l    | k  | ld,ck | head,enter_roundabout-4,destination |
            | n    | o  | nf,go | head,enter_roundabout-1,destination |
            | n    | i  | nf,ai | head,enter_roundabout-2,destination |
            | n    | k  | nf,ck | head,enter_roundabout-3,destination |
            | n    | m  | nf,em | head,enter_roundabout-4,destination |
            | p    | i  | ph,ai | head,enter_roundabout-1,destination |
            | p    | k  | ph,ck | head,enter_roundabout-2,destination |
            | p    | m  | ph,em | head,enter_roundabout-3,destination |
            | p    | o  | ph,go | head,enter_roundabout-4,destination |
