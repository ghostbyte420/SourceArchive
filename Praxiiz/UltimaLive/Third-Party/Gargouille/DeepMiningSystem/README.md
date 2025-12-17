# DeepMining System

[![DeepMining Demo](https://github.com/user-attachments/assets/a7c9b936-76d2-4bf6-803c-d03052577d59)](https://youtu.be/xfHA9Dwr4Ug)
--

## Installation

### Prerequisites
- [Praxiis' ULTIMA LIVE](https://github.com/ghostbyte420/SourceArchive/tree/main/Praxiiz/UltimaLive) is required.

### Steps
1. **Add Blackmap Files**
   - Place the three blackmap files into your server’s client files directory.
   - You can add multiple blackmaps by renaming them (e.g., `map34.mul`, `map35.mul`, etc.).

2. **Register Your Map**
   - In `Server/Scripts/Misc/MapDefinitions.cs`, add:
     ```csharp
     RegisterMap(34, 34, 34, 7168, 4096, 1, "MapNoire", MapRules.TrammelRules);
     ```

3. **Register for Ultima Live**
   - In `UltimaLive/Core/MapRegistry.cs`, add:
     ```csharp
     AddMapDefinition(34, 34, new Point2D(7168, 4096), new Point2D(5120, 4096));
     ```

4. **Modify Harvest Tools**
   - Edit `PickAxe.cs` (and other mining tools) and replace the `HarvestSystem` declaration with:
     ```csharp
     public override HarvestSystem HarvestSystem { get { return DeepMine.DeepMining.GetSystem(this); } }
     ```

5. **Add Scripts**
   - Place the `DeepMining` and `DynamicGumps` folders in your custom scripts directory.

6. **Register Black Maps**
   - In `DeepMining/DeepMineMapRegistry.cs`, define your maps, mines, and levels:
     ```csharp
     public static MapRegister[] MapEntries = new MapRegister[]
     {
         new MapRegister(34, 10, 10) // Map.MapID, number of mines, number of levels
     };
     ```

---

## Usage

### GM Commands
- **`DEEPGO`**: Navigate between maps, mines, and levels as a GM.
- **`SETMINE`**: Define which ore types can be found in a mine (and all its levels) while standing in the mine as a GM.

### Player Mechanics
- Players must use a **HeavyPickAxe** to dig tunnels.
- Digging may reveal holes that lead to deeper mine levels.
- Players can only harvest the base ore type (defined in `DeepMineHarvestInfo.cs`) unless they are near a spot with a different ore type.
- Ore spots spawn randomly, and their ore type is chosen randomly from the defined types.

### Manual Setup
- Add teleporters to transport players from playable maps to mine entrances.

---

## Planned Features (Beta)
- Monster spawning
- Ore spot decay (time-based or harvest-based)
- Scaled difficulty and rarity based on mine depth
- Deepmine region rules (e.g., casting, item use, light level)

---

## Notes
- This is a **beta system** and serves as a canvas for customization. Each shard owner can adapt it to their needs.
