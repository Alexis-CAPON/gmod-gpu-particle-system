-- Check particle definition
local content = file.Read("particles/test_basic.gpart", "GAME")
if content then
    print("=== test_basic.gpart contents ===")
    print(content)
    print("=== END ===")
else
    print("Could not read file")
end
