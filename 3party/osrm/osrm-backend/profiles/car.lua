-- Begin of globals
--require("lib/access") --function temporarily inlined

barrier_whitelist = { ["cattle_grid"] = true, ["border_control"] = true, ["toll_booth"] = true, ["sally_port"] = true, ["gate"] = true, ["no"] = true, ["entrance"] = true }
access_tag_whitelist = { ["yes"] = true, ["motorcar"] = true, ["motor_vehicle"] = true, ["vehicle"] = true, ["permissive"] = true, ["designated"] = true }
access_tag_blacklist = { ["no"] = true, ["private"] = true, ["agricultural"] = true, ["forestry"] = true, ["emergency"] = true }
access_tag_restricted = { ["destination"] = true, ["delivery"] = true }
access_tags = { "motorcar", "motor_vehicle", "vehicle" }
access_tags_hierachy = { "motorcar", "motor_vehicle", "vehicle", "access" }
service_tag_restricted = { ["parking_aisle"] = true }
ignore_in_grid = { ["ferry"] = true }
restriction_exception_tags = { "motorcar", "motor_vehicle", "vehicle" }

speed_profile = {
  ["motorway"] = 90,
  ["motorway_link"] = 75,
  ["trunk"] = 85,
  ["trunk_link"] = 70,
  ["primary"] = 65,
  ["primary_link"] = 60,
  ["secondary"] = 55,
  ["secondary_link"] = 50,
  ["tertiary"] = 40,
  ["tertiary_link"] = 30,
  ["unclassified"] = 25,
  ["residential"] = 25,
  ["living_street"] = 10,
  ["service"] = 15,
--  ["track"] = 5,
  ["ferry"] = 5,
  ["shuttle_train"] = 10,
  ["default"] = 10
}

traffic_signal_penalty          = 2

-- End of globals
local take_minimum_of_speeds    = false
local obey_oneway               = true
local obey_bollards             = true
local use_turn_restrictions     = true
local ignore_areas              = true     -- future feature
local u_turn_penalty            = 20

local abs = math.abs
local min = math.min
local max = math.max

local speed_reduction = 0.8

local function find_access_tag(source,access_tags_hierachy)
  for i,v in ipairs(access_tags_hierachy) do
    local has_tag = source.tags:Holds(v)
    if has_tag then
      return source.tags:Find(v)
    end
  end
  return nil
end

function get_exceptions(vector)
  for i,v in ipairs(restriction_exception_tags) do
    vector:Add(v)
  end
end

local function parse_maxspeed(source)
  if not source then
    return 0
  end
  local n = tonumber(source:match("%d*"))
  if not n then
    n = 0
  end
  if string.match(source, "mph") or string.match(source, "mp/h") then
    n = (n*1609)/1000;
  end
  return n
end

-- function turn_function (angle)
--   -- print ("called at angle " .. angle )
--   local index = math.abs(math.floor(angle/10+0.5))+1 -- +1 'coz LUA starts as idx 1
--   local penalty = turn_cost_table[index]
--   -- print ("index: " .. index .. ", bias: " .. penalty )
--   return penalty
-- end

function node_function (node)
  local access = find_access_tag(node, access_tags_hierachy)

  --flag node if it carries a traffic light
  if node.tags:Holds("highway") then
    if node.tags:Find("highway") == "traffic_signals" then
      node.traffic_light = true;
    end
  end

  -- parse access and barrier tags
  if access and access ~= "" then
    if access_tag_blacklist[access] then
      node.bollard = true
    end
  elseif node.tags:Holds("barrier") then
    local barrier = node.tags:Find("barrier")
    if barrier_whitelist[barrier] then
      return
    else
      node.bollard = true
    end
  end
end

function way_function (way)

  local is_highway = way.tags:Holds("highway")
  local is_route = way.tags:Holds("route")

  if not (is_highway or is_route) then
    return
  end

  -- we dont route over areas
  local is_area = way.tags:Holds("area")
  if ignore_areas and is_area then
    local area = way.tags:Find("area")
    if "yes" == area then
      return
    end
  end

  -- check if oneway tag is unsupported
  local oneway = way.tags:Find("oneway")
  if "reversible" == oneway then
    return
  end

  local is_impassable = way.tags:Holds("impassable")
  if is_impassable then
    local impassable = way.tags:Find("impassable")
    if "yes" == impassable then
      return
    end
  end

  local is_status = way.tags:Holds("status")
  if is_status then
    local status = way.tags:Find("status")
    if "impassable" == status then
      return
    end
  end

  -- Check if we are allowed to access the way
  local access = find_access_tag(way, access_tags_hierachy)
  if access_tag_blacklist[access] then
    return
  end

  -- Second, parse the way according to these properties
  local highway = way.tags:Find("highway")
  local route = way.tags:Find("route")

  -- Handling ferries and piers
  local route_speed = speed_profile[route]
  if(route_speed and route_speed > 0) then
    highway = route;
    local duration  = way.tags:Find("duration")
    if durationIsValid(duration) then
      way.duration = max( parseDuration(duration), 1 );
    end
    way.direction = Way.bidirectional
    way.speed = route_speed
  end

  -- leave early of this way is not accessible
  if "" == highway then
    return
  end

  if way.speed == -1 then
    local highway_speed = speed_profile[highway]
    local max_speed = parse_maxspeed( way.tags:Find("maxspeed") )
    -- Set the avg speed on the way if it is accessible by road class
    if highway_speed then
      if max_speed > highway_speed then
        way.speed = max_speed
        -- max_speed = math.huge
      else
        way.speed = highway_speed
      end
    else
      -- Set the avg speed on ways that are marked accessible
      if access_tag_whitelist[access] then
        way.speed = speed_profile["default"]
      end
    end
    if 0 == max_speed then
      max_speed = math.huge
    end
    way.speed = min(way.speed, max_speed)
  end

  if -1 == way.speed then
    return
  end

  -- parse the remaining tags
  local name = way.tags:Find("name")
  local ref = way.tags:Find("ref")
  local junction = way.tags:Find("junction")
  -- local barrier = way.tags:Find("barrier")
  -- local cycleway = way.tags:Find("cycleway")
  local service  = way.tags:Find("service")

  -- Set the name that will be used for instructions
  if "" ~= ref then
    way.name = ref
  elseif "" ~= name then
    way.name = name
--  else
      --    way.name = highway  -- if no name exists, use way type
  end

  if "roundabout" == junction then
    way.roundabout = true;
  end

  -- Set access restriction flag if access is allowed under certain restrictions only
  if access ~= "" and access_tag_restricted[access] then
    way.is_access_restricted = true
  end

  -- Set access restriction flag if service is allowed under certain restrictions only
  if service ~= "" and service_tag_restricted[service] then
    way.is_access_restricted = true
  end

  -- Set direction according to tags on way
  way.direction = Way.bidirectional
  if obey_oneway  then
    if oneway == "-1" then
      way.direction = Way.opposite
    elseif oneway == "yes" or
    oneway == "1" or
    oneway == "true" or
    junction == "roundabout" or
    (highway == "motorway_link" and oneway ~="no") or
    (highway == "motorway" and oneway ~= "no") then
      way.direction = Way.oneway
    end
  end

  -- Override speed settings if explicit forward/backward maxspeeds are given
  local maxspeed_forward = parse_maxspeed(way.tags:Find( "maxspeed:forward"))
  local maxspeed_backward = parse_maxspeed(way.tags:Find( "maxspeed:backward"))
  if maxspeed_forward > 0 then
    if Way.bidirectional == way.direction then
      way.backward_speed = way.speed
    end
    way.speed = maxspeed_forward
  end
  if maxspeed_backward > 0 then
    way.backward_speed = maxspeed_backward
  end

  -- Override general direction settings of there is a specific one for our mode of travel
  if ignore_in_grid[highway] then
    way.ignore_in_grid = true
  end
  way.type = 1

  -- scale speeds to get better avg driving times
  way.speed = way.speed * speed_reduction
  if maxspeed_backward > 0 then
    way.backward_speed = way.backward_speed*speed_reduction
  end
  return
end

-- These are wrappers to parse vectors of nodes and ways and thus to speed up any tracing JIT
function node_vector_function(vector)
  for v in vector.nodes do
    node_function(v)
  end
end
