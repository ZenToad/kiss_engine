workspace "Box2D"
	location ( "Build/%{_ACTION}" )
	architecture "x86_64"
	configurations { "Debug", "Release" }
	
	configuration "vs*"
		defines { "_CRT_SECURE_NO_WARNINGS" }	
		
	filter "configurations:Debug"
		targetdir ( "Build/%{_ACTION}/bin/Debug" )
	 	defines { "DEBUG" }
		symbols "On"

	filter "configurations:Release"
		targetdir ( "Build/%{_ACTION}/bin/Release" )
		defines { "NDEBUG" }
		optimize "On"

project "Box2D"
	kind "StaticLib"
	language "C++"
	files { "Box2D/**.h", "Box2D/**.cpp" }
	includedirs { "." }
	
