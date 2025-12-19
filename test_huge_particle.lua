-- Spawn a HUGE stationary particle for easy viewing
print("=== HUGE PARTICLE TEST ===")

-- Simple particle system - single burst, no movement, HUGE size
local testData = [[{
  "system": {
    "duration": 10.0,
    "looping": false,
    "startLifetime": { "mode": "Constant", "constant": 10.0 },
    "startSpeed": { "mode": "Constant", "constant": 0.0 },
    "startSize": { "mode": "Constant", "constant": 50.0 },
    "startColor": { "r": 1.0, "g": 1.0, "b": 0.0, "a": 1.0 },
    "maxParticles": 100
  },
  "emission": {
    "enabled": true,
    "rateOverTime": { "mode": "Constant", "constant": 0.0 },
    "bursts": [{ "time": 0.0, "count": 1, "repeatInterval": 0 }]
  },
  "shape": {
    "enabled": true,
    "shapeType": "Sphere",
    "radius": 0.1
  },
  "renderer": {
    "renderMode": "Billboard"
  }
}]]

particles.LoadFromString("huge_test", testData)

timer.Simple(1, function()
    local pos = LocalPlayer():GetPos() + LocalPlayer():GetForward() * 150
    pos.z = pos.z + 50  -- Raise it up

    particles.Spawn("huge_test", pos, 1.0, Color(255, 255, 0, 255))
    print("Spawned HUGE yellow particle 150 units ahead, 50 units up")
    print("Look straight ahead - you should see a BIG yellow circle!")
end)
