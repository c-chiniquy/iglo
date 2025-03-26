
dxc.exe -WX -Qstrip_debug -Qstrip_reflect -T vs_6_6 -E VSMain -Vn "g_VS_InstancedRect" -Fh "compiled/VS_InstancedRect.h" "InstancedRect.hlsl"
dxc.exe -WX -Qstrip_debug -Qstrip_reflect -T vs_6_6 -E VSMain -Vn "g_VS_InstancedSprite" -Fh "compiled/VS_InstancedSprite.h" "InstancedSprite.hlsl"
dxc.exe -WX -Qstrip_debug -Qstrip_reflect -T vs_6_6 -E VSMain -Vn "g_VS_RawRect" -Fh "compiled/VS_RawRect.h" "VertexPulledRect.hlsl" -D RAW_BUFFER
dxc.exe -WX -Qstrip_debug -Qstrip_reflect -T vs_6_6 -E VSMain -Vn "g_VS_StructuredRect" -Fh "compiled/VS_StructuredRect.h" "VertexPulledRect.hlsl" -D STRUCTURED_BUFFER
dxc.exe -WX -Qstrip_debug -Qstrip_reflect -T vs_6_6 -E VSMain -Vn "g_VS_RawSprite" -Fh "compiled/VS_RawSprite.h" "VertexPulledSprite.hlsl" -D RAW_BUFFER
dxc.exe -WX -Qstrip_debug -Qstrip_reflect -T vs_6_6 -E VSMain -Vn "g_VS_StructuredSprite" -Fh "compiled/VS_StructuredSprite.h" "VertexPulledSprite.hlsl" -D STRUCTURED_BUFFER
dxc.exe -WX -Qstrip_debug -Qstrip_reflect -T ps_6_6 -E PSMain -Vn "g_PS_Color" -Fh "compiled/PS_Color.h" "PixelColor.hlsl"
dxc.exe -WX -Qstrip_debug -Qstrip_reflect -T ps_6_6 -E PSMain -Vn "g_PS_Texture" -Fh "compiled/PS_Texture.h" "PixelTexture.hlsl"
dxc.exe -WX -Qstrip_debug -Qstrip_reflect -T ps_6_6 -E PSMain -Vn "g_PS_Wobble" -Fh "compiled/PS_Wobble.h" "Wobble.hlsl"

pause
