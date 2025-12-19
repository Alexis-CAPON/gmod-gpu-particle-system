-- Particle spawn test
print("=== PARTICLE SPAWN TEST ===")
print("InitGPU result:", particles.InitGPU())

-- Spawn where you're looking, with 10x scale
local trace = LocalPlayer():GetEyeTrace()
local spawnPos = trace.HitPos
print("Spawning at:", spawnPos)

local instanceID = ClientParticles.Spawn("test_basic.gpart", spawnPos, 10.0)
print("Spawn returned ID:", instanceID)
print("Total particles:", ClientParticles.GetTotalParticleCount())

if instanceID >= 0 then
    print("SUCCESS! Particles spawned. Look at where you were aiming!")
else
    print("FAILED! Check the log file.")
end
