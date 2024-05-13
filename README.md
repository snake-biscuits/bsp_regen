# `bsp_regen`

`Titanfall -> Titanfall | 2` Map Converter

Just the `.bsp`.
No audio, textures, materials, models etc.

Stable-ish physics for `.ain` generation.
The rest can come later.


## Usage

```bash
convert.exe titanfall_map.bsp titanfall2_map.bsp
```
You will need a `r1/` folder wherever you run `convert.exe`
This folder must contain all models used in the map
Most will be in the same `.vpk` as the `.bsp` (`englishclient_mp_whatever.bsp.pak000_dir.vpk`)
Some will be in the `common` `.vpk` (`englishclient_mp_common.bsp.pak000_dir.vpk`)
`bsp_regen` doesn't convert the models, but they are nessecary for map conversion (physics)


## Building

`bsp_regen` builds with CMake on the following platforms
 * Windows
   - MinGW
   - MSVC (Visual Studio Developer Command Prompt)
 * Linux
   - GNU make

Use the following commands:

```bash
cmake .
cmake --build .
```
