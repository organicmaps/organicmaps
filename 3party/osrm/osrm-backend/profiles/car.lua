-- Begin of globals
--require("lib/access") --function temporarily inlined

barrier_whitelist = { ["cattle_grid"] = true, ["border_control"] = true, ["checkpoint"] = true, ["toll_booth"] = true, ["sally_port"] = true, ["gate"] = true, ["no"] = true, ["entrance"] = true }
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
  ["motorway_link"] = 45,
  ["trunk"] = 85,
  ["trunk_link"] = 40,
  ["primary"] = 65,
  ["primary_link"] = 30,
  ["secondary"] = 55,
  ["secondary_link"] = 25,
  ["tertiary"] = 40,
  ["tertiary_link"] = 20,
  ["unclassified"] = 25,
  ["residential"] = 25,
  ["living_street"] = 10,
  ["service"] = 15,
--  ["track"] = 5,
  ["ferry"] = 5,
  ["shuttle_train"] = 10,
  ["default"] = 10
}


-- surface/trackype/smoothness
-- values were estimated from looking at the photos at the relevant wiki pages

-- max speed for surfaces
surface_speeds = {
  ["asphalt"] = nil,    -- nil mean no limit. removing the line has the same effect
  ["concrete"] = nil,
  ["concrete:plates"] = nil,
  ["concrete:lanes"] = nil,
  ["paved"] = nil,

  ["cement"] = 80,
  ["compacted"] = 80,
  ["fine_gravel"] = 80,

  ["paving_stones"] = 60,
  ["metal"] = 60,
  ["bricks"] = 60,

  ["grass"] = 40,
  ["wood"] = 40,
  ["sett"] = 40,
  ["grass_paver"] = 40,
  ["gravel"] = 40,
  ["unpaved"] = 40,
  ["ground"] = 40,
  ["dirt"] = 40,
  ["pebblestone"] = 40,
  ["tartan"] = 40,

  ["cobblestone"] = 30,
  ["clay"] = 30,

  ["earth"] = 20,
  ["stone"] = 20,
  ["rocky"] = 20,
  ["sand"] = 20,

  ["mud"] = 10
}

-- max speed for tracktypes
tracktype_speeds = {
  ["grade1"] =  60,
  ["grade2"] =  40,
  ["grade3"] =  30,
  ["grade4"] =  25,
  ["grade5"] =  20
}

-- max speed for smoothnesses
smoothness_speeds = {
  ["intermediate"]    =  80,
  ["bad"]             =  40,
  ["very_bad"]        =  20,
  ["horrible"]        =  10,
  ["very_horrible"]   =  5,
  ["impassable"]      =  0
}

-- http://wiki.openstreetmap.org/wiki/Speed_limits
maxspeed_table_default = {
  ["urban"] = 50,
  ["rural"] = 90,
  ["trunk"] = 110,
  ["motorway"] = 130
}

-- List only exceptions
maxspeed_table = {
  ["de:living_street"] = 7,
  ["ru:living_street"] = 20,
  ["ru:urban"] = 60,
  ["ua:urban"] = 60,
  ["at:rural"] = 100,
  ["de:rural"] = 100,
  ["at:trunk"] = 100,
  ["cz:trunk"] = 0,
  ["ro:trunk"] = 100,
  ["cz:motorway"] = 0,
  ["de:motorway"] = 0,
  ["ru:motorway"] = 110,
  ["gb:nsl_single"] = (60*1609)/1000,
  ["gb:nsl_dual"] = (70*1609)/1000,
  ["gb:motorway"] = (70*1609)/1000,
  ["uk:nsl_single"] = (60*1609)/1000,
  ["uk:nsl_dual"] = (70*1609)/1000,
  ["uk:motorway"] = (70*1609)/1000
}

traffic_signal_penalty          = 2

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

--modes
local mode_normal = 1
local mode_ferry = 2


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
  if n then
    if string.match(source, "mph") or string.match(source, "mp/h") then
      n = (n*1609)/1000;
    end
  else
    -- parse maxspeed like FR:urban
    source = string.lower(source)
    n = maxspeed_table[source]
    if not n then
      local highway_type = string.match(source, "%a%a:(%a+)")
      n = maxspeed_table_default[highway_type]
      if not n then
        n = 0
      end
    end
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
    way.forward_mode = mode_ferry
    way.backward_mode = mode_ferry
    way.forward_speed = route_speed
    way.backward_speed = route_speed
  end

  -- leave early of this way is not accessible
  if "" == highway then
    return
  end

  if way.forward_speed == -1 then
    local highway_speed = speed_profile[highway]
    local max_speed = parse_maxspeed( way.tags:Find("maxspeed") )
    -- Set the avg speed on the way if it is accessible by road class
    if highway_speed then
      if max_speed > highway_speed then
        way.forward_speed = max_speed
        way.backward_speed = max_speed
        -- max_speed = math.huge
      else
        way.forward_speed = highway_speed
        way.backward_speed = highway_speed
      end
    else
      -- Set the avg speed on ways that are marked accessible
      if access_tag_whitelist[access] then
        way.forward_speed = speed_profile["default"]
        way.backward_speed = speed_profile["default"]
      end
    end
    if 0 == max_speed then
      max_speed = math.huge
    end
    way.forward_speed = min(way.forward_speed, max_speed)
    way.backward_speed = min(way.backward_speed, max_speed)
  end

  if -1 == way.forward_speed and -1 == way.backward_speed then
    return
  end

  -- reduce speed on bad surfaces
  local surface = way.tags:Find("surface")
  local tracktype = way.tags:Find("tracktype")
  local smoothness = way.tags:Find("smoothness")

  if surface and surface_speeds[surface] then
    way.forward_speed = math.min(surface_speeds[surface], way.forward_speed)
    way.backward_speed = math.min(surface_speeds[surface], way.backward_speed)
  end
  if tracktype and tracktype_speeds[tracktype] then
    way.forward_speed = math.min(tracktype_speeds[tracktype], way.forward_speed)
    way.backward_speed = math.min(tracktype_speeds[tracktype], way.backward_speed)
  end
  if smoothness and smoothness_speeds[smoothness] then
    way.forward_speed = math.min(smoothness_speeds[smoothness], way.forward_speed)
    way.backward_speed = math.min(smoothness_speeds[smoothness], way.backward_speed)
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
  if obey_oneway  then
    if oneway == "-1" then
      way.forward_mode = 0
    elseif oneway == "yes" or
    oneway == "1" or
    oneway == "true" or
    junction == "roundabout" or
    (highway == "motorway_link" and oneway ~="no") or
    (highway == "motorway" and oneway ~= "no") then
      way.backward_mode = 0
    end
  end

  -- Override speed settings if explicit forward/backward maxspeeds are given
  local maxspeed_forward = parse_maxspeed(way.tags:Find( "maxspeed:forward"))
  local maxspeed_backward = parse_maxspeed(way.tags:Find( "maxspeed:backward"))
  if maxspeed_forward > 0 then
    if 0 ~= way.forward_mode and 0 ~= way.backward_mode then
      way.backward_speed = way.forward_speed
    end
    way.forward_speed = maxspeed_forward
  end
  if maxspeed_backward > 0 then
    way.backward_speed = maxspeed_backward
  end

  -- Override general direction settings of there is a specific one for our mode of travel
  if ignore_in_grid[highway] then
    way.ignore_in_grid = true
  end

  -- scale speeds to get better avg driving times
  way.forward_speed = way.forward_speed * speed_reduction
  if way.backward_speed > 0 then
    way.backward_speed = way.backward_speed*speed_reduction
  end
end

-- These are wrappers to parse vectors of nodes and ways and thus to speed up any tracing JIT
function node_vector_function(vector)
  for v in vector.nodes do
    node_function(v)
  end
end
