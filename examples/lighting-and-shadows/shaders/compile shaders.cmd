dxc.exe -T vs_6_6 -E VSDepth -Fo "VS_Depth.cso" "LightingAndShadows.hlsl" -Wno-ignored-attributes
dxc.exe -T vs_6_6 -E VSLightingAndShadows -Fo "VS_LightingAndShadows.cso" "LightingAndShadows.hlsl" -Wno-ignored-attributes
dxc.exe -T ps_6_6 -E PSLightingAndShadows -Fo "PS_LightingAndShadows.cso" "LightingAndShadows.hlsl" -Wno-ignored-attributes
dxc.exe -T vs_6_6 -E VSDepth -Fo "VS_Depth_SPIRV.cso" "LightingAndShadows.hlsl" -spirv -fspv-target-env=vulkan1.3 -fvk-use-dx-layout -fvk-bind-resource-heap 0 0 -fvk-bind-sampler-heap 1 0
dxc.exe -T vs_6_6 -E VSLightingAndShadows -Fo "VS_LightingAndShadows_SPIRV.cso" "LightingAndShadows.hlsl" -spirv -fspv-target-env=vulkan1.3 -fvk-use-dx-layout -fvk-bind-resource-heap 0 0 -fvk-bind-sampler-heap 1 0
dxc.exe -T ps_6_6 -E PSLightingAndShadows -Fo "PS_LightingAndShadows_SPIRV.cso" "LightingAndShadows.hlsl" -spirv -fspv-target-env=vulkan1.3 -fvk-use-dx-layout -fvk-bind-resource-heap 0 0 -fvk-bind-sampler-heap 1 0

pause
