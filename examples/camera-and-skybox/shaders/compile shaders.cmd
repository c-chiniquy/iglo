dxc.exe -T vs_6_6 -E VSMain -Fo "VS_InstancingShader.cso" "InstancingShader.hlsl" -Wno-ignored-attributes
dxc.exe -T ps_6_6 -E PSMain -Fo "PS_InstancingShader.cso" "InstancingShader.hlsl" -Wno-ignored-attributes
dxc.exe -T vs_6_6 -E VSMain -Fo "VS_RegularShader.cso" "RegularShader.hlsl" -Wno-ignored-attributes
dxc.exe -T ps_6_6 -E PSMain -Fo "PS_RegularShader.cso" "RegularShader.hlsl" -Wno-ignored-attributes
dxc.exe -T vs_6_6 -E VSMain -Fo "VS_SkyboxShader.cso" "SkyboxShader.hlsl" -Wno-ignored-attributes
dxc.exe -T ps_6_6 -E PSMain -Fo "PS_SkyboxShader.cso" "SkyboxShader.hlsl" -Wno-ignored-attributes

dxc.exe -T vs_6_6 -E VSMain -Fo "VS_InstancingShader_SPIRV.cso" "InstancingShader.hlsl" -spirv -fspv-target-env=vulkan1.3 -fvk-use-dx-layout -fvk-bind-resource-heap 0 0 -fvk-bind-sampler-heap 1 0
dxc.exe -T ps_6_6 -E PSMain -Fo "PS_InstancingShader_SPIRV.cso" "InstancingShader.hlsl" -spirv -fspv-target-env=vulkan1.3 -fvk-use-dx-layout -fvk-bind-resource-heap 0 0 -fvk-bind-sampler-heap 1 0
dxc.exe -T vs_6_6 -E VSMain -Fo "VS_RegularShader_SPIRV.cso" "RegularShader.hlsl" -spirv -fspv-target-env=vulkan1.3 -fvk-use-dx-layout -fvk-bind-resource-heap 0 0 -fvk-bind-sampler-heap 1 0
dxc.exe -T ps_6_6 -E PSMain -Fo "PS_RegularShader_SPIRV.cso" "RegularShader.hlsl" -spirv -fspv-target-env=vulkan1.3 -fvk-use-dx-layout -fvk-bind-resource-heap 0 0 -fvk-bind-sampler-heap 1 0
dxc.exe -T vs_6_6 -E VSMain -Fo "VS_SkyboxShader_SPIRV.cso" "SkyboxShader.hlsl" -spirv -fspv-target-env=vulkan1.3 -fvk-use-dx-layout -fvk-bind-resource-heap 0 0 -fvk-bind-sampler-heap 1 0
dxc.exe -T ps_6_6 -E PSMain -Fo "PS_SkyboxShader_SPIRV.cso" "SkyboxShader.hlsl" -spirv -fspv-target-env=vulkan1.3 -fvk-use-dx-layout -fvk-bind-resource-heap 0 0 -fvk-bind-sampler-heap 1 0

pause
