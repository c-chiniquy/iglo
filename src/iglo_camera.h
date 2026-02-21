#pragma once

namespace ig
{

	/*
		TODO: The BaseCamera class should serve as the base class for any potential 'FirstPersonCamera', 'FlyingCamera' and 'OrbitingCamera' classes.
			  At the moment, BaseCamera probably lacks some functionality needed to make this work.
			  A quaternion should probably be used instead of yaw pitch roll for keeping track of rotation.
			  Should BaseCamera have a LookAt() function which determine rotation as well?
			  How should transition animations be implemented?
	*/

	class BaseCamera
	{
	public:
		BaseCamera() { SetProjection(640.0f / 480.0f, 90.0f, 0.07f, 700.0f); };

		void SetProjection(float aspectRatio, float fovInDegrees, float zNear, float zFar);
		void SetAspectRatio(float aspectRatio);
		void SetFOV(float fovInDegrees);
		void SetZNearAndZFar(float zNear, float zFar);

		void SetYawPitchRoll(float yaw, float pitch, float roll);
		void ApplyYaw(float yaw);
		void ApplyPitch(float pitch);
		void ApplyRoll(float roll);
		void SetPosition(Vector3 position);
		void SetPosition(float x, float y, float z);

		// Moves relative to world
		void Move(Vector3 movement);
		void Move(float x, float y, float z);

		float GetFOV();
		float GetZNear();
		float GetZFar();
		float GetAspectRatio();
		float GetYaw();
		float GetPitch();
		float GetRoll();
		Vector3 GetPosition();
		Vector3 GetUp();
		Vector3 GetForward();
		Vector3 GetForwardXZPlane();
		Vector3 GetLeft();
		Matrix4x4 GetViewMatrix();
		Matrix4x4 GetProjectionMatrix();
		Matrix4x4 GetViewProjMatrix();

	private:
		static float CapPitch(float pitch);

		Vector3 GetOriginalLeft();
		Vector3 GetOriginalUp();
		Vector3 GetOriginalForward();
		void UpdateVectors();
		void UpdateMatrices();

		Matrix4x4 view;
		Matrix4x4 proj;
		Matrix4x4 viewProj;
		Vector3 position;
		Vector3 forward;
		Vector3 forwardXZPlane;
		Vector3 left;
		Vector3 up;

		bool dirtyVectors = true;
		bool dirtyMatrices = true;

		float fovInDegrees = 0;
		float zNear = 0;
		float zFar = 0;
		float aspectRatio = 0;
		
		//TODO: Should probably use a quaternion here instead.
		float yaw = 0;
		float pitch = 0;
		float roll = 0;
	};

}