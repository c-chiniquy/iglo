#include "igloUtility.h"
#include "igloCamera.h"

namespace ig
{
	void BaseCamera::SetProjection(float aspectRatio, float fovInDegrees, float zNear, float zFar)
	{
		SetAspectRatio(aspectRatio);
		SetFOV(fovInDegrees);
		SetZNearAndZFar(zNear, zFar);
	}

	void BaseCamera::SetAspectRatio(float aspectRatio)
	{
		if (this->aspectRatio != aspectRatio) dirtyMatrices = true;
		this->aspectRatio = aspectRatio;
	}

	void BaseCamera::SetFOV(float fovInDegrees)
	{
		if (this->fovInDegrees != fovInDegrees) dirtyMatrices = true;
		this->fovInDegrees = fovInDegrees;
	}

	void BaseCamera::SetZNearAndZFar(float zNear, float zFar)
	{
		if (this->zNear != zNear) dirtyMatrices = true;
		this->zNear = zNear;

		if (this->zFar != zFar) dirtyMatrices = true;
		this->zFar = zFar;
	}

	void BaseCamera::SetYawPitchRoll(float yaw, float pitch, float roll)
	{
		dirtyMatrices = true;
		dirtyVectors = true;

		this->yaw = yaw;
		this->pitch = CapPitch(pitch);
		this->roll = roll;
	}

	void BaseCamera::ApplyYaw(float yaw)
	{
		dirtyMatrices = true;
		dirtyVectors = true;
		this->yaw += yaw;
	}

	void BaseCamera::ApplyPitch(float pitch)
	{
		dirtyMatrices = true;
		dirtyVectors = true;

		this->pitch = CapPitch(this->pitch + pitch);
	}

	void BaseCamera::ApplyRoll(float roll)
	{
		dirtyMatrices = true;
		dirtyVectors = true;
		this->roll += roll;
	}

	ig::Vector3 BaseCamera::GetOriginalLeft()
	{
		return ig::Vector3(-1, 0, 0);
	}
	ig::Vector3 BaseCamera::GetOriginalUp()
	{
		return ig::Vector3(0, 1, 0);
	}
	ig::Vector3 BaseCamera::GetOriginalForward()
	{
		return ig::Vector3(0, 0, 1);
	}

	float BaseCamera::CapPitch(float pitch)
	{
		float out = pitch;
		if (out > float(IGLO_PI / 2.0))out = float(IGLO_PI / 2.0);
		if (out < float(-IGLO_PI / 2.0))out = float(-IGLO_PI / 2.0);
		return out;
	}

	void BaseCamera::SetPosition(ig::Vector3 position)
	{
		dirtyMatrices = true;
		this->position = position;
	}

	void BaseCamera::SetPosition(float x, float y, float z)
	{
		dirtyMatrices = true;
		position = ig::Vector3(x, y, z);
	}

	void BaseCamera::Move(ig::Vector3 movement)
	{
		dirtyMatrices = true;
		position += movement;
	}

	void BaseCamera::Move(float x, float y, float z)
	{
		dirtyMatrices = true;
		position.x += x;
		position.y += y;
		position.z += z;
	}

	float BaseCamera::GetFOV()
	{
		return fovInDegrees;
	}

	float BaseCamera::GetZNear()
	{
		return zNear;
	}

	float BaseCamera::GetZFar()
	{
		return zFar;
	}

	float BaseCamera::GetAspectRatio()
	{
		return aspectRatio;
	}

	float BaseCamera::GetYaw()
	{
		return yaw;
	}

	float BaseCamera::GetPitch()
	{
		return pitch;
	}

	float BaseCamera::GetRoll()
	{
		return roll;
	}

	ig::Vector3 BaseCamera::GetPosition()
	{
		return position;
	}

	ig::Vector3 BaseCamera::GetUp()
	{
		if (dirtyVectors) UpdateVectors();
		return up;
	}

	ig::Vector3 BaseCamera::GetForward()
	{
		if (dirtyVectors) UpdateVectors();
		return forward;
	}

	ig::Vector3 BaseCamera::GetForwardXZPlane()
	{
		if (dirtyVectors) UpdateVectors();
		return forwardXZPlane;
	}

	ig::Matrix4x4 BaseCamera::GetViewMatrix()
	{
		if (dirtyMatrices) UpdateMatrices();
		return view;
	}

	ig::Matrix4x4 BaseCamera::GetProjectionMatrix()
	{
		if (dirtyMatrices) UpdateMatrices();
		return proj;
	}

	ig::Matrix4x4 BaseCamera::GetViewProjMatrix()
	{
		if (dirtyMatrices) UpdateMatrices();
		return viewProj;
	}

	ig::Vector3 BaseCamera::GetLeft()
	{
		if (dirtyVectors) UpdateVectors();
		return left;
	}

	void BaseCamera::UpdateVectors()
	{
		ig::Matrix4x4 rot;
		rot = (ig::Matrix4x4::RotationY(yaw) * ig::Matrix4x4::RotationX(pitch)) * ig::Matrix4x4::RotationZ(roll);
		forwardXZPlane = ig::Vector3::TransformCoord(GetOriginalForward(), ig::Matrix4x4::RotationY(yaw));
		forward = ig::Vector3::TransformCoord(GetOriginalForward(), rot);
		up = ig::Vector3::TransformCoord(GetOriginalUp(), rot);
		left = ig::Vector3::TransformCoord(GetOriginalLeft(), rot);
		dirtyVectors = false;
	}

	void BaseCamera::UpdateMatrices()
	{
		if (dirtyVectors) UpdateVectors();
		view = ig::Matrix4x4::LookToLH(position, forward, up);
		proj = ig::Matrix4x4::PerspectiveFovLH(aspectRatio, fovInDegrees, zNear, zFar);
		viewProj = proj * view;
		dirtyMatrices = false;
	}

}