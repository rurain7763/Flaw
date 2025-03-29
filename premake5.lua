vcpkg_root = os.getenv("VCPKG_ROOT")

workspace "Flaw"
    architecture "x86_64"
    configurations { "Debug", "Release" }
    startproject "Flaw-Editor"
    
    include "Flaw"
    
    include "Flaw-Editor"
    
    include "Flaw-ScriptCore"