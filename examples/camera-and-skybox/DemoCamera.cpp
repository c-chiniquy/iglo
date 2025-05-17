#include "../../src/iglo.h"
#include "../../src/iglo_camera.h"
#include "DemoCamera.h"

namespace ig
{

	void DemoCamera::Update(const IGLOContext& context, double elapsedSeconds)
	{
		oldMousePos = currentMousePos;
		currentMousePos = context.GetMousePosition();

		float travelSpeed = (float)elapsedSeconds * 10;
		if (travelSpeed < 0.001f) travelSpeed = 0.001f;
		if (travelSpeed > 10) travelSpeed = 10;

		if (context.IsKeyDown(Key::LeftShift) || context.IsKeyDown(Key::RightShift))
		{
			travelSpeed *= 20;
		}
		if (context.IsKeyDown(Key::W))
		{
			Move(GetForward() * travelSpeed);
		}
		if (context.IsKeyDown(Key::S))
		{
			Move(-GetForward() * travelSpeed);
		}
		if (context.IsKeyDown(Key::Up))
		{
			Move(GetForwardXZPlane().GetNormalized() * travelSpeed);
		}
		if (context.IsKeyDown(Key::Down))
		{
			Move(-GetForwardXZPlane().GetNormalized() * travelSpeed);
		}
		if (context.IsKeyDown(Key::Space))
		{
			Move(0, travelSpeed, 0);
		}
		if (context.IsKeyDown(Key::A) || context.IsKeyDown(Key::Left))
		{
			Move(GetLeft() * travelSpeed);
		}
		if (context.IsKeyDown(Key::D) || context.IsKeyDown(Key::Right))
		{
			Move(-GetLeft() * travelSpeed);
		}
		if (context.IsKeyDown(Key::E))
		{
			ApplyRoll(-0.2f * travelSpeed);
		}
		if (context.IsKeyDown(Key::Q))
		{
			ApplyRoll(0.2f * travelSpeed);
		}
		if (context.IsMouseButtonDown(MouseButton::Left))
		{
			ApplyYaw(-((float)(GetMouseMovement().x)) * 0.005f);
			ApplyPitch(-((float)(GetMouseMovement().y)) * 0.005f);
		}
		if (context.IsMouseButtonDown(MouseButton::Right))
		{
			float rot = ((float)(GetMouseMovement().x)) * 0.005f;
			ApplyYaw(-rot);
			Vector2 temp2 = Vector2(GetPosition().x, GetPosition().z).GetRotated(rot);
			SetPosition(temp2.x, GetPosition().y, temp2.y);
		}
		if (context.IsMouseButtonDown(MouseButton::Middle))
		{
			SetPosition(0, 0, 0);
			SetYawPitchRoll(0, 0, 0);
		}
	}

	IntPoint DemoCamera::GetMouseMovement()
	{
		return currentMousePos - oldMousePos;
	}

}