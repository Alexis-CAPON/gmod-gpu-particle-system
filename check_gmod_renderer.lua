-- Quick script to detect GMod's rendering API
-- Run this in GMod console: lua_run_cl include("check_gmod_renderer.lua")

print("========================================")
print("  GMod Renderer Detection")
print("========================================")

-- Method 1: Check render library
local renderLib = render and render.GetDXLevel and render.GetDXLevel() or "Unknown"
print("DXLevel: " .. tostring(renderLib))

-- Method 2: Check material system
local matsys = materialsystem
if matsys then
    print("Material System exists: YES")

    -- Try to get adapter info
    local adapterInfo = matsys.GetCurrentAdapter and matsys.GetCurrentAdapter() or "Unknown"
    print("Adapter: " .. tostring(adapterInfo))
end

-- Method 3: Check for specific rendering functions
print("\nRendering API Checks:")
print("- render.GetDXLevel: " .. tostring(render.GetDXLevel ~= nil))
print("- render.SupportsPixelShaders_2_0: " .. tostring(render.SupportsPixelShaders_2_0 ~= nil))

-- Method 4: Check ConVars
local mat_dxlevel = GetConVar("mat_dxlevel")
if mat_dxlevel then
    print("\nmat_dxlevel: " .. mat_dxlevel:GetInt())
end

local mat_queue_mode = GetConVar("mat_queue_mode")
if mat_queue_mode then
    print("mat_queue_mode: " .. mat_queue_mode:GetInt())
end

print("\n========================================")
print("  Instructions:")
print("========================================")
print("1. Check the DXLevel value:")
print("   - 90/95/98 = DirectX 9")
print("   - 100+ = DirectX 10/11")
print("")
print("2. Run this in console: mat_info")
print("   (This will show detailed renderer info)")
print("")
print("3. Check GMod launch options for:")
print("   -dxlevel 90/95/98 (forces DX9)")
print("   -dxlevel 100+ (forces DX10+)")
print("   -gl (forces OpenGL)")
print("========================================")
