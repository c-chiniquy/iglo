dxc.exe -T vs_6_6 -E VSMain -Fo "VS_InstancingShader.cso" "InstancingShader.hlsl"
dxc.exe -T ps_6_6 -E PSMain -Fo "PS_InstancingShader.cso" "InstancingShader.hlsl"
dxc.exe -T vs_6_6 -E VSMain -Fo "VS_RegularShader.cso" "RegularShader.hlsl"
dxc.exe -T ps_6_6 -E PSMain -Fo "PS_RegularShader.cso" "RegularShader.hlsl"
dxc.exe -T vs_6_6 -E VSMain -Fo "VS_SkyboxShader.cso" "SkyboxShader.hlsl"
dxc.exe -T ps_6_6 -E PSMain -Fo "PS_SkyboxShader.cso" "SkyboxShader.hlsl"

pause
