Vulkan shaders are updated by running this command:
```
tools/unix/generate_vulkan_shaders.sh
```

It modifies two files that should be committed in a separate `[shaders] Regenerated` commit:
- data/vulkan_shaders/reflection.json
- data/vulkan_shaders/shaders_pack.spv
