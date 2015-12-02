-- Reset RNG seed to get consistent results across runs (i.e. XCode)
math.randomseed(12345)

local static = _ARGS[1] == 'static'
local action = premake.action.current()

if string.startswith(_ACTION, "vs") then
	if action then
		-- Disable solution generation
		function action.onsolution(sln)
			sln.vstudio_configs = premake.vstudio_buildconfigs(sln)
		end

		-- Rename output file
		function action.onproject(prj)
            local name = "%%_" .. _ACTION .. (static and "_static" or "")

            if static then
                for k, v in pairs(prj.project.__configs) do
                    v.objectsdir = v.objectsdir .. "Static"
                end
            end

            if _ACTION == "vs2010" then
                premake.generate(prj, name .. ".vcxproj", premake.vs2010_vcxproj)
            else
                premake.generate(prj, name .. ".vcproj", premake.vs200x_vcproj)
            end
		end
	end
elseif _ACTION == "codeblocks" then
	action.onsolution = nil

	function action.onproject(prj)
		premake.generate(prj, "%%_" .. _ACTION .. ".cbp", premake.codeblocks_cbp)
	end
elseif _ACTION == "codelite" then
	action.onsolution = nil

	function action.onproject(prj)
		premake.generate(prj, "%%_" .. _ACTION .. ".project", premake.codelite_project)
	end
end

solution "pugixml"
	objdir(_ACTION)
	targetdir(_ACTION)

if string.startswith(_ACTION, "vs") then
	if _ACTION ~= "vs2002" and _ACTION ~= "vs2003" then
		platforms { "x32", "x64" }

		configuration "x32" targetdir(_ACTION .. "/x32")
		configuration "x64" targetdir(_ACTION .. "/x64")
	end

	configurations { "Debug", "Release" }

    if static then
        configuration "Debug" targetsuffix "sd"
        configuration "Release" targetsuffix "s"
    else
        configuration "Debug" targetsuffix "d"
    end
else
	if _ACTION == "xcode3" then
		platforms "universal"
	end

	configurations { "Debug", "Release" }

	configuration "Debug" targetsuffix "d"
end

project "pugixml"
	kind "StaticLib"
	language "C++"
	files { "../src/pugixml.hpp", "../src/pugiconfig.hpp", "../src/pugixml.cpp" }
	flags { "NoPCH", "NoMinimalRebuild", "NoEditAndContinue", "Symbols" }
	uuid "89A1E353-E2DC-495C-B403-742BE206ACED"

configuration "Debug"
	defines { "_DEBUG" }

configuration "Release"
	defines { "NDEBUG" }
	flags { "Optimize" }

if static then
    configuration "*"
        flags { "StaticRuntime" }
end
