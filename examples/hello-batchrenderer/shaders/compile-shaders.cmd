
dxc.exe -WX -Qstrip_debug -T vs_6_6 -E VSMain -Vn "g_VS_InstancedRect" -Fh "compiled/VS_InstancedRect.h" "InstancedRect.hlsl" -Wno-ignored-attributes -Qstrip_reflect
dxc.exe -WX -Qstrip_debug -T vs_6_6 -E VSMain -Vn "g_VS_InstancedRect" -Fh "compiled/VS_InstancedRect_SPIRV.h" "InstancedRect.hlsl" -spirv -fspv-target-env=vulkan1.3 -fvk-use-dx-layout -fvk-bind-resource-heap 0 0 -fvk-bind-sampler-heap 1 0
dxc.exe -WX -Qstrip_debug -T vs_6_6 -E VSMain -Vn "g_VS_InstancedSprite" -Fh "compiled/VS_InstancedSprite.h" "InstancedSprite.hlsl" -Wno-ignored-attributes -Qstrip_reflect
dxc.exe -WX -Qstrip_debug -T vs_6_6 -E VSMain -Vn "g_VS_InstancedSprite" -Fh "compiled/VS_InstancedSprite_SPIRV.h" "InstancedSprite.hlsl" -spirv -fspv-target-env=vulkan1.3 -fvk-use-dx-layout -fvk-bind-resource-heap 0 0 -fvk-bind-sampler-heap 1 0
dxc.exe -WX -Qstrip_debug -T vs_6_6 -E VSMain -Vn "g_VS_RawRect" -Fh "compiled/VS_RawRect.h" "VertexPulledRect.hlsl" -D RAW_BUFFER -Wno-ignored-attributes -Qstrip_reflect
dxc.exe -WX -Qstrip_debug -T vs_6_6 -E VSMain -Vn "g_VS_RawRect" -Fh "compiled/VS_RawRect_SPIRV.h" "VertexPulledRect.hlsl" -D RAW_BUFFER -spirv -fspv-target-env=vulkan1.3 -fvk-use-dx-layout -fvk-bind-resource-heap 0 0 -fvk-bind-sampler-heap 1 0
dxc.exe -WX -Qstrip_debug -T vs_6_6 -E VSMain -Vn "g_VS_StructuredRect" -Fh "compiled/VS_StructuredRect.h" "VertexPulledRect.hlsl" -D STRUCTURED_BUFFER -Wno-ignored-attributes -Qstrip_reflect
dxc.exe -WX -Qstrip_debug -T vs_6_6 -E VSMain -Vn "g_VS_StructuredRect" -Fh "compiled/VS_StructuredRect_SPIRV.h" "VertexPulledRect.hlsl" -D STRUCTURED_BUFFER -spirv -fspv-target-env=vulkan1.3 -fvk-use-dx-layout -fvk-bind-resource-heap 0 0 -fvk-bind-sampler-heap 1 0
dxc.exe -WX -Qstrip_debug -T vs_6_6 -E VSMain -Vn "g_VS_RawSprite" -Fh "compiled/VS_RawSprite.h" "VertexPulledSprite.hlsl" -D RAW_BUFFER -Wno-ignored-attributes -Qstrip_reflect
dxc.exe -WX -Qstrip_debug -T vs_6_6 -E VSMain -Vn "g_VS_RawSprite" -Fh "compiled/VS_RawSprite_SPIRV.h" "VertexPulledSprite.hlsl" -D RAW_BUFFER -spirv -fspv-target-env=vulkan1.3 -fvk-use-dx-layout -fvk-bind-resource-heap 0 0 -fvk-bind-sampler-heap 1 0
dxc.exe -WX -Qstrip_debug -T vs_6_6 -E VSMain -Vn "g_VS_StructuredSprite" -Fh "compiled/VS_StructuredSprite.h" "VertexPulledSprite.hlsl" -D STRUCTURED_BUFFER -Wno-ignored-attributes -Qstrip_reflect
dxc.exe -WX -Qstrip_debug -T vs_6_6 -E VSMain -Vn "g_VS_StructuredSprite" -Fh "compiled/VS_StructuredSprite_SPIRV.h" "VertexPulledSprite.hlsl" -D STRUCTURED_BUFFER -spirv -fspv-target-env=vulkan1.3 -fvk-use-dx-layout -fvk-bind-resource-heap 0 0 -fvk-bind-sampler-heap 1 0
dxc.exe -WX -Qstrip_debug -T ps_6_6 -E PSMain -Vn "g_PS_Color" -Fh "compiled/PS_Color.h" "PixelColor.hlsl" -Wno-ignored-attributes -Qstrip_reflect
dxc.exe -WX -Qstrip_debug -T ps_6_6 -E PSMain -Vn "g_PS_Color" -Fh "compiled/PS_Color_SPIRV.h" "PixelColor.hlsl" -spirv -fspv-target-env=vulkan1.3 -fvk-use-dx-layout -fvk-bind-resource-heap 0 0 -fvk-bind-sampler-heap 1 0
dxc.exe -WX -Qstrip_debug -T ps_6_6 -E PSMain -Vn "g_PS_Texture" -Fh "compiled/PS_Texture.h" "PixelTexture.hlsl" -Wno-ignored-attributes -Qstrip_reflect
dxc.exe -WX -Qstrip_debug -T ps_6_6 -E PSMain -Vn "g_PS_Texture" -Fh "compiled/PS_Texture_SPIRV.h" "PixelTexture.hlsl" -spirv -fspv-target-env=vulkan1.3 -fvk-use-dx-layout -fvk-bind-resource-heap 0 0 -fvk-bind-sampler-heap 1 0
dxc.exe -WX -Qstrip_debug -T ps_6_6 -E PSMain -Vn "g_PS_Wobble" -Fh "compiled/PS_Wobble.h" "Wobble.hlsl" -Wno-ignored-attributes -Qstrip_reflect
dxc.exe -WX -Qstrip_debug -T ps_6_6 -E PSMain -Vn "g_PS_Wobble" -Fh "compiled/PS_Wobble_SPIRV.h" "Wobble.hlsl" -spirv -fspv-target-env=vulkan1.3 -fvk-use-dx-layout -fvk-bind-resource-heap 0 0 -fvk-bind-sampler-heap 1 0

pause
