-- Console output test
print("========================================")
print("CONSOLE TEST - If you see this, output works!")
print("========================================")

-- Test 1: Check if file exists
local content = file.Read("particles/test_basic.gpart", "GAME")
if content then
    print("[TEST] File found! Size: " .. #content .. " bytes")
    print("[TEST] First 50 chars: " .. string.sub(content, 1, 50))
else
    print("[TEST] File NOT found at: particles/test_basic.gpart")
end

-- Test 2: Check particles module
if particles then
    print("[TEST] particles module exists")
    if particles.LoadFromString then
        print("[TEST] LoadFromString function exists")
    end
else
    print("[TEST] particles module is nil!")
end

-- Test 3: Try manual load
if content and particles and particles.LoadFromString then
    print("[TEST] Attempting to load particle system...")
    local success = particles.LoadFromString("test_basic.gpart", content)
    print("[TEST] Load result: " .. tostring(success))
end

print("========================================")
print("TEST COMPLETE")
print("========================================")
