--[[
    Client-side Particle System
    Receives network messages and calls binary module
]]--

-- Client particle interface
ClientParticles = ClientParticles or {}

-- Cache for loaded particle systems
local loadedSystems = {}

--[[
    Load a particle system from file
    @param effectName string - Name of the .gpart file
    @return boolean - Success
]]
function ClientParticles.Load(effectName)
    -- Check if already loaded
    if loadedSystems[effectName] then
        return true
    end

    -- Read the .gpart file using GMod's file system
    local filePath = "particles/" .. effectName
    local jsonContent = file.Read(filePath, "GAME")

    if not jsonContent then
        print("[ClientParticles] Failed to read file: " .. filePath)
        print("[ClientParticles] Make sure the file exists in: addons/starwars_particles/particles/")
        return false
    end

    print("[ClientParticles] Read " .. #jsonContent .. " bytes from " .. filePath)

    -- Call binary module with the JSON string
    local success = particles.LoadFromString(effectName, jsonContent)

    if success then
        loadedSystems[effectName] = true
        print("[ClientParticles] Loaded: " .. effectName)
    else
        print("[ClientParticles] Failed to parse: " .. effectName)
    end

    return success
end

--[[
    Spawn a particle effect locally
    @param effectName string - Name of the .gpart file
    @param position Vector - World position
    @param scale number - Scale multiplier (optional)
    @param color Color - Color tint (optional)
    @return number - Instance ID, or -1 on failure
]]
function ClientParticles.Spawn(effectName, position, scale, color)
    scale = scale or 1.0
    color = color or Color(255, 255, 255, 255)

    -- Ensure the system is loaded
    if not loadedSystems[effectName] then
        ClientParticles.Load(effectName)
    end

    -- Call binary module to spawn
    local instanceID = particles.Spawn(effectName, position, scale, color)

    if GetConVar("particles_debug"):GetBool() then
        print("[ClientParticles] Spawned '" .. effectName .. "' at " .. tostring(position) .. " (ID: " .. instanceID .. ")")
    end

    return instanceID
end

--[[
    Spawn a particle effect attached to an entity
    @param effectName string - Name of the .gpart file
    @param entity Entity - Entity to attach to
    @param attachmentID number - Attachment point ID
    @param offset Vector - Offset from attachment
    @param scale number - Scale multiplier
    @param color Color - Color tint
    @return number - Instance ID
]]
function ClientParticles.SpawnAttached(effectName, entity, attachmentID, offset, scale, color)
    if not IsValid(entity) then return -1 end

    attachmentID = attachmentID or 0
    offset = offset or Vector(0, 0, 0)
    scale = scale or 1.0
    color = color or Color(255, 255, 255, 255)

    -- Get attachment position
    local attachPos = entity:GetPos()
    if attachmentID > 0 then
        local attach = entity:GetAttachment(attachmentID)
        if attach then
            attachPos = attach.Pos
        end
    end

    attachPos = attachPos + offset

    return ClientParticles.Spawn(effectName, attachPos, scale, color)
end

--[[
    Kill a particle effect instance
    @param instanceID number - Instance ID returned from Spawn
]]
function ClientParticles.Kill(instanceID)
    particles.Kill(instanceID)
end

--[[
    Kill all particle effects in radius
    @param position Vector - Center position
    @param radius number - Radius
]]
function ClientParticles.KillInRadius(position, radius)
    particles.KillInRadius(position, radius)
end

--[[
    Get list of loaded particle systems
    @return table - Table of loaded system names
]]
function ClientParticles.GetLoadedSystems()
    local systems = {}
    for name, _ in pairs(loadedSystems) do
        table.insert(systems, name)
    end
    return systems
end

--[[
    Get total particle count across all instances
    @return number - Total particle count
]]
function ClientParticles.GetTotalParticleCount()
    return particles.GetTotalParticleCount()
end

--[[
    Get GPU time spent on particle simulation/rendering
    @return number - Time in milliseconds
]]
function ClientParticles.GetGPUTime()
    return particles.GetGPUTime()
end

-- Network message receivers
net.Receive("ParticleSpawn", function()
    local effectName = net.ReadString()
    local position = net.ReadVector()
    local scale = net.ReadFloat()
    local color = net.ReadColor()

    ClientParticles.Spawn(effectName, position, scale, color)
end)

net.Receive("ParticleSpawnAttached", function()
    local effectName = net.ReadString()
    local entity = net.ReadEntity()
    local attachmentID = net.ReadUInt(8)
    local offset = net.ReadVector()
    local scale = net.ReadFloat()
    local color = net.ReadColor()

    ClientParticles.SpawnAttached(effectName, entity, attachmentID, offset, scale, color)
end)

net.Receive("ParticleKill", function()
    local position = net.ReadVector()
    local radius = net.ReadFloat()

    ClientParticles.KillInRadius(position, radius)
end)

-- Console commands for testing
concommand.Add("cl_particle_spawn", function(ply, cmd, args)
    if #args < 1 then
        print("Usage: cl_particle_spawn <effect_name> [scale]")
        return
    end

    local effectName = args[1]
    local scale = tonumber(args[2]) or 1.0
    local pos = LocalPlayer():GetEyeTrace().HitPos

    ClientParticles.Spawn(effectName, pos, scale)
end)

concommand.Add("cl_particle_list", function()
    print("Loaded particle systems:")
    for _, name in ipairs(ClientParticles.GetLoadedSystems()) do
        print("  - " .. name)
    end
end)

concommand.Add("cl_particle_stats", function()
    print("Particle System Statistics:")
    print("  Total particles: " .. ClientParticles.GetTotalParticleCount())
    print("  GPU time: " .. ClientParticles.GetGPUTime() .. " ms")
end)

-- Preload common effects on map load
hook.Add("InitPostEntity", "ParticleSystem_Preload", function()
    -- Add your common particle effects here
    local commonEffects = {
        -- "explosion_small.gpart",
        -- "explosion_medium.gpart",
        -- "explosion_large.gpart",
        -- "muzzle_flash.gpart",
        -- "blood_splatter.gpart",
    }

    for _, effect in ipairs(commonEffects) do
        ClientParticles.Load(effect)
    end
end)

-- Create console variable
CreateClientConVar("particles_debug", "0", true, false, "Enable debug output for particle system")

-- Hook into GMod's think and render systems
hook.Add("Think", "ParticleSystem_Update", function()
    -- Update particles every frame
    -- Delta time is calculated by the C++ module
    if particles and particles.Update then
        particles.Update(FrameTime())
    end
end)

hook.Add("PostDrawOpaqueRenderables", "ParticleSystem_Render", function()
    -- Render particles after opaque world geometry but before translucent
    if not particles or not particles.Render then
        return
    end

    -- Get camera view setup
    local view = render.GetViewSetup()
    if not view then
        return
    end

    -- Validate required fields
    if not view.origin or not view.angles then
        return
    end

    -- Call C++ render function
    -- The C++ module will extract origin, angles, fov, and aspect ratio
    local success, err = pcall(function()
        particles.Render(view)
    end)

    if not success and err then
        -- Only print error once to avoid spam
        if not PARTICLE_RENDER_ERROR_PRINTED then
            print("[ClientParticles] Render error: " .. tostring(err))
            PARTICLE_RENDER_ERROR_PRINTED = true
        end
    end
end)

print("[ClientParticles] Loaded successfully")
