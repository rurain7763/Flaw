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
        vcpkg_root .. "/installed/%{cfg.architecture:gsub('x86_64','x64')}-%{cfg.system}/include",
        vcpkg_root .. "/installed/%{cfg.architecture:gsub('x86_64','x64')}-%{cfg.system}/include/Imath",
        vcpkg_root .. "/installed/%{cfg.architecture:gsub('x86_64','x64')}-%{cfg.system}/include/physx",
    }
    
    links {
        "box2d.lib",
        "libmono-static-sgen.lib",
        "msdfgen-core.lib",
        "msdfgen-ext.lib",
        "msdf-atlas-gen.lib",
        "skia.dll.lib",
        "version.lib",
        "bcrypt.lib",
        "PhysX_64.lib",
        "PhysXCommon_64.lib",
        "PhysXExtensions_static_64.lib",
        "PhysXFoundation_64.lib",
        "PhysXCooking_64.lib",
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
            "libpng16d.lib",
            "freetyped.lib",
            "fmtd.lib",
            "spdlogd.lib",
            "yaml-cppd.lib",
            "fmodL_vc.lib",
            "assimp-vc143-mtd.lib",
            "OpenEXR-3_3_d.lib",
            "Imath-3_1_d.lib"
        }

    filter "configurations:Release"
        runtime "Release"
        optimize "on"   
        
        defines {
            "NDEBUG"
        }

        libdirs {
            vcpkg_root .. "/installed/%{cfg.architecture:gsub('x86_64','x64')}-%{cfg.system}/lib",
            "./Vendor/lib"
        }

        links {
            "libpng16.lib",
            "freetype.lib",
            "fmt.lib",
            "spdlog.lib",
            "yaml-cpp.lib",
            "fmod_vc.lib",
            "assimp-vc143-mt.lib",
            "OpenEXR-3_3.lib",
            "Imath-3_1.lib"
        }

