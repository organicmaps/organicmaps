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

function node_function (node)
  local barrier = node.tags:Find ("barrier")
  local access = Access.find_access_tag(node, access_tags_hierachy)
  local traffic_signal = node.tags:Find("highway")

	-- flag node if it carries a traffic light
	if traffic_signal == "traffic_signals" then
		node.traffic_light = true
	end

	-- parse access and barrier tags
	if access and access ~= "" then
		if access_tag_blacklist[access] then
			node.bollard = true
		else
			node.bollard = false
		end
	elseif barrier and barrier ~= "" then
		if barrier_whitelist[barrier] then
			node.bollard = false
		else
			node.bollard = true
		end
	end

	-- return 1
end

function way_function (way)
  -- initial routability check, filters out buildings, boundaries, etc
  local highway = way.tags:Find("highway")
  local route = way.tags:Find("route")
  local man_made = way.tags:Find("man_made")
  local railway = way.tags:Find("railway")
  local amenity = way.tags:Find("amenity")
  local public_transport = way.tags:Find("public_transport")
  if (not highway or highway == '') and
  (not route or route == '') and
  (not railway or railway=='') and
  (not amenity or amenity=='') and
  (not man_made or man_made=='') and
  (not public_transport or public_transport=='')
  then
    return
  end

  -- don't route on ways or railways that are still under construction
  if highway=='construction' or railway=='construction' then
    return
  end

  -- access
  local access = Access.find_access_tag(way, access_tags_hierachy)
  if access_tag_blacklist[access] then
    return
  end

  -- other tags
  local name = way.tags:Find("name")
  local ref = way.tags:Find("ref")
  local junction = way.tags:Find("junction")
  local maxspeed = parse_maxspeed(way.tags:Find ( "maxspeed") )
  local maxspeed_forward = parse_maxspeed(way.tags:Find( "maxspeed:forward"))
  local maxspeed_backward = parse_maxspeed(way.tags:Find( "maxspeed:backward"))
  local barrier = way.tags:Find("barrier")
  local oneway = way.tags:Find("oneway")
  local onewayClass = way.tags:Find("oneway:bicycle")
  local cycleway = way.tags:Find("cycleway")
  local cycleway_left = way.tags:Find("cycleway:left")
  local cycleway_right = way.tags:Find("cycleway:right")
  local duration = way.tags:Find("duration")
  local service = way.tags:Find("service")
  local area = way.tags:Find("area")
  local foot = way.tags:Find("foot")
  local surface = way.tags:Find("surface")
  local bicycle = way.tags:Find("bicycle")

  -- name
  if "" ~= ref and "" ~= name then
    way.name = name .. ' / ' .. ref
  elseif "" ~= ref then
    way.name = ref
  elseif "" ~= name then
    way.name = name
  else
    -- if no name exists, use way type
    -- this encoding scheme is excepted to be a temporary solution
    way.name = "{highway:"..highway.."}"
  end

  -- roundabout handling
  if "roundabout" == junction then
    way.roundabout = true;
  end

  -- speed
  if route_speeds[route] then
    -- ferries (doesn't cover routes tagged using relations)
    way.forward_mode = mode_ferry
    way.backward_mode = mode_ferry
    way.ignore_in_grid = true
    if durationIsValid(duration) then
      way.duration = math.max( 1, parseDuration(duration) )
    else
       way.forward_speed = route_speeds[route]
       way.backward_speed = route_speeds[route]
    end
  elseif railway and platform_speeds[railway] then
    -- railway platforms (old tagging scheme)
    way.forward_speed = platform_speeds[railway]
    way.backward_speed = platform_speeds[railway]
  elseif platform_speeds[public_transport] then
    -- public_transport platforms (new tagging platform)
    way.forward_speed = platform_speeds[public_transport]
    way.backward_speed = platform_speeds[public_transport]
    elseif railway and railway_speeds[railway] then
      way.forward_mode = mode_train
      way.backward_mode = mode_train
     -- railways
    if access and access_tag_whitelist[access] then
      way.forward_speed = railway_speeds[railway]
      way.backward_speed = railway_speeds[railway]
    end
  elseif amenity and amenity_speeds[amenity] then
    -- parking areas
    way.forward_speed = amenity_speeds[amenity]
    way.backward_speed = amenity_speeds[amenity]
  elseif bicycle_speeds[highway] then
    -- regular ways
    way.forward_speed = bicycle_speeds[highway]
    way.backward_speed = bicycle_speeds[highway]
  elseif access and access_tag_whitelist[access] then
    -- unknown way, but valid access tag
    way.forward_speed = default_speed
    way.backward_speed = default_speed
  else
    -- biking not allowed, maybe we can push our bike?
    -- essentially requires pedestrian profiling, for example foot=no mean we can't push a bike
    if foot ~= 'no' and junction ~= "roundabout" then
      if pedestrian_speeds[highway] then
        -- pedestrian-only ways and areas
        way.forward_speed = pedestrian_speeds[highway]
        way.backward_speed = pedestrian_speeds[highway]
        way.forward_mode = mode_pushing
        way.backward_mode = mode_pushing
      elseif man_made and man_made_speeds[man_made] then
        -- man made structures
        way.forward_speed = man_made_speeds[man_made]
        way.backward_speed = man_made_speeds[man_made]
        way.forward_mode = mode_pushing
        way.backward_mode = mode_pushing
      elseif foot == 'yes' then
        way.forward_speed = walking_speed
        way.backward_speed = walking_speed
        way.forward_mode = mode_pushing
        way.backward_mode = mode_pushing
      elseif foot_forward == 'yes' then
        way.forward_speed = walking_speed
        way.forward_mode = mode_pushing
        way.backward_mode = 0
      elseif foot_backward == 'yes' then
        way.forward_speed = walking_speed
        way.forward_mode = 0
        way.backward_mode = mode_pushing
      end
    end
  end

  -- direction
  local impliedOneway = false
  if junction == "roundabout" or highway == "motorway_link" or highway == "motorway" then
    impliedOneway = true
  end

  if onewayClass == "yes" or onewayClass == "1" or onewayClass == "true" then
    way.backward_mode = 0
  elseif onewayClass == "no" or onewayClass == "0" or onewayClass == "false" then
    -- prevent implied oneway
  elseif onewayClass == "-1" then
    way.forward_mode = 0
  elseif oneway == "no" or oneway == "0" or oneway == "false" then
    -- prevent implied oneway
  elseif cycleway and string.find(cycleway, "opposite") == 1 then
    if impliedOneway then
      way.forward_mode = 0
      way.backward_mode = mode_normal
      way.backward_speed = bicycle_speeds["cycleway"]
    end
  elseif cycleway_left and cycleway_tags[cycleway_left] and cycleway_right and cycleway_tags[cycleway_right] then
    -- prevent implied
  elseif cycleway_left and cycleway_tags[cycleway_left] then
    if impliedOneway then
      way.forward_mode = 0
      way.backward_mode = mode_normal
      way.backward_speed = bicycle_speeds["cycleway"]
    end
  elseif cycleway_right and cycleway_tags[cycleway_right] then
    if impliedOneway then
      way.forward_mode = mode_normal
      way.backward_speed = bicycle_speeds["cycleway"]
      way.backward_mode = 0
    end
  elseif oneway == "-1" then
    way.forward_mode = 0
  elseif oneway == "yes" or oneway == "1" or oneway == "true" or impliedOneway then
    way.backward_mode = 0
  end

  -- pushing bikes
  if bicycle_speeds[highway] or pedestrian_speeds[highway] then
    if foot ~= "no" and junction ~= "roundabout" then
      if way.backward_mode == 0 then
        way.backward_speed = walking_speed
        way.backward_mode = mode_pushing
      elseif way.forward_mode == 0 then
        way.forward_speed = walking_speed
        way.forward_mode = mode_pushing
      end
    end
  end

  -- cycleways
  if cycleway and cycleway_tags[cycleway] then
    way.forward_speed = bicycle_speeds["cycleway"]
  elseif cycleway_left and cycleway_tags[cycleway_left] then
    way.forward_speed = bicycle_speeds["cycleway"]
  elseif cycleway_right and cycleway_tags[cycleway_right] then
    way.forward_speed = bicycle_speeds["cycleway"]
  end

  -- dismount
  if bicycle == "dismount" then
    way.forward_mode = mode_pushing
    way.backward_mode = mode_pushing
    way.forward_speed = walking_speed
    way.backward_speed = walking_speed
  end

  -- surfaces
  if surface then
    surface_speed = surface_speeds[surface]
    if surface_speed then
      if way.forward_speed > 0 then
        way.forward_speed = surface_speed
      end
      if way.backward_speed > 0 then
        way.backward_speed  = surface_speed
      end
    end
  end

  -- maxspeed
  MaxSpeed.limit( way, maxspeed, maxspeed_forward, maxspeed_backward )
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
