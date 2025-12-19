--[[
    Server-side Particle System
    Coordinates particle spawning across all clients
]]--

-- Network string for particle spawning
util.AddNetworkString("ParticleSpawn")
util.AddNetworkString("ParticleSpawnAttached")
util.AddNetworkString("ParticleKill")

-- Particle system management
ServerParticles = ServerParticles or {}

--[[
    Spawn a particle effect for all clients
    @param effectName string - Name of the .gpart file (e.g., "explosion.gpart")
    @param position Vector - World position to spawn at
    @param scale number - Scale multiplier (optional, default 1.0)
    @param color Color - Color tint (optional, default white)
]]
function ServerParticles.Spawn(effectName, position, scale, color)
    scale = scale or 1.0
    color = color or Color(255, 255, 255, 255)

    net.Start("ParticleSpawn")
        net.WriteString(effectName)
        net.WriteVector(position)
        net.WriteFloat(scale)
        net.WriteColor(color)
    net.Broadcast()

    -- Debug output
    if GetConVar("particles_debug"):GetBool() then
        print("[ServerParticles] Spawned '" .. effectName .. "' at " .. tostring(position))
    end
end

--[[
    Spawn a particle effect for specific players
    @param effectName string - Name of the .gpart file
    @param position Vector - World position
    @param recipients table - Table of players to send to
    @param scale number - Scale multiplier (optional)
    @param color Color - Color tint (optional)
]]
function ServerParticles.SpawnForPlayers(effectName, position, recipients, scale, color)
    scale = scale or 1.0
    color = color or Color(255, 255, 255, 255)

    net.Start("ParticleSpawn")
        net.WriteString(effectName)
        net.WriteVector(position)
        net.WriteFloat(scale)
        net.WriteColor(color)
    net.Send(recipients)
end

--[[
    Spawn a particle effect attached to an entity
    @param effectName string - Name of the .gpart file
    @param entity Entity - Entity to attach to
    @param attachmentID number - Attachment point ID (optional)
    @param offset Vector - Offset from attachment (optional)
    @param scale number - Scale multiplier (optional)
    @param color Color - Color tint (optional)
]]
function ServerParticles.SpawnAttached(effectName, entity, attachmentID, offset, scale, color)
    if not IsValid(entity) then return end

    attachmentID = attachmentID or 0
    offset = offset or Vector(0, 0, 0)
    scale = scale or 1.0
    color = color or Color(255, 255, 255, 255)

    net.Start("ParticleSpawnAttached")
        net.WriteString(effectName)
        net.WriteEntity(entity)
        net.WriteUInt(attachmentID, 8)
        net.WriteVector(offset)
        net.WriteFloat(scale)
        net.WriteColor(color)
    net.Broadcast()
end

--[[
    Kill all particle effects at a position (within radius)
    @param position Vector - Center position
    @param radius number - Radius to kill particles within
]]
function ServerParticles.KillInRadius(position, radius)
    net.Start("ParticleKill")
        net.WriteVector(position)
        net.WriteFloat(radius)
    net.Broadcast()
end

-- Console commands for testing
concommand.Add("sv_particle_spawn", function(ply, cmd, args)
    if not ply:IsAdmin() then return end
    if #args < 1 then
        print("Usage: sv_particle_spawn <effect_name> [scale]")
        return
    end

    local effectName = args[1]
    local scale = tonumber(args[2]) or 1.0
    local pos = ply:GetEyeTrace().HitPos

    ServerParticles.Spawn(effectName, pos, scale)
end)

-- Example hooks for automatic particle spawning
hook.Add("EntityTakeDamage", "ParticleSystem_DamageEffects", function(target, dmginfo)
    local damage = dmginfo:GetDamage()

    -- Blood splatter on high damage
    if target:IsPlayer() or target:IsNPC() then
        if damage > 50 then
            ServerParticles.Spawn("blood_splatter_large.gpart", target:GetPos() + Vector(0, 0, 40), 1.0)
        elseif damage > 20 then
            ServerParticles.Spawn("blood_splatter_small.gpart", target:GetPos() + Vector(0, 0, 40), 0.5)
        end
    end
end)

hook.Add("OnNPCKilled", "ParticleSystem_DeathEffects", function(npc, attacker, inflictor)
    -- Death explosion for certain NPCs
    local npcClass = npc:GetClass()

    if npcClass == "npc_hunter" or npcClass == "npc_strider" then
        ServerParticles.Spawn("explosion_large.gpart", npc:GetPos(), 2.0)
    end
end)

-- Create console variable for debug mode
CreateConVar("particles_debug", "0", FCVAR_ARCHIVE, "Enable debug output for particle system")

print("[ServerParticles] Loaded successfully")
