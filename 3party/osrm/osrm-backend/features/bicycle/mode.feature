@routing @bicycle @mode
Feature: Bike - Mode flag

# bicycle modes:
# 1 bike
# 2 pushing
# 3 ferry
# 4 train

	Background:
		Given the profile "bicycle"
    
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
    	 | from | to | route    | turns                       | modes |
    	 | a    | d  | ab,bc,cd | head,right,left,destination | 1,3,1 |
    	 | d    | a  | cd,bc,ab | head,right,left,destination | 1,3,1 |
    	 | c    | a  | bc,ab    | head,left,destination       | 3,1   |
    	 | d    | b  | cd,bc    | head,right,destination      | 1,3   |
    	 | a    | c  | ab,bc    | head,right,destination      | 1,3   |
    	 | b    | d  | bc,cd    | head,left,destination       | 3,1   |

     Scenario: Bike - Mode when using a train
     	Given the node map
     	 | a | b |   |
     	 |   | c | d |

     	And the ways
     	 | nodes | highway | railway | bicycle |
     	 | ab    | primary |         |         |
     	 | bc    |         | train   | yes     |
     	 | cd    | primary |         |         |

     	When I route I should get
     	 | from | to | route    | turns                       | modes |
     	 | a    | d  | ab,bc,cd | head,right,left,destination | 1,4,1 |
     	 | d    | a  | cd,bc,ab | head,right,left,destination | 1,4,1 |
     	 | c    | a  | bc,ab    | head,left,destination       | 4,1   |
     	 | d    | b  | cd,bc    | head,right,destination      | 1,4   |
     	 | a    | c  | ab,bc    | head,right,destination      | 1,4   |
     	 | b    | d  | bc,cd    | head,left,destination       | 4,1   |

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
     	 | from | to | route    | turns                       | modes |
     	 | a    | d  | ab,bc,cd | head,right,left,destination | 1,1,1 |
     	 | d    | a  | cd,bc,ab | head,right,left,destination | 1,2,1 |
     	 | c    | a  | bc,ab    | head,left,destination       | 2,1   |
     	 | d    | b  | cd,bc    | head,right,destination      | 1,2   |
     	 | a    | c  | ab,bc    | head,right,destination      | 1,1   |
     	 | b    | d  | bc,cd    | head,left,destination       | 1,1   |

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
     	 | from | to | route    | turns                       | modes |
     	 | a    | d  | ab,bc,cd | head,right,left,destination | 1,2,1 |
     	 | d    | a  | cd,bc,ab | head,right,left,destination | 1,2,1 |
     	 | c    | a  | bc,ab    | head,left,destination       | 2,1   |
     	 | d    | b  | cd,bc    | head,right,destination      | 1,2   |
     	 | a    | c  | ab,bc    | head,right,destination      | 1,2   |
     	 | b    | d  | bc,cd    | head,left,destination       | 2,1   |

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
     	 | from | to | route     | modes |
     	 | a    | f  | ab,bcd,df | 1,2,1 |
     	 | f    | a  | df,bcd,ab | 1,2,1 |
     	 | d    | a  | bcd,ab    | 2,1   |
     	 | f    | b  | df,bcd    | 1,2   |
     	 | a    | d  | ab,bcd    | 1,2   |
     	 | b    | f  | bcd,df    | 2,1   |

     Scenario: Bike - Mode when pushing on steps
     	Given the node map
     	 | a | b |   |   |
     	 |   | c | d | f |

     	And the ways
    	 | nodes | highway |
    	 | ab    | primary |
    	 | bc    | steps   |
    	 | cd    | primary |

     	When I route I should get
    	 | from | to | route    | turns                       | modes |
    	 | a    | d  | ab,bc,cd | head,right,left,destination | 1,2,1 |
    	 | d    | a  | cd,bc,ab | head,right,left,destination | 1,2,1 |
    	 | c    | a  | bc,ab    | head,left,destination       | 2,1   |
    	 | d    | b  | cd,bc    | head,right,destination      | 1,2   |
    	 | a    | c  | ab,bc    | head,right,destination      | 1,2   |
    	 | b    | d  | bc,cd    | head,left,destination       | 2,1   |

     Scenario: Bike - Mode when bicycle=dismount
     	Given the node map
     	 | a | b |   |   |
     	 |   | c | d | f |

     	And the ways
    	 | nodes | highway | bicycle  |
    	 | ab    | primary |          |
    	 | bc    | primary | dismount |
    	 | cd    | primary |          |

     	When I route I should get
    	 | from | to | route    | turns                       | modes |
    	 | a    | d  | ab,bc,cd | head,right,left,destination | 1,2,1 |
    	 | d    | a  | cd,bc,ab | head,right,left,destination | 1,2,1 |
    	 | c    | a  | bc,ab    | head,left,destination       | 2,1   |
    	 | d    | b  | cd,bc    | head,right,destination      | 1,2   |
    	 | a    | c  | ab,bc    | head,right,destination      | 1,2   |
         | b    | d  | bc,cd    | head,left,destination       | 2,1   |

    Scenario: Bicycle - Modes when starting on forward oneway
        Given the node map
         | a | b |

        And the ways
         | nodes | oneway |
         | ab    | yes    |

        When I route I should get
         | from | to | route | modes |
         | a    | b  | ab    | 1     |
         | b    | a  | ab    | 2     |

    Scenario: Bicycle - Modes when starting on reverse oneway
        Given the node map
         | a | b |

        And the ways
         | nodes | oneway |
         | ab    | -1     |

        When I route I should get
         | from | to | route | modes |
         | a    | b  | ab    | 2     |
         | b    | a  | ab    | 1     |
