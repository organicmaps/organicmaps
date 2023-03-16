-- Testbot, with turn penalty
-- Used for testing turn penalties

require 'testbot'

function turn_function (angle)
    return 200*math.abs(angle)/180 -- penalty 
end
