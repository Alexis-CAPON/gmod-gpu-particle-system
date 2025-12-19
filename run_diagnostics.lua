--[[
    GPU Particle System - Full Diagnostic Test
    Run this in GMod with: lua_openscript_cl ../../../Dev/gpu-particle-system/run_diagnostics.lua
]]--

print("========================================")
print("  GPU Particle System - Auto Test")
print("========================================")
print("")

-- Test 1: Check if module loaded
print("[Test] Checking if particles module loaded...")
if particles then
    print("[Test] ✓ particles module loaded")
else
    print("[Test] ✗ particles module NOT loaded!")
    print("[Test] Make sure gmcl_particles_win32.dll is in garrysmod/lua/bin/")
    return
end

-- Test 2: Initialize GPU
print("[Test] Initializing DirectX 9...")
local gpuInitResult = particles.InitGPU()
print("[Test] InitGPU result:    " .. tostring(gpuInitResult))
print("")

-- Test 3: Load particle system
print("[Test] Loading test_basic.gpart...")
local fileContent = file.Read("particles/test_basic.gpart", "GAME")
if not fileContent then
    print("[Test] ✗ Could not read particles/test_basic.gpart")
    print("[Test] Make sure the file exists in: garrysmod/addons/starwars_particles/particles/")
    return
end

print("[Test] File size:    " .. #fileContent .. "    bytes")
local loadSuccess = particles.LoadFromString("test_basic", fileContent)
if loadSuccess then
    print("[Test] ✓ Particle system loaded!")
else
    print("[Test] ✗ Failed to parse particle system")
    return
end

print("")
print("========================================")
print("  Test will spawn particles in 5 seconds...")
print("========================================")
print("")

-- Test 4: Spawn particles after delay
timer.Simple(5, function()
    local ply = LocalPlayer()
    if not IsValid(ply) then
        print("[Test] ERROR: LocalPlayer not valid")
        return
    end

    local pos = ply:GetPos() + ply:GetForward() * 200
    print("[Test] Spawning particle at:    " .. pos.x .. " " .. pos.y .. " " .. pos.z)

    local instanceID = particles.Spawn("test_basic", pos, 1.0, Color(255, 255, 255, 255))

    if instanceID >= 0 then
        print("[Test] ✓ Particles spawned! Instance ID: " .. instanceID)
        print("[Test] You should see particles in front of you")
    else
        print("[Test] ERROR: Failed to spawn particles")
        print("[Test] Instance ID: " .. instanceID)
    end
end)
