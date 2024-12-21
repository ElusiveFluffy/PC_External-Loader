# PC_External Loader

A Ty 1 plugin to load files from PC_External that normally would only get loaded when using the debug OpenAL32.dll, but this plugin doesn't have the downsides of using the debug OpenAL dll (Level select breaking and not being able to enter boss fights even if you have the required amount of thunder eggs)

Big thanks to Dnawrkshp's mod loader as that helped alot with getting this to work

# Build From Source

To build from source you'll need MinHook, you can install it with vcpkg with this command
```
vcpkg install minhook --triplet x86-windows-static-md
```
