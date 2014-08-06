@routing @bicycle @mode
Feature: Bike - Mode flag

    Background:
        Given the profile "bicycle"

    @todo
    Scenario: Bike - Mode when using a ferry
        Given the node map
            | a | b |   |
            |   | c | d |

        And the ways
            | nodes | highway | route | duration |
            | ab    | primary |       |          |
            | bc    |         | ferry | 0:01     |
            | cd    | primary |       |          |

        When I route I should get
            | from | to | route    | turns                        | modes           |
            | a    | d  | ab,bc,cd | head,right,left, destination | bike,ferry,bike |
            | d    | a  | cd,bc,ab | head,right,left, destination | bike,ferry,bike |
            | c    | a  | bc,ab    | head,left,destination        | ferry,bike      |
            | d    | b  | cd,bc    | head,right,destination       | bike,ferry      |
            | a    | c  | ab,bc    | head,right,destination       | bike,ferry      |
            | b    | d  | bc,cd    | head,left,destination        | ferry,bike      |

    @todo
    Scenario: Bike - Mode when pushing bike against oneways
        Given the node map
            | a | b |   |
            |   | c | d |

        And the ways
            | nodes | highway | oneway |
            | ab    | primary |        |
            | bc    | primary | yes    |
            | cd    | primary |        |

        When I route I should get
            | from | to | route    | turns                       | modes          |
            | a    | d  | ab,bc,cd | head,right,left,destination | bike,push,bike |
            | d    | a  | cd,bc,ab | head,right,left,destination | bike,push,bike |
            | c    | a  | bc,ab    | head,left,destination       | push,bike      |
            | d    | b  | cd,bc    | head,right,destination      | bike,push      |
            | a    | c  | ab,bc    | head,right,destination      | bike,push      |
            | b    | d  | bc,cd    | head,left,destination       | push,bike      |

    @todo
    Scenario: Bike - Mode when pushing on pedestrain streets
        Given the node map
            | a | b |   |
            |   | c | d |

        And the ways
            | nodes | highway    |
            | ab    | primary    |
            | bc    | pedestrian |
            | cd    | primary    |

        When I route I should get
            | from | to | route    | turns                       | modes          |
            | a    | d  | ab,bc,cd | head,right,left,destination | bike,push,bike |
            | d    | a  | cd,bc,ab | head,right,left,destination | bike,push,bike |
            | c    | a  | bc,ab    | head,left,destination       | push,bike      |
            | d    | b  | cd,bc    | head,right,destination      | bike,push      |
            | a    | c  | ab,bc    | head,right,destination      | bike,push      |
            | b    | d  | bc,cd    | head,left,destination       | push,bike      |

    @todo
    Scenario: Bike - Mode when pushing on pedestrain areas
        Given the node map
            | a | b |   |   |
            |   | c | d | f |

        And the ways
            | nodes | highway    | area |
            | ab    | primary    |      |
            | bcd   | pedestrian | yes  |
            | df    | primary    |      |

        When I route I should get
            | from | to | route     | modes          |
            | a    | f  | ab,bcd,df | bike,push,bike |
            | f    | a  | df,bcd,ab | bike,push,bike |
            | d    | a  | bcd,ab    | push,bike      |
            | f    | b  | df,bcd    | bike,push      |
            | a    | d  | ab,bcd    | bike,push      |
            | b    | f  | bcd,df    | push,bike      |
