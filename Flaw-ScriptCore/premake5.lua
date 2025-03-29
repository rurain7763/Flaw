project "Flaw-ScriptCore"
    kind "SharedLib"
    language "C#"
    dotnetframework "4.8"

    targetdir ("%{wks.location}/bin/%{cfg.buildcfg}/Flaw-Editor/Resources/Scripts")
    objdir ("%{wks.location}/bin/%{cfg.buildcfg}/Flaw-Editor/Resources/Scripts/Intermediate/%{prj.name}")

    files {
        "src/**.cs",
    }

    filter "configurations:Debug"
        optimize "off"
        symbols "default"

    filter "configurations:Release"
        optimize "on"
        symbols "default"


