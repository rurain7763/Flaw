project "Flaw-Editor"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"

    targetdir ("%{wks.location}/bin/%{cfg.buildcfg}/%{prj.name}")
    objdir ("%{wks.location}/bin-int/%{cfg.buildcfg}/%{prj.name}")

    files {
        "./src/**.h",
        "./src/**.cpp",
    }

    includedirs {
        "./src",
        "%{wks.location}/Flaw/src",
        "./Vendor/include",
        vcpkg_root .. "/installed/%{cfg.architecture:gsub('x86_64','x64')}-%{cfg.system}/include",
    }

    libdirs {
        "%{wks.location}/bin/%{cfg.buildcfg}/Flaw",
    }

    links {
        "Flaw.lib",
        "imgui_win32_dx11.lib"
    }

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

        libdirs {
            vcpkg_root .. "/installed/%{cfg.architecture:gsub('x86_64','x64')}-%{cfg.system}/debug/lib",
            "./Vendor/lib/debug"
        }

    filter "configurations:Release"
        runtime "Release"
        optimize "on"

        libdirs {
            vcpkg_root .. "/installed/%{cfg.architecture:gsub('x86_64','x64')}-%{cfg.system}/lib",
            "./Vendor/lib"
        }


