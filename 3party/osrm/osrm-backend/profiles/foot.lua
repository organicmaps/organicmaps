-- Foot profile

require("lib/access")

barrier_whitelist = { [""] = true, ["cycle_barrier"] = true, ["bollard"] = true, ["entrance"] = true, ["cattle_grid"] = true, ["border_control"] = true, ["toll_booth"] = true, ["sally_port"] = true, ["gate"] = true, ["no"] = true}
access_tag_whitelist = { ["yes"] = true, ["foot"] = true, ["permissive"] = true, ["designated"] = true  }
access_tag_blacklist = { ["no"] = true, ["private"] = true, ["agricultural"] = true, ["forestery"] = true }
access_tag_restricted = { ["destination"] = true, ["delivery"] = true }
access_tags_hierachy = { "foot", "access" }
service_tag_restricted = { ["parking_aisle"] = true }
ignore_in_grid = { ["ferry"] = true }
restriction_exception_tags = { "foot" }

walking_speed = 5

speeds = {
  ["primary"] = walking_speed,
  ["primary_link"] = walking_speed,
  ["secondary"] = walking_speed,
  ["secondary_link"] = walking_speed,
  ["tertiary"] = walking_speed,
  ["tertiary_link"] = walking_speed,
  ["unclassified"] = walking_speed,
  ["residential"] = walking_speed,
  ["road"] = walking_speed,
  ["living_street"] = walking_speed,
  ["service"] = walking_speed,
  ["track"] = walking_speed,
  ["path"] = walking_speed,
  ["steps"] = walking_speed,
  ["pedestrian"] = walking_speed,
  ["footway"] = walking_speed,
  ["pier"] = walking_speed,
  ["default"] = walking_speed
}

route_speeds = {
	["ferry"] = 5
}

platform_speeds = {
	["platform"] = walking_speed
}

amenity_speeds = {
	["parking"] = walking_speed,
	["parking_entrance"] = walking_speed
}

man_made_speeds = {
	["pier"] = walking_speed
}

surface_speeds = {
	["fine_gravel"] =   walking_speed*0.75,
	["gravel"] =        walking_speed*0.75,
	["pebbelstone"] =   walking_speed*0.75,
	["mud"] =           walking_speed*0.5,
	["sand"] =          walking_speed*0.5
}

traffic_signal_penalty 	= 2
u_turn_penalty 			= 2
use_turn_restrictions   = false

--modes
local mode_normal = 1
local mode_ferry = 2

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
		result.traffic_light = true
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

	return 1
end

function way_function (way, result)
	-- initial routability check, filters out buildings, boundaries, etc
	local highway = way:get_value_by_key("highway")
	local route = way:get_value_by_key("route")
	local man_made = way:get_value_by_key("man_made")
	local railway = way:get_value_by_key("railway")
	local amenity = way:get_value_by_key("amenity")
	local public_transport = way:get_value_by_key("public_transport")
	if (not highway or highway == '') and
		(not route or route == '') and
		(not railway or railway=='') and
		(not amenity or amenity=='') and
		(not man_made or man_made=='') and
		(not public_transport or public_transport=='')
		then
		return
    end

    -- don't route on ways that are still under construction
    if highway=='construction' then
        return
    end

	-- access
    local access = Access.find_access_tag(way, access_tags_hierachy)
    if access_tag_blacklist[access] then
		return
    end

	local name = way:get_value_by_key("name")
	local ref = way:get_value_by_key("ref")
	local junction = way:get_value_by_key("junction")
	local onewayClass = way:get_value_by_key("oneway:foot")
	local duration	= way:get_value_by_key("duration")
	local service	= way:get_value_by_key("service")
	local area = way:get_value_by_key("area")
	local foot = way:get_value_by_key("foot")
	local surface = way:get_value_by_key("surface")

 	-- name
	if ref and "" ~= ref and name and "" ~= name then
		result.name = name .. ' / ' .. ref
    elseif ref and "" ~= ref then
    	result.name = ref
	elseif name and "" ~= name then
		result.name = name
	elseif highway then
		result.name = "{highway:"..highway.."}"	-- if no name exists, use way type
		                                        -- this encoding scheme is excepted to be a temporary solution
	end

    -- roundabouts
	if "roundabout" == junction then
	  result.roundabout = true;
	end

    -- speed
    if route_speeds[route] then
		-- ferries (doesn't cover routes tagged using relations)
		result.ignore_in_grid = true
		if duration and durationIsValid(duration) then
			result.duration = math.max( 1, parseDuration(duration) )
		else
			result.forward_speed = route_speeds[route]
			result.backward_speed = route_speeds[route]
		end
		result.forward_mode = mode_ferry
		result.backward_mode = mode_ferry
	elseif railway and platform_speeds[railway] then
		-- railway platforms (old tagging scheme)
		result.forward_speed = platform_speeds[railway]
		result.backward_speed = platform_speeds[railway]
	elseif platform_speeds[public_transport] then
		-- public_transport platforms (new tagging platform)
		result.forward_speed = platform_speeds[public_transport]
		result.backward_speed = platform_speeds[public_transport]
	elseif amenity and amenity_speeds[amenity] then
		-- parking areas
		result.forward_speed = amenity_speeds[amenity]
		result.backward_speed = amenity_speeds[amenity]
	elseif speeds[highway] then
		-- regular ways
		result.forward_speed = speeds[highway]
		result.backward_speed = speeds[highway]
	elseif access and access_tag_whitelist[access] then
	    -- unknown way, but valid access tag
		result.forward_speed = walking_speed
		result.backward_speed = walking_speed
    end

	-- oneway
	if onewayClass == "yes" or onewayClass == "1" or onewayClass == "true" then
		result.backward_mode = 0
	elseif onewayClass == "no" or onewayClass == "0" or onewayClass == "false" then
		-- nothing to do
	elseif onewayClass == "-1" then
		result.forward_mode = 0
    end

    -- surfaces
    if surface then
        surface_speed = surface_speeds[surface]
        if surface_speed then
            result.forward_speed = math.min(result.forward_speed, surface_speed)
            result.backward_speed  = math.min(result.backward_speed, surface_speed)
        end
    end
end
