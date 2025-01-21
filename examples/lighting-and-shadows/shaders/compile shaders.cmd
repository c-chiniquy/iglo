dxc.exe -T vs_6_6 -E VSDepth -Fo "VS_Depth.cso" "LightingAndShadows.hlsl"
dxc.exe -T vs_6_6 -E VSLightingAndShadows -Fo "VS_LightingAndShadows.cso" "LightingAndShadows.hlsl"
dxc.exe -T ps_6_6 -E PSLightingAndShadows -Fo "PS_LightingAndShadows.cso" "LightingAndShadows.hlsl"

pause
