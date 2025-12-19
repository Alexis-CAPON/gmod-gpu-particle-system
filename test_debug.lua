-- Debug script to test particle loading
-- Run this in GMod console with: lua_openscript_cl test_debug.lua

print("=== Particle System Debug ===")
print("1. Checking if particles module exists...")
if particles then
    print("   ✓ particles table exists")
    print("   Functions available:", table.concat(table.GetKeys(particles), ", "))
else
    print("   ✗ particles table is nil - module not loaded!")
    return
end

print("\n2. Checking ClientParticles wrapper...")
if ClientParticles then
    print("   ✓ ClientParticles exists")
else
    print("   ✗ ClientParticles is nil")
    return
end

print("\n3. Testing direct particles.Load() call...")
local success = particles.Load("test_basic.gpart")
print("   Result:", success)

print("\n4. Checking file system...")
print("   Current directory hint: Look for particles/ relative to GarrysMod folder")
print("   The C++ module is looking for: particles/test_basic.gpart")

print("\n5. Attempting to find the file...")
-- Try to locate where the particle file should be
if file then
    local testPaths = {
        "particles/test_basic.gpart",
        "../particles/test_basic.gpart",
        "addons/starwars_particles/particles/test_basic.gpart",
    }

    for _, path in ipairs(testPaths) do
        if file.Exists(path, "GAME") then
            print("   ✓ Found at:", path, "(GAME)")
        elseif file.Exists(path, "MOD") then
            print("   ✓ Found at:", path, "(MOD)")
        end
    end
end

print("\n=== Debug Complete ===")
