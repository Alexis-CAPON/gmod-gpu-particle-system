--[[
    Shared Particle List
    Registry of all available particle effects
]]--

-- Particle effect registry
PARTICLE_EFFECTS = PARTICLE_EFFECTS or {}

--[[
    Register a particle effect
    @param id string - Unique identifier
    @param filename string - .gpart filename
    @param category string - Category for organization
    @param description string - Human-readable description
]]
local function RegisterEffect(id, filename, category, description)
    PARTICLE_EFFECTS[id] = {
        id = id,
        file = filename,
        category = category,
        description = description
    }
end

-- ===========================================================================
-- Star Wars Effects Registry
-- ===========================================================================

-- Weapons
RegisterEffect("sw_blaster_red", "sw_blaster_red.gpart", "Weapons", "Red blaster bolt")
RegisterEffect("sw_blaster_blue", "sw_blaster_blue.gpart", "Weapons", "Blue blaster bolt")
RegisterEffect("sw_blaster_green", "sw_blaster_green.gpart", "Weapons", "Green blaster bolt")
RegisterEffect("sw_proton_torpedo", "sw_proton_torpedo.gpart", "Weapons", "Proton torpedo projectile")

-- Lightsabers
RegisterEffect("sw_lightsaber_clash", "sw_lightsaber_clash.gpart", "Lightsaber", "Lightsaber clash sparks")
RegisterEffect("sw_lightsaber_ignite", "sw_lightsaber_ignite.gpart", "Lightsaber", "Lightsaber ignition")
RegisterEffect("sw_lightsaber_swing", "sw_lightsaber_swing.gpart", "Lightsaber", "Lightsaber swing trail")

-- Force Powers
RegisterEffect("sw_force_push", "sw_force_push.gpart", "Force Powers", "Force push shockwave")
RegisterEffect("sw_force_lightning", "sw_force_lightning.gpart", "Force Powers", "Force lightning bolts")
RegisterEffect("sw_force_heal", "sw_force_heal.gpart", "Force Powers", "Force healing particles")
RegisterEffect("sw_force_speed", "sw_force_speed.gpart", "Force Powers", "Force speed trail")

-- Explosions
RegisterEffect("sw_explosion_small", "sw_explosion_small.gpart", "Explosions", "Small explosion")
RegisterEffect("sw_explosion_medium", "sw_explosion_medium.gpart", "Explosions", "Medium explosion")
RegisterEffect("sw_explosion_large", "sw_explosion_large.gpart", "Explosions", "Large explosion")
RegisterEffect("sw_explosion_ship", "sw_explosion_ship.gpart", "Explosions", "Ship destruction explosion")
RegisterEffect("sw_explosion_thermal", "sw_explosion_thermal.gpart", "Explosions", "Thermal detonator explosion")

-- Environmental
RegisterEffect("sw_smoke_plume", "sw_smoke_plume.gpart", "Environmental", "Rising smoke plume")
RegisterEffect("sw_fire_medium", "sw_fire_medium.gpart", "Environmental", "Medium fire")
RegisterEffect("sw_sparks_burst", "sw_sparks_burst.gpart", "Environmental", "Spark burst")
RegisterEffect("sw_steam_vent", "sw_steam_vent.gpart", "Environmental", "Steam vent")
RegisterEffect("sw_dust_cloud", "sw_dust_cloud.gpart", "Environmental", "Dust cloud")

-- Engine Trails
RegisterEffect("sw_engine_xwing", "sw_engine_xwing.gpart", "Engine Trails", "X-Wing engine trail")
RegisterEffect("sw_engine_tiefighter", "sw_engine_tiefighter.gpart", "Engine Trails", "TIE Fighter engine trail")
RegisterEffect("sw_engine_speeder", "sw_engine_speeder.gpart", "Engine Trails", "Speeder engine trail")

-- Impacts
RegisterEffect("sw_impact_metal", "sw_impact_metal.gpart", "Impacts", "Blaster impact on metal")
RegisterEffect("sw_impact_concrete", "sw_impact_concrete.gpart", "Impacts", "Blaster impact on concrete")
RegisterEffect("sw_impact_dirt", "sw_impact_dirt.gpart", "Impacts", "Blaster impact on dirt")
RegisterEffect("sw_impact_shield", "sw_impact_shield.gpart", "Impacts", "Impact on energy shield")

-- Misc
RegisterEffect("sw_teleport_in", "sw_teleport_in.gpart", "Misc", "Teleport in effect")
RegisterEffect("sw_teleport_out", "sw_teleport_out.gpart", "Misc", "Teleport out effect")
RegisterEffect("sw_hologram_flicker", "sw_hologram_flicker.gpart", "Misc", "Hologram interference")

-- ===========================================================================
-- Helper Functions
-- ===========================================================================

--[[
    Get particle effect by ID
    @param id string - Effect ID
    @return table - Effect data, or nil if not found
]]
function GetParticleEffect(id)
    return PARTICLE_EFFECTS[id]
end

--[[
    Get all particle effects in a category
    @param category string - Category name
    @return table - Array of effect data
]]
function GetParticleEffectsByCategory(category)
    local effects = {}
    for _, effect in pairs(PARTICLE_EFFECTS) do
        if effect.category == category then
            table.insert(effects, effect)
        end
    end
    return effects
end

--[[
    Get all particle effect categories
    @return table - Array of category names
]]
function GetParticleCategories()
    local categories = {}
    local seen = {}

    for _, effect in pairs(PARTICLE_EFFECTS) do
        if not seen[effect.category] then
            table.insert(categories, effect.category)
            seen[effect.category] = true
        end
    end

    table.sort(categories)
    return categories
end

-- Print loaded effects (debug)
if SERVER then
    print("[ParticleRegistry] Registered " .. table.Count(PARTICLE_EFFECTS) .. " particle effects")
end
