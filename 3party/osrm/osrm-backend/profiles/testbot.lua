-- Testbot profile

-- Moves at fixed, well-known speeds, practical for testing speed and travel times:

-- Primary road:  36km/h = 36000m/3600s = 100m/10s
-- Secondary road:  18km/h = 18000m/3600s = 100m/20s
-- Tertiary road:  12km/h = 12000m/3600s = 100m/30s

-- modes:
-- 1: normal
-- 2: route
-- 3: river downstream
-- 4: river upstream
-- 5: steps down
-- 6: steps up

speed_profile = {
  ["primary"] = 36,
  ["secondary"] = 18,
  ["tertiary"] = 12,
  ["steps"] = 6,
  ["default"] = 24
}

-- these settings are read directly by osrm

take_minimum_of_speeds  = true
obey_oneway             = true
obey_barriers           = true
use_turn_restrictions   = true
ignore_areas            = true  -- future feature
traffic_signal_penalty  = 7     -- seconds
u_turn_penalty          = 20

function limit_speed(speed, limits)
  -- don't use ipairs(), since it stops at the first nil value
  for i=1, #limits do
    limit = limits[i]
    if limit ~= nil and limit > 0 then
      if limit < speed then
        return limit        -- stop at first speedlimit that's smaller than speed
      end
    end
  end
  return speed
end

function node_function (node)
  local traffic_signal = node.tags:Find("highway")

  if traffic_signal == "traffic_signals" then
    node.traffic_light = true;
    -- TODO: a way to set the penalty value
  end
  return 1
end

function way_function (way)
  local highway = way.tags:Find("highway")
  local name = way.tags:Find("name")
  local oneway = way.tags:Find("oneway")
  local route = way.tags:Find("route")
  local duration = way.tags:Find("duration")
  local maxspeed = tonumber(way.tags:Find ( "maxspeed"))
  local maxspeed_forward = tonumber(way.tags:Find( "maxspeed:forward"))
  local maxspeed_backward = tonumber(way.tags:Find( "maxspeed:backward"))
  local junction = way.tags:Find("junction")

  way.name = name

  if route ~= nil and durationIsValid(duration) then
    way.duration = math.max( 1, parseDuration(duration) )
    way.forward_mode = 2
    way.backward_mode = 2
  else
    local speed_forw = speed_profile[highway] or speed_profile['default']
    local speed_back = speed_forw

    if highway == "river" then
      local temp_speed = speed_forw;
      way.forward_mode = 3
      way.backward_mode = 4
      speed_forw = temp_speed*1.5
      speed_back = temp_speed/1.5
    elseif highway == "steps" then
      way.forward_mode = 5
      way.backward_mode = 6
    end

    if maxspeed_forward ~= nil and maxspeed_forward > 0 then
      speed_forw = maxspeed_forward
    else
      if maxspeed ~= nil and maxspeed > 0 and speed_forw > maxspeed then
        speed_forw = maxspeed
      end
    end

    if maxspeed_backward ~= nil and maxspeed_backward > 0 then
      speed_back = maxspeed_backward
    else
      if maxspeed ~=nil and maxspeed > 0 and speed_back > maxspeed then
        speed_back = maxspeed
      end
    end

    way.forward_speed = speed_forw
    way.backward_speed = speed_back
  end

  if oneway == "no" or oneway == "0" or oneway == "false" then
    -- nothing to do
  elseif oneway == "-1" then
    way.forward_mode = 0
  elseif oneway == "yes" or oneway == "1" or oneway == "true" or junction == "roundabout" then
    way.backward_mode = 0
  end

  if junction == 'roundabout' then
    way.roundabout = true
  end
end
