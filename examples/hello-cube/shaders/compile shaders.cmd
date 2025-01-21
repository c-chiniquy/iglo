dxc.exe -T vs_6_6 -E VSMain -Fo "VS_HelloCube.cso" "HelloCube.hlsl"
dxc.exe -T ps_6_6 -E PSMain -Fo "PS_HelloCube.cso" "HelloCube.hlsl"

pause
