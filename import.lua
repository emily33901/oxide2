project "oxide_test"
    kind "ConsoleApp"
    language "C"
    targetdir "bin/%{cfg.platform}_%{cfg.buildcfg}/%{prj.name}"

    links {"oxide2"}

    includedirs { "include" }
    files { "test/**.h", "test/**.c", "include/**.h" }

project "oxide2"
    kind "StaticLib"
    language "C"
    targetdir "lib/%{cfg.platform}_%{cfg.buildcfg}/%{prj.name}"

    libdirs{"src/lib/RelWithDebInfo"}

    filter{"system:windows"}
        links{"OpenGL32", "dwmapi"}
    filter{}

    links{"glfw3", "glew32s.lib"}

    includedirs { "src", "include" }
    files { "src/**.h", "src/**.c", "include/**.h" }