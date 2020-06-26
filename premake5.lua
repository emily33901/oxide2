workspace "oxide2"
    configurations { "Debug", "Release" }
    platforms { "x64", "x32" }

    location "premake"
    
    filter {"system:windows"}
        characterset "MBCS"
    filter {}

    -- Set up platforms 
    filter {"platforms:x32"}
        architecture "x32"
    filter {"platforms:x64"}
        architecture "x64"
    filter {}

    -- Setup c++ spec per platform
    -- Linux uses a buildoption to allow for more
    -- up to date standards (2a)
    filter {"system:windows"}
        toolset "msc-v141"
    filter {"system:linux"}
        toolset "clang" -- prefer clang over gcc
    filter {}

    -- Setup configurations
    filter "configurations:Debug"
        defines { "DEBUG", "_DEBUG" }
        optimize "Off"

        filter {"system:windows"}
            symbols "Full"
        filter {"system:linux"}
            symbols "On"
            buildoptions "-g3" -- need this for gdb
        filter {}
        runtime "Debug"

    filter {"configurations:Release"}
        defines { "NDEBUG" }
        optimize "Full"
        symbols "Off"
        flags {"LinkTimeOptimization"}
        
    filter {}
    
    require "import"