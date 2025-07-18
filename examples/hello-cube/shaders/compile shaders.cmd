dxc.exe -T vs_6_6 -E VSMain -Fo "VS_HelloCube.cso" "HelloCube.hlsl" -Wno-ignored-attributes
dxc.exe -T ps_6_6 -E PSMain -Fo "PS_HelloCube.cso" "HelloCube.hlsl" -Wno-ignored-attributes
dxc.exe -T vs_6_6 -E VSMain -Fo "VS_HelloCube_SPIRV.cso" "HelloCube.hlsl" -spirv -fspv-target-env=vulkan1.3 -fvk-use-dx-layout -fvk-bind-resource-heap 0 0 -fvk-bind-sampler-heap 1 0
dxc.exe -T ps_6_6 -E PSMain -Fo "PS_HelloCube_SPIRV.cso" "HelloCube.hlsl" -spirv -fspv-target-env=vulkan1.3 -fvk-use-dx-layout -fvk-bind-resource-heap 0 0 -fvk-bind-sampler-heap 1 0

pause
