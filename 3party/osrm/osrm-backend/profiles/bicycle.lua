require("lib/access")
require("lib/maxspeed")

-- Begin of globals
barrier_whitelist = { [""] = true, ["cycle_barrier"] = true, ["bollard"] = true, ["entrance"] = true, ["cattle_grid"] = true, ["border_control"] = true, ["toll_booth"] = true, ["sally_port"] = true, ["gate"] = true, ["no"] = true }
access_tag_whitelist = { ["yes"] = true, ["permissive"] = true, ["designated"] = true }
access_tag_blacklist = { ["no"] = true, ["private"] = true, ["agricultural"] = true, ["forestery"] = true }
access_tag_restricted = { ["destination"] = true, ["delivery"] = true }
access_tags_hierachy = { "bicycle", "vehicle", "access" }
cycleway_tags = {["track"]=true,["lane"]=true,["opposite"]=true,["opposite_lane"]=true,["opposite_track"]=true,["share_busway"]=true,["sharrow"]=true,["shared"]=true }
service_tag_restricted = { ["parking_aisle"] = true }
restriction_exception_tags = { "bicycle", "vehicle", "access" }

default_speed = 15

walking_speed = 6

bicycle_speeds = {
  ["cycleway"] = default_speed,
  ["primary"] = default_speed,
  ["primary_link"] = default_speed,
  ["secondary"] = default_speed,
  ["secondary_link"] = default_speed,
  ["tertiary"] = default_speed,
  ["tertiary_link"] = default_speed,
  ["residential"] = default_speed,
  ["unclassified"] = default_speed,
  ["living_street"] = default_speed,
  ["road"] = default_speed,
  ["service"] = default_speed,
  ["track"] = 12,
  ["path"] = 12
  --["footway"] = 12,
  --["pedestrian"] = 12,
}

pedestrian_speeds = {
  ["footway"] = walking_speed,
  ["pedestrian"] = walking_speed,
  ["steps"] = 2
}

railway_speeds = {
  ["train"] = 10,
  ["railway"] = 10,
  ["subway"] = 10,
  ["light_rail"] = 10,
  ["monorail"] = 10,
  ["tram"] = 10
}

platform_speeds = {
  ["platform"] = walking_speed
}

amenity_speeds = {
  ["parking"] = 10,
  ["parking_entrance"] = 10
}

man_made_speeds = {
  ["pier"] = walking_speed
}

route_speeds = {
  ["ferry"] = 5
}

bridge_speeds = {
  ["movable"] = 5
}

surface_speeds = {
  ["asphalt"] = default_speed,
  ["cobblestone:flattened"] = 10,
  ["paving_stones"] = 10,
  ["compacted"] = 10,
  ["cobblestone"] = 6,
  ["unpaved"] = 6,
  ["fine_gravel"] = 6,
  ["gravel"] = 6,
  ["fine_gravel"] = 6,
  ["pebbelstone"] = 6,
  ["ground"] = 6,
  ["dirt"] = 6,
  ["earth"] = 6,
  ["grass"] = 6,
  ["mud"] = 3,
  ["sand"] = 3
}

take_minimum_of_speeds  = true
obey_oneway       = true
obey_bollards       = false
use_restrictions    = true
ignore_areas      = true    -- future feature
traffic_signal_penalty  = 5
u_turn_penalty      = 20
use_turn_restrictions   = false
turn_penalty      = 60
turn_bias         = 1.4


--modes
mode_normal = 1
mode_pushing = 2
mode_ferry = 3
mode_train = 4
mode_movable_bridge = 5

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


function get_exceptions(vector)
  for i,v in ipairs(restriction_exception_tags) do
    vector:Add(v)
  end
end

function node_function (node, result)
  local barrier = node:get_value_by_key("barrier")
  local access = Access.find_access_tag(node, access_tags_hierachy)
  local traffic_signal = node:get_value_by_key("highway")

	-- flag node if it carries a traffic light
	if traffic_signal and traffic_signal == "traffic_signals" then
		result.traffic_lights = true
	end

	-- parse access and barrier tags
	if access and access ~= "" then
		if access_tag_blacklist[access] then
			result.barrier = true
		else
			result.barrier = false
		end
	elseif barrier and barrier ~= "" then
		if barrier_whitelist[barrier] then
			result.barrier = false
		else
			result.barrier = true
		end
	end
end

function way_function (way, result)
  -- initial routability check, filters out buildings, boundaries, etc
  local highway = way:get_value_by_key("highway")
  local route = way:get_value_by_key("route")
  local man_made = way:get_value_by_key("man_made")
  local railway = way:get_value_by_key("railway")
  local amenity = way:get_value_by_key("amenity")
  local public_transport = way:get_value_by_key("public_transport")
  local bridge = way:get_value_by_key("bridge")
  if (not highway or highway == '') and
  (not route or route == '') and
  (not railway or railway=='') and
  (not amenity or amenity=='') and
  (not man_made or man_made=='') and
  (not public_transport or public_transport=='') and
  (not bridge or bridge=='')
  then
    return
  end

  -- don't route on ways or railways that are still under construction
  if highway=='construction' or railway=='construction' then
    return
  end

  -- access
  local access = Access.find_access_tag(way, access_tags_hierachy)
  if access and access_tag_blacklist[access] then
    return
  end

  -- other tags
  local name = way:get_value_by_key("name")
  local ref = way:get_value_by_key("ref")
  local junction = way:get_value_by_key("junction")
  local maxspeed = parse_maxspeed(way:get_value_by_key ( "maxspeed") )
  local maxspeed_forward = parse_maxspeed(way:get_value_by_key( "maxspeed:forward"))
  local maxspeed_backward = parse_maxspeed(way:get_value_by_key( "maxspeed:backward"))
  local barrier = way:get_value_by_key("barrier")
  local oneway = way:get_value_by_key("oneway")
  local onewayClass = way:get_value_by_key("oneway:bicycle")
  local cycleway = way:get_value_by_key("cycleway")
  local cycleway_left = way:get_value_by_key("cycleway:left")
  local cycleway_right = way:get_value_by_key("cycleway:right")
  local duration = way:get_value_by_key("duration")
  local service = way:get_value_by_key("service")
  local area = way:get_value_by_key("area")
  local foot = way:get_value_by_key("foot")
  local surface = way:get_value_by_key("surface")
  local bicycle = way:get_value_by_key("bicycle")

  -- name
  if ref and "" ~= ref and name and "" ~= name then
    result.name = name .. ' / ' .. ref
  elseif ref and "" ~= ref then
    result.name = ref
  elseif name and "" ~= name then
    result.name = name
  elseif highway then
    -- if no name exists, use way type
    -- this encoding scheme is excepted to be a temporary solution
    result.name = "{highway:"..highway.."}"
  end

  -- roundabout handling
  if junction and "roundabout" == junction then
    result.roundabout = true;
  end

  -- speed
  local bridge_speed = bridge_speeds[bridge]
  if (bridge_speed and bridge_speed > 0) then
    highway = bridge;
    if duration and durationIsValid(duration) then
      result.duration = math.max( parseDuration(duration), 1 );
    end
    result.forward_mode = mode_movable_bridge
    result.backward_mode = mode_movable_bridge
    result.forward_speed = bridge_speed
    result.backward_speed = bridge_speed
  elseif route_speeds[route] then
    -- ferries (doesn't cover routes tagged using relations)
    result.forward_mode = mode_ferry
    result.backward_mode = mode_ferry
    result.ignore_in_grid = true
    if duration and durationIsValid(duration) then
      result.duration = math.max( 1, parseDuration(duration) )
    else
       result.forward_speed = route_speeds[route]
       result.backward_speed = route_speeds[route]
    end
  elseif railway and platform_speeds[railway] then
    -- railway platforms (old tagging scheme)
    result.forward_speed = platform_speeds[railway]
    result.backward_speed = platform_speeds[railway]
  elseif platform_speeds[public_transport] then
    -- public_transport platforms (new tagging platform)
    result.forward_speed = platform_speeds[public_transport]
    result.backward_speed = platform_speeds[public_transport]
    elseif railway and railway_speeds[railway] then
      result.forward_mode = mode_train
      result.backward_mode = mode_train
     -- railways
    if access and access_tag_whitelist[access] then
      result.forward_speed = railway_speeds[railway]
      result.backward_speed = railway_speeds[railway]
    end
  elseif amenity and amenity_speeds[amenity] then
    -- parking areas
    result.forward_speed = amenity_speeds[amenity]
    result.backward_speed = amenity_speeds[amenity]
  elseif bicycle_speeds[highway] then
    -- regular ways
    result.forward_speed = bicycle_speeds[highway]
    result.backward_speed = bicycle_speeds[highway]
  elseif access and access_tag_whitelist[access] then
    -- unknown way, but valid access tag
    result.forward_speed = default_speed
    result.backward_speed = default_speed
  else
    -- biking not allowed, maybe we can push our bike?
    -- essentially requires pedestrian profiling, for example foot=no mean we can't push a bike
    if foot ~= 'no' and junction ~= "roundabout" then
      if pedestrian_speeds[highway] then
        -- pedestrian-only ways and areas
        result.forward_speed = pedestrian_speeds[highway]
        result.backward_speed = pedestrian_speeds[highway]
        result.forward_mode = mode_pushing
        result.backward_mode = mode_pushing
      elseif man_made and man_made_speeds[man_made] then
        -- man made structures
        result.forward_speed = man_made_speeds[man_made]
        result.backward_speed = man_made_speeds[man_made]
        result.forward_mode = mode_pushing
        result.backward_mode = mode_pushing
      elseif foot == 'yes' then
        result.forward_speed = walking_speed
        result.backward_speed = walking_speed
        result.forward_mode = mode_pushing
        result.backward_mode = mode_pushing
      elseif foot_forward == 'yes' then
        result.forward_speed = walking_speed
        result.forward_mode = mode_pushing
        result.backward_mode = 0
      elseif foot_backward == 'yes' then
        result.forward_speed = walking_speed
        result.forward_mode = 0
        result.backward_mode = mode_pushing
      end
    end
  end

  -- direction
  local impliedOneway = false
  if junction == "roundabout" or highway == "motorway_link" or highway == "motorway" then
    impliedOneway = true
  end

  if onewayClass == "yes" or onewayClass == "1" or onewayClass == "true" then
    result.backward_mode = 0
  elseif onewayClass == "no" or onewayClass == "0" or onewayClass == "false" then
    -- prevent implied oneway
  elseif onewayClass == "-1" then
    result.forward_mode = 0
  elseif oneway == "no" or oneway == "0" or oneway == "false" then
    -- prevent implied oneway
  elseif cycleway and string.find(cycleway, "opposite") == 1 then
    if impliedOneway then
      result.forward_mode = 0
      result.backward_mode = mode_normal
      result.backward_speed = bicycle_speeds["cycleway"]
    end
  elseif cycleway_left and cycleway_tags[cycleway_left] and cycleway_right and cycleway_tags[cycleway_right] then
    -- prevent implied
  elseif cycleway_left and cycleway_tags[cycleway_left] then
    if impliedOneway then
      result.forward_mode = 0
      result.backward_mode = mode_normal
      result.backward_speed = bicycle_speeds["cycleway"]
    end
  elseif cycleway_right and cycleway_tags[cycleway_right] then
    if impliedOneway then
      result.forward_mode = mode_normal
      result.backward_speed = bicycle_speeds["cycleway"]
      result.backward_mode = 0
    end
  elseif oneway == "-1" then
    result.forward_mode = 0
  elseif oneway == "yes" or oneway == "1" or oneway == "true" or impliedOneway then
    result.backward_mode = 0
  end

  -- pushing bikes
  if bicycle_speeds[highway] or pedestrian_speeds[highway] then
    if foot ~= "no" and junction ~= "roundabout" then
      if result.backward_mode == 0 then
        result.backward_speed = walking_speed
        result.backward_mode = mode_pushing
      elseif result.forward_mode == 0 then
        result.forward_speed = walking_speed
        result.forward_mode = mode_pushing
      end
    end
  end

  -- cycleways
  if cycleway and cycleway_tags[cycleway] then
    result.forward_speed = bicycle_speeds["cycleway"]
  elseif cycleway_left and cycleway_tags[cycleway_left] then
    result.forward_speed = bicycle_speeds["cycleway"]
  elseif cycleway_right and cycleway_tags[cycleway_right] then
    result.forward_speed = bicycle_speeds["cycleway"]
  end

  -- dismount
  if bicycle == "dismount" then
    result.forward_mode = mode_pushing
    result.backward_mode = mode_pushing
    result.forward_speed = walking_speed
    result.backward_speed = walking_speed
  end

  -- surfaces
  if surface then
    surface_speed = surface_speeds[surface]
    if surface_speed then
      if result.forward_speed > 0 then
        result.forward_speed = surface_speed
      end
      if result.backward_speed > 0 then
        result.backward_speed  = surface_speed
      end
    end
  end

  -- maxspeed
  MaxSpeed.limit( result, maxspeed, maxspeed_forward, maxspeed_backward )
end

function turn_function (angle)
  -- compute turn penalty as angle^2, with a left/right bias
  k = turn_penalty/(90.0*90.0)
  if angle>=0 then
    return angle*angle*k/turn_bias
  else
    return angle*angle*k*turn_bias
  end
end
