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
		void SetPosition(ig::Vector3 position);
		void SetPosition(float x, float y, float z);

		// Moves relative to world
		void Move(ig::Vector3 movement);
		void Move(float x, float y, float z);

		float GetFOV();
		float GetZNear();
		float GetZFar();
		float GetAspectRatio();
		float GetYaw();
		float GetPitch();
		float GetRoll();
		ig::Vector3 GetPosition();
		ig::Vector3 GetUp();
		ig::Vector3 GetForward();
		ig::Vector3 GetForwardXZPlane();
		ig::Vector3 GetLeft();
		ig::Matrix4x4 GetViewMatrix();
		ig::Matrix4x4 GetProjectionMatrix();
		ig::Matrix4x4 GetViewProjMatrix();

	private:
		static float CapPitch(float pitch);

		ig::Vector3 GetOriginalLeft();
		ig::Vector3 GetOriginalUp();
		ig::Vector3 GetOriginalForward();
		void UpdateVectors();
		void UpdateMatrices();

		ig::Matrix4x4 view;
		ig::Matrix4x4 proj;
		ig::Matrix4x4 viewProj;
		ig::Vector3 position;
		ig::Vector3 forward;
		ig::Vector3 forwardXZPlane;
		ig::Vector3 left;
		ig::Vector3 up;

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