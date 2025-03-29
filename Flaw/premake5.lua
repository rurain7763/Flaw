project "Flaw"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"

    targetdir ("%{wks.location}/bin/%{cfg.buildcfg}/%{prj.name}")
    objdir ("%{wks.location}/bin-int/%{cfg.buildcfg}/%{prj.name}")

    pchheader "pch.h"
    pchsource "./src/pch.cpp"

    files {
        "./src/**.h",
        "./src/**.cpp"
    }
    
    defines {
        "FL_ENABLE_ASSERTS"
    }
    
    includedirs {
        "./src",
        "./Vendor/include",
        vcpkg_root .. "/installed/%{cfg.architecture:gsub('x86_64','x64')}-%{cfg.system}-static/include",
    }
    
    links {
        "box2d.lib",
        "libmono-static-sgen.lib",
        "version.lib",
        "bcrypt.lib"
    }

    filter "action:vs*"
        buildoptions { "/utf-8" }
    
    filter "system:windows"
        links {
            "d3d11.lib",
            "Ws2_32.lib",
            "winmm.lib",
        }

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

        libdirs {
            vcpkg_root .. "/installed/%{cfg.architecture:gsub('x86_64','x64')}-%{cfg.system}/debug/lib",
            "./Vendor/lib/debug"
        }

        links {
            "fmtd.lib",
            "spdlogd.lib",
            "yaml-cppd.lib",
        }

    filter "configurations:Release"
        runtime "Release"
        optimize "On"    

        libdirs {
            vcpkg_root .. "/installed/%{cfg.architecture:gsub('x86_64','x64')}-%{cfg.system}/lib",
            "./Vendor/lib"
        }

        links {
            "fmt.lib",
            "spdlog.lib",
            "yaml-cpp.lib",
        }

