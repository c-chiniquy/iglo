#pragma once

namespace ig
{
	/*
		This demo camera can be controlled with mouse and keyboard.
		It checks mouse and keyboard input in Update().
	*/

	class DemoCamera : public BaseCamera
	{
	public:
		void Update(const IGLOContext&, double elapsedSeconds);

	private:
		IntPoint GetMouseMovement();

		IntPoint oldMousePos;
		IntPoint currentMousePos;
	};
}