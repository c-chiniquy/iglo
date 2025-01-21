#include "igloConfig.h"
#include "igloUtility.h"

#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>
#include <thread>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
// Conflicts with ig::CreateDirectory
#ifdef CreateDirectory
#undef CreateDirectory
#endif
#endif

#ifdef __linux__
#include <time.h>
#include <errno.h>
#include <cstring>
#endif

namespace ig
{
	const Quaternion Quaternion::Identity = Quaternion();
	const Matrix4x4 Matrix4x4::Identity = Matrix4x4();

	const uint32_t cp1252_codepoint_table[] = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,
		26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,
		60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,
		94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,
		121,122,123,124,125,126,127,8364,129,8218,402,8222,8230,8224,8225,710,8240,352,8249,338,141,381,143,
		144,8216,8217,8220,8221,8226,8211,8212,732,8482,353,8250,339,157,382,376,160,161,162,163,164,165,166,
		167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
		192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,
		217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,
		242,243,244,245,246,247,248,249,250,251,252,253,254,255 };

	const uint32_t cp437_codepoint_table[] = { 0,9786,9787,9829,9830,9827,9824,8226,9688,9675,9689,9794,9792,9834,
		9835,9788,9658,9668,8597,8252,182,167,9644,8616,8593,8595,8594,8592,8735,8596,9650,9660,32,33,34,35,36,37,
		38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,
		73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,
		106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,8962,199,252,233,226,
		228,224,229,231,234,235,232,239,238,236,196,197,201,230,198,244,246,242,251,249,255,214,220,162,163,165,
		8359,402,225,237,243,250,241,209,170,186,191,8976,172,189,188,161,171,187,9617,9618,9619,9474,9508,9569,
		9570,9558,9557,9571,9553,9559,9565,9564,9563,9488,9492,9524,9516,9500,9472,9532,9566,9567,9562,9556,9577,
		9574,9568,9552,9580,9575,9576,9572,9573,9561,9560,9554,9555,9579,9578,9496,9484,9608,9604,9612,9616,9600,
		945,223,915,960,931,963,181,964,934,920,937,948,8734,966,949,8745,8801,177,8805,8804,8992,8993,247,8776,176,
		8729,183,8730,8319,178,9632,160 };


	PackedBoolArray::PackedBoolArray(const PackedBoolArray& a)
	{
		*this = a;
	}

	PackedBoolArray& PackedBoolArray::operator = (const PackedBoolArray& a)
	{
		if (a.booleanCount == 0)
		{
			this->Clear();
			return *this;
		}
		uint64_t newElementCount = GetElementCount(a.booleanCount, elementSize);
		this->booleanCount = a.booleanCount;
		this->data = std::make_unique<uint8_t[]>(newElementCount);
		memcpy(this->data.get(), a.data.get(), newElementCount);
		return *this;
	}

	bool PackedBoolArray::GetAt(uint64_t index) const
	{
		const uint64_t whatElement = index / elementSize;
		const int whatBit = index % elementSize;
		const bool isTrue = (data[whatElement] & (1 << whatBit));
		return isTrue;
	}

	void PackedBoolArray::Set(uint64_t index, bool value)
	{
		if (value == true) SetTrue(index);
		else SetFalse(index);
	}

	void PackedBoolArray::SetTrue(uint64_t index)
	{
		const uint64_t whatElement = index / elementSize;
		const uint32_t whatBit = index % elementSize;
		data[whatElement] |= (1 << whatBit);
	}

	void PackedBoolArray::SetFalse(uint64_t index)
	{
		const uint64_t whatElement = index / elementSize;
		const uint32_t whatBit = index % elementSize;
		data[whatElement] &= ~(1 << whatBit);
	}

	void PackedBoolArray::Clear()
	{
		booleanCount = 0;
		data = nullptr;
	}

	void PackedBoolArray::Resize(uint64_t size, bool initialValue)
	{
		if (size == 0)
		{
			Clear();
			return;
		}
		// If the new size is the same as old size, do nothing
		if (size == booleanCount)
		{
			return;
		}
		if (booleanCount == 0)
		{
			booleanCount = size;
			data = std::make_unique<uint8_t[]>(GetElementCount(size, elementSize));
			AssignValueToAll(initialValue);
			return;
		}
		// If size is expanded...
		if (size > booleanCount)
		{
			uint64_t elemCountOld = GetElementCount(booleanCount, elementSize);
			uint64_t elemCountNew = GetElementCount(size, elementSize);

			// If there is no need to add any more elements, just assign the new values
			if (elemCountOld == elemCountNew)
			{
				for (uint64_t i = booleanCount; i < size; i++)
				{
					if (initialValue)
					{
						SetTrue(i);
					}
					else
					{
						SetFalse(i);
					}
				}
				booleanCount = size;
				return;
			}
			else
			{
				// We expand the number of elements in this array.
				std::unique_ptr<uint8_t[]> oldData = std::move(data);
				data = std::make_unique<uint8_t[]>(elemCountNew);

				// Copy over the old element values to the new array.
				memcpy(data.get(), oldData.get(), elemCountOld);

				// Fill in the values of the new elements.
				uint8_t fillValue = initialValue ? 0xff : 0;
				std::fill(data.get() + elemCountOld, data.get() + elemCountNew, fillValue);

				// Assign values to the small number of booleans sitting between the old and new elements.
				// For example, assuming 8 bits per element, if booleanCount is 7, then there is 1 bit remaining to fill in.
				uint64_t target = booleanCount + uint64_t(elementSize - (booleanCount % elementSize));
				for (uint64_t i = booleanCount; i < target; i++)
				{
					if (initialValue)
					{
						SetTrue(i);
					}
					else
					{
						SetFalse(i);
					}
				}
				booleanCount = size;
				return;
			}
		}
		// If size is reduced...
		else if (size < booleanCount)
		{
			uint64_t elemCountOld = GetElementCount(booleanCount, elementSize);
			uint64_t elemCountNew = GetElementCount(size, elementSize);

			// If there is no need to remove any elements, do nothing
			if (elemCountOld == elemCountNew)
			{
				booleanCount = size;
				return;
			}
			else
			{
				// We reduce the number of elements in this array.
				std::unique_ptr<uint8_t[]> oldData = std::move(data);
				data = std::make_unique<uint8_t[]>(elemCountNew);

				// Copy over the old element values to the new array.
				memcpy(data.get(), oldData.get(), elemCountNew);

				booleanCount = size;
				return;
			}
		}
	}

	void PackedBoolArray::AssignValueToAll(bool value)
	{
		if (booleanCount == 0) return;
		uint64_t numElements = GetElementCount(booleanCount, elementSize);
		uint8_t fillValue = value ? 0xff : 0;
		std::fill(data.get(), data.get() + numElements, fillValue);
	}

	uint64_t PackedBoolArray::GetElementCount(uint64_t numBooleans, uint32_t sizeOfEachElement)
	{
		return uint64_t(numBooleans / sizeOfEachElement) + 1;
	};

	void Timer::Reset()
	{
		t1 = std::chrono::steady_clock::now();
	}
	double Timer::GetSeconds()
	{
		return std::chrono::duration<double>(std::chrono::steady_clock::now() - t1).count();
	}
	double Timer::GetMilliseconds()
	{
		return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t1).count();
	}
	double Timer::GetMicroseconds()
	{
		return std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t1).count();
	}
	uint64_t Timer::GetNanoseconds()
	{
		return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - t1).count();
	}

	double Timer::GetSecondsAndReset()
	{
		std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();
		double elapsedTime = std::chrono::duration<double>(t2 - t1).count();
		t1 = t2; // This resets the timer
		return elapsedTime; // Return elapsed time
	}
	double Timer::GetMillisecondsAndReset()
	{
		std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();
		double elapsedTime = std::chrono::duration<double, std::milli>(t2 - t1).count();
		t1 = t2; // This resets the timer
		return elapsedTime; // Return elapsed time
	}
	double Timer::GetMicrosecondsAndReset()
	{
		std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();
		double elapsedTime = std::chrono::duration<double, std::micro>(t2 - t1).count();
		t1 = t2; // This resets the timer
		return elapsedTime; // Return elapsed time
	}
	uint64_t Timer::GetNanosecondsAndReset()
	{
		std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();
		uint64_t elapsedTime = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
		t1 = t2; // This resets the timer
		return elapsedTime; // Return elapsed time
	}

	void FrameRateLimiter::LimitFPS(double maxFramesPerSecond)
	{
		LimitElapsedMilliseconds(1000.0 / maxFramesPerSecond);
	}

	void FrameRateLimiter::LimitElapsedMilliseconds(double milliseconds)
	{
		if (!passedFirstFrame)
		{
			t1 = std::chrono::steady_clock::now() + m_dur(0);
			passedFirstFrame = true;
			return;
		}
		std::chrono::steady_clock::time_point currentMoment;
		while (true)
		{
			currentMoment = std::chrono::steady_clock::now();
			if (currentMoment >= t1)
			{
				t1 = std::chrono::steady_clock::now() + m_dur(milliseconds);
				return;
			}

			double remaining_ms = std::chrono::duration_cast<m_dur>(t1 - currentMoment).count();

			// The actual sleep duration you get varies, but small sleeps will not go above these values most of the time.
			// (i have tested multiple small sleeps VS one long sleep, and found that multiple small sleeps is more reliable)
			const double upperLimitSleepMS_1us = 0.8;
			const double upperLimitSleepMS_100us = 1.0;
			if (remaining_ms > upperLimitSleepMS_1us)
			{
				uint32_t targetSleep_us = 1;
				if (remaining_ms > upperLimitSleepMS_100us) targetSleep_us = 100;
				SleepMicroseconds(targetSleep_us);
			}
			else
			{
				// There isn't enough time left to safely sleep without risking oversleeping.
				// CPU Spinning is used for the remaining time.
				while (true)
				{
					currentMoment = std::chrono::steady_clock::now();
					if (currentMoment >= t1)
					{
						t1 = currentMoment + m_dur(milliseconds);
						return;
					}
				}
			}
		}
	}

	float Vector2::Distance(float x1, float y1, float x2, float y2)
	{
		return sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
	}
	float Vector2::Distance(const Vector2& p0, const Vector2& p1)
	{
		return sqrt((p1.x - p0.x) * (p1.x - p0.x) + (p1.y - p0.y) * (p1.y - p0.y));
	}

	Vector2 Vector2::GetRotated(float rotationInRadians) const
	{
		float s = std::sin(rotationInRadians);
		float c = std::cos(rotationInRadians);
		return Vector2((x * c) - (y * s), (x * s) + (y * c));
	}
	float Vector2::GetMagnitude() const
	{
		return sqrt((x * x) + (y * y));
	}
	float Vector2::GetSquaredMagnitude() const
	{
		return (x * x) + (y * y);
	}

	Vector2 Vector2::GetNormalized() const
	{
		Vector2 norm;
		norm.x = x;
		norm.y = y;
		float length = GetMagnitude();
		if (length > 0)
		{
			norm.x /= length;
			norm.y /= length;
		}
		return norm;
	}
	std::string Vector2::ToString() const
	{
		std::string s = "(" + std::to_string(x) + ", " + std::to_string(y) + ")";
		return s;
	}

	float Vector3::Distance(float x1, float y1, float z1, float x2, float y2, float z2)
	{
		return sqrt(
			(x1 - x2) * (x1 - x2) +
			(y1 - y2) * (y1 - y2) +
			(z1 - z2) * (z1 - z2));
	}
	float Vector3::Distance(const Vector3& p0, const Vector3& p1)
	{
		return sqrt(
			(p0.x - p1.x) * (p0.x - p1.x) +
			(p0.y - p1.y) * (p0.y - p1.y) +
			(p0.z - p1.z) * (p0.z - p1.z));
	}
	Vector3 Vector3::CrossProduct(const Vector3& a, const Vector3& b)
	{
		return Vector3((a.y * b.z) - (a.z * b.y), (a.z * b.x) - (a.x * b.z), (a.x * b.y) - (a.y * b.x));
	}
	float Vector3::DotProduct(const Vector3& a, const Vector3& b)
	{
		return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
	}
	Vector3 Vector3::CalculateNormal(const Vector3& a, const Vector3& b, const Vector3& c)
	{
		Vector3 p0 = c - a;
		Vector3 p1 = b - a;
		return CrossProduct(p0, p1).GetNormalized();
	}
	Vector3 Vector3::TransformCoord(const Vector3& v, const Matrix4x4& m)
	{
		float norm = (v.x * m.a4) + (v.y * m.b4) + (v.z * m.c4) + m.d4;
		return Vector3(
			((v.x * m.a1) + (v.y * m.b1) + (v.z * m.c1) + m.d1) / norm,
			((v.x * m.a2) + (v.y * m.b2) + (v.z * m.c2) + m.d2) / norm,
			((v.x * m.a3) + (v.y * m.b3) + (v.z * m.c3) + m.d3) / norm);
	}
	Vector3 Vector3::TransformNormal(const Vector3& v, const Matrix4x4& m)
	{
		return Vector3(
			(v.x * m.a1) + (v.y * m.b1) + (v.z * m.c1),
			(v.x * m.a2) + (v.y * m.b2) + (v.z * m.c2),
			(v.x * m.a3) + (v.y * m.b3) + (v.z * m.c3));
	}

	float Vector3::GetMagnitude() const
	{
		return sqrt((x * x) + (y * y) + (z * z));
	}
	float Vector3::GetSquaredMagnitude() const
	{
		return (x * x) + (y * y) + (z * z);
	}

	Vector3 Vector3::GetNormalized() const
	{
		Vector3 norm;
		norm.x = x;
		norm.y = y;
		norm.z = z;
		float length = GetMagnitude();
		if (length > 0)
		{
			norm.x /= length;
			norm.y /= length;
			norm.z /= length;
		}
		return norm;
	}
	std::string Vector3::ToString() const
	{
		std::string s = "(" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ")";
		return s;
	}

	Vector4 Vector4::GetNormalized() const
	{
		Vector4 norm;
		norm.x = x;
		norm.y = y;
		norm.z = z;
		norm.w = z;
		float length = GetMagnitude();
		if (length > 0)
		{
			norm.x /= length;
			norm.y /= length;
			norm.z /= length;
			norm.w /= length;
		}
		return norm;
	}
	Vector4 Vector4::Transform(const Vector4& v, const Matrix4x4& m)
	{
		return Vector4(
			(v.x * m.a1) + (v.y * m.b1) + (v.z * m.c1) + (v.w * m.d1),
			(v.x * m.a2) + (v.y * m.b2) + (v.z * m.c2) + (v.w * m.d2),
			(v.x * m.a3) + (v.y * m.b3) + (v.z * m.c3) + (v.w * m.d3),
			(v.x * m.a4) + (v.y * m.b4) + (v.z * m.c4) + (v.w * m.d4));
	}
	float Vector4::GetMagnitude() const
	{
		return sqrt((x * x) + (y * y) + (z * z) + (w * w));
	}
	float Vector4::GetSquaredMagnitude() const
	{
		return (x * x) + (y * y) + (z * z) + (w * w);
	}
	std::string Vector4::ToString() const
	{
		std::string s = "(" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ", " + std::to_string(w) + ")";
		return s;
	}

	bool Quaternion::operator == (const Quaternion& a) const
	{
		return a.x == x && a.y == y && a.z == z && a.w == w;
	}
	bool Quaternion::operator != (const Quaternion& a) const
	{
		return !operator==(a);
	}
	Quaternion& Quaternion::operator *=(const Quaternion& a)
	{
		*this = Quaternion(
			w * a.x + x * a.w + y * a.z - z * a.y,
			w * a.y - x * a.z + y * a.w + z * a.x,
			w * a.z + x * a.y - y * a.x + z * a.w,
			w * a.w - x * a.x - y * a.y - z * a.z);
		return *this;
	}
	Quaternion& Quaternion::operator *= (const float& a)
	{
		x *= a;
		y *= a;
		z *= a;
		w *= a;
		return *this;
	}
	Quaternion Quaternion::AxisAngle(const Vector3& axis, float angleRadians)
	{
		Vector3 v = axis.GetNormalized() * std::sin(0.5f * angleRadians);
		return Quaternion(v.x, v.y, v.z, cos(0.5f * angleRadians));
	}
	Quaternion Quaternion::Euler(float x, float y, float z)
	{
		Quaternion xrot = AxisAngle(Vector3(1, 0, 0), x);
		Quaternion yrot = AxisAngle(Vector3(0, 1, 0), y);
		Quaternion zrot = AxisAngle(Vector3(0, 0, 1), z);
		return yrot * xrot * zrot;
	}
	Vector3 Quaternion::GetEulerAngles() const
	{
		return Vector3(
			std::atan2(2.0f * ((w * x) + (y * z)), 1.0f - (2.0f * ((x * x) + (y * y)))),
			std::asin(2.0f * ((w * y) - (z * x))),
			std::atan2(2.0f * ((w * z) + (x * y)), 1.0f - (2.0f * ((y * y) + (z * z)))));
	}

	Quaternion Quaternion::GetNormalized() const
	{
		Quaternion norm;
		norm.x = x;
		norm.y = y;
		norm.z = z;
		norm.w = w;
		float length = sqrt((x * x) + (y * y) + (z * z) + (w * w));
		if (length > 0)
		{
			norm.x /= length;
			norm.y /= length;
			norm.z /= length;
			norm.w /= length;
		}
		return norm;
	}
	std::string Quaternion::ToString() const
	{
		std::string s = "(" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ", " + std::to_string(w) + ")";
		return s;
	}

	Matrix4x4 Matrix4x4::GetTransposed() const
	{
		return Matrix4x4(a1, b1, c1, d1, a2, b2, c2, d2, a3, b3, c3, d3, a4, b4, c4, d4);
	}
	bool Matrix4x4::operator == (const Matrix4x4& a) const
	{
		return
			a.a1 == a1 && a.a2 == a2 && a.a3 == a3 && a.a4 == a4 &&
			a.b1 == b1 && a.b2 == b2 && a.b3 == b3 && a.b4 == b4 &&
			a.c1 == c1 && a.c2 == c2 && a.c3 == c3 && a.c4 == c4 &&
			a.d1 == d1 && a.d2 == d2 && a.d3 == d3 && a.d4 == d4;
	}
	bool Matrix4x4::operator != (const Matrix4x4& a) const
	{
		return !operator==(a);
	}
	Matrix4x4& Matrix4x4::operator *=(const Matrix4x4& a)
	{
		*this = Matrix4x4(
			a.a1 * a1 + a.a2 * b1 + a.a3 * c1 + a.a4 * d1,
			a.a1 * a2 + a.a2 * b2 + a.a3 * c2 + a.a4 * d2,
			a.a1 * a3 + a.a2 * b3 + a.a3 * c3 + a.a4 * d3,
			a.a1 * a4 + a.a2 * b4 + a.a3 * c4 + a.a4 * d4,
			a.b1 * a1 + a.b2 * b1 + a.b3 * c1 + a.b4 * d1,
			a.b1 * a2 + a.b2 * b2 + a.b3 * c2 + a.b4 * d2,
			a.b1 * a3 + a.b2 * b3 + a.b3 * c3 + a.b4 * d3,
			a.b1 * a4 + a.b2 * b4 + a.b3 * c4 + a.b4 * d4,
			a.c1 * a1 + a.c2 * b1 + a.c3 * c1 + a.c4 * d1,
			a.c1 * a2 + a.c2 * b2 + a.c3 * c2 + a.c4 * d2,
			a.c1 * a3 + a.c2 * b3 + a.c3 * c3 + a.c4 * d3,
			a.c1 * a4 + a.c2 * b4 + a.c3 * c4 + a.c4 * d4,
			a.d1 * a1 + a.d2 * b1 + a.d3 * c1 + a.d4 * d1,
			a.d1 * a2 + a.d2 * b2 + a.d3 * c2 + a.d4 * d2,
			a.d1 * a3 + a.d2 * b3 + a.d3 * c3 + a.d4 * d3,
			a.d1 * a4 + a.d2 * b4 + a.d3 * c4 + a.d4 * d4);
		return *this;
	}
	Matrix4x4& Matrix4x4::operator *=(const float& a)
	{
		a1 *= a; a2 *= a; a3 *= a; a4 *= a;
		b1 *= a; b2 *= a; b3 *= a; b4 *= a;
		c1 *= a; c2 *= a; c3 *= a; c4 *= a;
		d1 *= a; d2 *= a; d3 *= a; d4 *= a;
		return *this;
	}
	Matrix4x4& Matrix4x4::operator +=(const Matrix4x4& a)
	{
		a1 += a.a1; a2 += a.a2; a3 += a.a3; a4 += a.a4;
		b1 += a.b1; b2 += a.b2; b3 += a.b3; b4 += a.b4;
		c1 += a.c1; c2 += a.c2; c3 += a.c3; c4 += a.c4;
		d1 += a.d1; d2 += a.d2; d3 += a.d3; d4 += a.d4;
		return *this;
	}
	Matrix4x4& Matrix4x4::operator -=(const Matrix4x4& a)
	{
		a1 -= a.a1; a2 -= a.a2; a3 -= a.a3; a4 -= a.a4;
		b1 -= a.b1; b2 -= a.b2; b3 -= a.b3; b4 -= a.b4;
		c1 -= a.c1; c2 -= a.c2; c3 -= a.c3; c4 -= a.c4;
		d1 -= a.d1; d2 -= a.d2; d3 -= a.d3; d4 -= a.d4;
		return *this;
	}

	Matrix4x4 Matrix4x4::Translation(const Vector3& translation)
	{
		return Matrix4x4(
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			translation.x, translation.y, translation.z, 1);
	}

	Matrix4x4 Matrix4x4::Scaling(float width, float height, float depth)
	{
		return Matrix4x4(
			width, 0, 0, 0,
			0, height, 0, 0,
			0, 0, depth, 0,
			0, 0, 0, 1);
	}

	Matrix4x4 Matrix4x4::RotationX(float radians)
	{
		return Matrix4x4(
			1, 0, 0, 0,
			0, std::cos(radians), std::sin(radians), 0,
			0, -std::sin(radians), std::cos(radians), 0,
			0, 0, 0, 1);
	}

	Matrix4x4 Matrix4x4::RotationY(float radians)
	{
		return Matrix4x4(
			std::cos(radians), 0, -std::sin(radians), 0,
			0, 1, 0, 0,
			std::sin(radians), 0, std::cos(radians), 0,
			0, 0, 0, 1);
	}

	Matrix4x4 Matrix4x4::RotationZ(float radians)
	{
		return Matrix4x4(
			std::cos(radians), std::sin(radians), 0, 0,
			-std::sin(radians), std::cos(radians), 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1);
	}

	Matrix4x4 Matrix4x4::QuaternionRotation(const Quaternion& q)
	{
		return Matrix4x4(
			1.0f - 2.0f * (q.y * q.y + q.z * q.z),
			2.0f * (q.x * q.y + q.z * q.w),
			2.0f * (q.x * q.z - q.y * q.w),
			0,

			2.0f * (q.x * q.y - q.z * q.w),
			1.0f - 2.0f * (q.x * q.x + q.z * q.z),
			2.0f * (q.y * q.z + q.x * q.w),
			0,

			2.0f * (q.x * q.z + q.y * q.w),
			2.0f * (q.y * q.z - q.x * q.w),
			1.0f - 2.0f * (q.x * q.x + q.y * q.y),
			0,

			0, 0, 0, 1);
	}

	Matrix4x4 Matrix4x4::PerspectiveFovLH(float aspectRatio, float fovInDegrees, float zNear, float zFar)
	{
		return Matrix4x4(
			(1.0f / std::tan(float(IGLO_ToRadian(fovInDegrees)) / 2.0f)) / aspectRatio, 0, 0, 0,
			0, 1.0f / std::tan(float(IGLO_ToRadian(fovInDegrees)) / 2.0f), 0, 0,
			0, 0, zFar / (zFar - zNear), 1.0f,
			0, 0, -zNear * zFar / (zFar - zNear), 0);
	}

	Matrix4x4 Matrix4x4::PerspectiveFovRH(float aspectRatio, float fovInDegrees, float zNear, float zFar)
	{
		return Matrix4x4(
			(1.0f / std::tan(float(IGLO_ToRadian(fovInDegrees)) / 2.0f)) / aspectRatio, 0, 0, 0,
			0, 1.0f / std::tan(float(IGLO_ToRadian(fovInDegrees)) / 2.0f), 0, 0,
			0, 0, zFar / (zNear - zFar), -1.0f,
			0, 0, -zNear * zFar / (zFar - zNear), 0);
	}

	Matrix4x4 Matrix4x4::LookAtLH(const Vector3& eyePosition, const Vector3& lookAt, const Vector3& up)
	{
		return LookToLH(eyePosition, (lookAt - eyePosition), up);
	}

	Matrix4x4 Matrix4x4::LookToLH(const Vector3& eyePosition, const Vector3& toDirection, const Vector3& up)
	{
		Vector3 zaxis = toDirection.GetNormalized();
		Vector3 xaxis = Vector3::CrossProduct(up, zaxis).GetNormalized();
		Vector3 yaxis = Vector3::CrossProduct(zaxis, xaxis).GetNormalized();
		return Matrix4x4(
			xaxis.x, yaxis.x, zaxis.x, 0,
			xaxis.y, yaxis.y, zaxis.y, 0,
			xaxis.z, yaxis.z, zaxis.z, 0,
			-Vector3::DotProduct(xaxis, eyePosition), -Vector3::DotProduct(yaxis, eyePosition), -Vector3::DotProduct(zaxis, eyePosition), 1);
	}

	Matrix4x4 Matrix4x4::LookAtRH(const Vector3& eyePosition, const Vector3& lookAt, const Vector3& up)
	{
		return LookToRH(eyePosition, (lookAt - eyePosition), up);
	}

	Matrix4x4 Matrix4x4::LookToRH(const Vector3& eyePosition, const Vector3& toDirection, const Vector3& up)
	{
		Vector3 zaxis = toDirection.GetNormalized();
		Vector3 xaxis = Vector3::CrossProduct(up, zaxis).GetNormalized();
		Vector3 yaxis = Vector3::CrossProduct(zaxis, xaxis).GetNormalized();
		return Matrix4x4(
			-xaxis.x, yaxis.x, -zaxis.x, 0,
			-xaxis.y, yaxis.y, -zaxis.y, 0,
			-xaxis.z, yaxis.z, -zaxis.z, 0,
			Vector3::DotProduct(xaxis, eyePosition), -Vector3::DotProduct(yaxis, eyePosition), Vector3::DotProduct(zaxis, eyePosition), 1);
	}

	Matrix4x4 Matrix4x4::OrthoLH(float width, float height, float zNear, float zFar)
	{
		return Matrix4x4(
			2.0f / width, 0, 0, 0,
			0, 2.0f / height, 0, 0,
			0, 0, 1.0f / (zFar - zNear), 0,
			0, 0, zNear / (zNear - zFar), 1.0f);
	}

	Matrix4x4 Matrix4x4::OrthoRH(float width, float height, float zNear, float zFar)
	{
		return Matrix4x4(
			2.0f / width, 0, 0, 0,
			0, 2.0f / height, 0, 0,
			0, 0, 1.0f / (zNear - zFar), 0,
			0, 0, zNear / (zNear - zFar), 1.0f);
	}

	Matrix4x4 Matrix4x4::WorldTRS(Vector3 translation, Quaternion rotation, Vector3 scale)
	{
		Matrix4x4 scaleMat = Matrix4x4::Scaling(scale.x, scale.y, scale.z);
		Matrix4x4 translationMat = Matrix4x4::Translation(translation);
		Matrix4x4 rotationMat = Matrix4x4::QuaternionRotation(rotation);
		return translationMat * rotationMat * scaleMat;
	}

	std::string Matrix4x4::ToString() const
	{
		std::string s = "[" +
			std::to_string(elements[0]) + ", " + std::to_string(elements[1]) + ", " + std::to_string(elements[2]) + ", " + std::to_string(elements[3]) + "]\n[" +
			std::to_string(elements[4]) + ", " + std::to_string(elements[5]) + ", " + std::to_string(elements[6]) + ", " + std::to_string(elements[7]) + "]\n[" +
			std::to_string(elements[8]) + ", " + std::to_string(elements[9]) + ", " + std::to_string(elements[10]) + ", " + std::to_string(elements[11]) + "]\n[" +
			std::to_string(elements[12]) + ", " + std::to_string(elements[13]) + ", " + std::to_string(elements[14]) + ", " + std::to_string(elements[15]) + "]";
		return s;
	}

	bool Extent2D::operator == (const Extent2D& a) const
	{
		return ((a.width == width) && (a.height == height));
	}
	bool Extent2D::operator != (const Extent2D& a) const
	{
		return !operator==(a);
	}
	std::string Extent2D::ToString() const
	{
		std::string s = "(" + std::to_string(width) + ", " + std::to_string(height) + ")";
		return s;
	}

	bool Extent3D::operator == (const Extent3D& a) const
	{
		return ((a.width == width) && (a.height == height) && (a.depth == depth));
	}
	bool Extent3D::operator != (const Extent3D& a) const
	{
		return !operator==(a);
	}
	std::string Extent3D::ToString() const
	{
		std::string s = "(" + std::to_string(width) + ", " + std::to_string(height) + ", " + std::to_string(depth) + ")";
		return s;
	}

	bool IntPoint::operator == (const IntPoint& a) const
	{
		return ((a.x == x) && (a.y == y));
	}
	bool IntPoint::operator != (const IntPoint& a) const
	{
		return !operator==(a);
	}
	std::string IntPoint::ToString() const
	{
		std::string s = "(" + std::to_string(x) + ", " + std::to_string(y) + ")";
		return s;
	}

	IntRect IntRect::GetNormalized() const
	{
		IntRect sorted = IntRect(left, top, right, bottom);
		if (top > bottom)
		{
			sorted.top = bottom;
			sorted.bottom = top;
		}
		if (left > right)
		{
			sorted.left = right;
			sorted.right = left;
		}
		return sorted;
	}

	bool IntRect::InclusiveContainsPoint(int32_t pointX, int32_t pointY) const
	{
		return InclusiveContainsPoint(IntPoint(pointX, pointY));
	}

	bool IntRect::InclusiveContainsPoint(IntPoint point) const
	{
		return point.x >= left && point.x <= right && point.y >= top && point.y <= bottom;
	}

	bool IntRect::ContainsPoint(int32_t pointX, int32_t pointY) const
	{
		return ContainsPoint(IntPoint(pointX, pointY));
	}

	bool IntRect::ContainsPoint(IntPoint point) const
	{
		return point.x >= left && point.x < right&& point.y >= top && point.y < bottom;
	}

	bool IntRect::ContainsRect(IntRect rect) const
	{
		return ContainsPoint(IntPoint(rect.left, rect.top)) && InclusiveContainsPoint(IntPoint(rect.right, rect.bottom));
	}

	bool IntRect::OverlapsWithRect(IntRect rect) const
	{
		return (right > rect.left && rect.right > left && bottom > rect.top && rect.bottom > top);
	}

	bool IntRect::operator == (const IntRect& a) const
	{
		return ((a.left == left) && (a.top == top) && (a.right == right) && (a.bottom == bottom));
	}

	bool IntRect::operator != (const IntRect& a) const
	{
		return !operator==(a);
	}

	bool FloatRect::InclusiveContainsPoint(float pointX, float pointY) const
	{
		return InclusiveContainsPoint(Vector2(pointX, pointY));
	}

	bool FloatRect::InclusiveContainsPoint(Vector2 point) const
	{
		return point.x >= left && point.x <= right && point.y >= top && point.y <= bottom;
	}

	bool FloatRect::ContainsPoint(float pointX, float pointY) const
	{
		return ContainsPoint(Vector2(pointX, pointY));
	}

	bool FloatRect::ContainsPoint(Vector2 point) const
	{
		return point.x >= left && point.x < right&& point.y >= top && point.y < bottom;
	}

	bool FloatRect::ContainsRect(FloatRect rect) const
	{
		return ContainsPoint(Vector2(rect.left, rect.top)) && InclusiveContainsPoint(Vector2(rect.right, rect.bottom));
	}

	bool FloatRect::OverlapsWithRect(FloatRect rect) const
	{
		return (right > rect.left && rect.right > left && bottom > rect.top && rect.bottom > top);
	}

	FloatRect FloatRect::GetNormalized() const
	{
		FloatRect sorted = FloatRect(left, top, right, bottom);
		if (top > bottom)
		{
			sorted.top = bottom;
			sorted.bottom = top;
		}
		if (left > right)
		{
			sorted.left = right;
			sorted.right = left;
		}
		return sorted;
	}

	bool FloatRect::operator == (const FloatRect& a) const
	{
		return ((a.left == left) && (a.top == top) && (a.right == right) && (a.bottom == bottom));
	}

	bool FloatRect::operator != (const FloatRect& a) const
	{
		return !operator==(a);
	}


	bool Color32::operator == (const Color32& a) const
	{
		return a.rgba == rgba;
	}
	bool Color32::operator != (const Color32& a) const
	{
		return !operator==(a);
	}


	bool Color::operator == (const Color& a) const
	{
		return ((a.red == red) && (a.green == green) && (a.blue == blue) && (a.alpha == alpha));
	}
	bool Color::operator != (const Color& a) const
	{
		return !operator==(a);
	}
	Color32 Color::ToColor32() const
	{
		float r = std::clamp(red, 0.0f, 1.0f);
		float g = std::clamp(green, 0.0f, 1.0f);
		float b = std::clamp(blue, 0.0f, 1.0f);
		float a = std::clamp(alpha, 0.0f, 1.0f);
		return Color32(
			(byte)std::roundf(r * 255.0f),
			(byte)std::roundf(g * 255.0f),
			(byte)std::roundf(b * 255.0f),
			(byte)std::roundf(a * 255.0f));
	}


	void SleepMicroseconds(uint32_t microseconds)
	{
#ifdef _WIN32
		if (microseconds <= 0) return;
		LARGE_INTEGER timeToWait;
		timeToWait.QuadPart = static_cast<int64_t>(microseconds) * -10;  // negative value for relative time
		HANDLE timerHandle = CreateWaitableTimerExW(NULL, NULL, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
		if (timerHandle)
		{
			SetWaitableTimerEx(timerHandle, &timeToWait, 0, NULL, NULL, NULL, 0);
			WaitForSingleObject(timerHandle, INFINITE);
			CloseHandle(timerHandle);
		}
#else
		struct timespec spec;
		spec.tv_sec = microseconds / 1000000;
		spec.tv_nsec = microseconds % 1000000 * 1000;
		while (nanosleep(&spec, &spec) == -1 && errno == EINTR)
		{
		}
#endif
	}

	void SleepMilliseconds(uint32_t milliseconds)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
	}

	std::u32string utf8_to_utf32(const std::string& utf8)
	{
		if (utf8.size() == 0) return std::u32string();
		std::u32string out;
		if (utf8.size() > 4) out.reserve(utf8.size() / 4);
		uint32_t codepoint;
		size_t i = 0;
		while (utf8_next_codepoint(utf8, i, utf8.size(), &i, &codepoint))
		{
			out.push_back(codepoint);
		}
		return out;
	}

	size_t utf8_length(const std::string& utf8)
	{
		size_t codePointCount = 0;
		size_t i = 0;
		while (utf8_next_codepoint(utf8, i, utf8.size(), &i, nullptr))
		{
			codePointCount++;
		}
		return codePointCount;
	}

	bool utf8_is_next_sequence_valid(const char* utf8, size_t length, size_t* out_stepForward)
	{
		// Rewritten from public domain source: https://github.com/sheredom/utf8.h/blob/master/utf8.h

		if (0xf0 == (0xf8 & ((byte)utf8[0])))
		{
			if (length < 4) return false; // ensure that there's 4 bytes or more remaining

			if ((0x80 != (0xc0 & ((byte)utf8[1]))) ||
				(0x80 != (0xc0 & ((byte)utf8[2]))) ||
				(0x80 != (0xc0 & ((byte)utf8[3])))) return false;

			if (0x80 == (0xc0 & ((byte)utf8[4]))) return false;

			if ((0 == (0x07 & ((byte)utf8[0]))) &&
				(0 == (0x30 & ((byte)utf8[1])))) return false;

			if (out_stepForward) *out_stepForward = 4; // 4-byte utf8 code point
		}
		else if (0xe0 == (0xf0 & ((byte)utf8[0])))
		{
			if (length < 3) return false; // ensure that there's 3 bytes or more remained

			if ((0x80 != (0xc0 & ((byte)utf8[1]))) ||
				(0x80 != (0xc0 & ((byte)utf8[2])))) return false;

			if (0x80 == (0xc0 & ((byte)utf8[3]))) return false;

			if ((0 == (0x0f & ((byte)utf8[0]))) &&
				(0 == (0x20 & ((byte)utf8[1])))) return false;

			if (out_stepForward) *out_stepForward = 3; // 3-byte utf8 code point
		}
		else if (0xc0 == (0xe0 & ((byte)utf8[0])))
		{
			if (length < 2) return false; // ensure that there's 2 bytes or more remained

			if (0x80 != (0xc0 & ((byte)utf8[1]))) return false;
			if (0x80 == (0xc0 & ((byte)utf8[2]))) return false;
			if (0 == (0x1e & ((byte)utf8[0]))) return false;

			if (out_stepForward) *out_stepForward = 2; // 2-byte utf8 code point
		}
		else if (0x00 == (0x80 & ((byte)utf8[0])))
		{
			if (out_stepForward) *out_stepForward = 1; // 1-byte ascii
		}
		else
		{
			return false; // we have an invalid utf8 code point entry
		}
		return true;
	}

	bool utf8_is_valid(const std::string& utf8, size_t startIndex, size_t endIndex)
	{
		return utf8_is_valid(&utf8[startIndex], endIndex - startIndex);
	}
	bool utf8_is_valid(const std::string& utf8)
	{
		return utf8_is_valid(&utf8[0], utf8.size());
	}
	bool utf8_is_valid(const char* utf8, size_t length)
	{
		size_t i = 0;
		while (i < length)
		{
			size_t stepForward = 0;
			if (!utf8_is_next_sequence_valid(&utf8[i], length - i, &stepForward)) return false;
			i += stepForward;
		}
		return true;
	}

	std::string utf8_make_valid(const std::string& utf8)
	{
		std::string out;
		size_t i = 0;
		while (i < utf8.size())
		{
			size_t stepForward = 0;
			if (utf8_is_next_sequence_valid(&utf8[i], utf8.size() - i, &stepForward))
			{
				// Include the bytes of this valid codepoint and move i forward to next codepoint
				while (stepForward > 0)
				{
					out.push_back(utf8[i]);
					stepForward--;
					i++;
				}
			}
			else
			{
				// Invalid codepoint. Skip forward 1 byte.
				i++;
			}
		}
		return out;
	}

	std::string utf8_replace_invalid(const std::string& utf8, byte asciiCharacter)
	{
		byte replacement = asciiCharacter;
		if (replacement == 0 || replacement > 127) replacement = '?';
		std::string out = utf8;
		size_t i = 0;
		while (i < out.size())
		{
			size_t stepForward = 0;
			if (utf8_is_next_sequence_valid(&out[i], out.size() - i, &stepForward))
			{
				// Valid codepoint. 'i' jumps forward to next codepoint.
				i += stepForward;
				continue;
			}
			else
			{
				// Invalid codepoint. Replace it.
				out[i] = replacement;
				// Skip forward 1 byte.
				i++;
			}
		}
		return out;
	}

	bool utf8_next_codepoint(const std::string& utf8, size_t startIndex, size_t endIndex, size_t* out_location, uint32_t* out_codepoint)
	{
		for (size_t i = startIndex; i < endIndex;)
		{
			if ((byte)utf8[i] < 0x80)
			{
				if (out_codepoint)*out_codepoint = (uint32_t)utf8[i]; // 1 byte
				if (out_location)*out_location = i + 1;
			}
			else
			{
				if (i + 1 >= endIndex) break; // cut-off
				if ((byte)utf8[i] < 0xE0) // 2 bytes
				{
					if (out_codepoint)*out_codepoint = (((byte)utf8[i] & 0x1F) << 6) | ((byte)utf8[i + 1] & 0x3F);
					if (out_location)*out_location = i + 2;
				}
				else
				{
					if (i + 2 >= endIndex) break; // cut-off
					if ((byte)utf8[i] < 0xF0) // 3 bytes
					{
						if (out_codepoint)*out_codepoint = (((byte)utf8[i] & 0x0F) << 12) |
							(((byte)utf8[i + 1] & 0x3F) << 6) |
							((byte)utf8[i + 2] & 0x3F);
						if (out_location)*out_location = i + 3;
					}
					else
					{
						if (i + 3 >= endIndex) break; // cut-off
						if ((byte)utf8[i] < 0xF8) // 4 bytes
						{
							if (out_codepoint)*out_codepoint = (((byte)utf8[i] & 0x07) << 18) |
								(((byte)utf8[i + 1] & 0x3F) << 12) |
								(((byte)utf8[i + 2] & 0x3F) << 6) |
								((byte)utf8[i + 3] & 0x3F);
							if (out_location)*out_location = i + 4;
						}
						else
						{
							i += 1;
							continue; // Invalid
						}
					}
				}
			}
			return true;
		}
		return false;
	}

	std::string utf32_to_utf8(const std::u32string& utf32)
	{
		// Rewritten from public domain source: https://github.com/JeffBezanson/cutef8/blob/master/utf8.c

		if (utf32.size() == 0) return std::string();
		std::string out;
		out.reserve(utf32.size());
		for (size_t i = 0; i < utf32.size(); i++)
		{
			uint32_t codepoint = (uint32_t)utf32[i];
			if (codepoint < 0x80)
			{
				out.push_back(codepoint);
			}
			else if (codepoint < 0x800)
			{
				out.push_back((codepoint >> 6) | 0xc0);
				out.push_back((codepoint & 0x3F) | 0x80);
			}
			else if (codepoint < 0x10000)
			{
				out.push_back((codepoint >> 12) | 0xE0);
				out.push_back(((codepoint >> 6) & 0x3F) | 0x80);
				out.push_back((codepoint & 0x3F) | 0x80);
			}
			else if (codepoint <= 0x110000)
			{
				out.push_back((codepoint >> 18) | 0xF0);
				out.push_back(((codepoint >> 12) & 0x3F) | 0x80);
				out.push_back(((codepoint >> 6) & 0x3F) | 0x80);
				out.push_back((codepoint & 0x3F) | 0x80);
			}
		}
		return out;
	}

	std::string utf32_to_utf8(uint32_t codepoint)
	{
		std::string out;
		if (codepoint < 0x80)
		{
			out.push_back(codepoint);
		}
		else if (codepoint < 0x800)
		{
			out.push_back((codepoint >> 6) | 0xc0);
			out.push_back((codepoint & 0x3F) | 0x80);
		}
		else if (codepoint < 0x10000)
		{
			out.push_back((codepoint >> 12) | 0xE0);
			out.push_back(((codepoint >> 6) & 0x3F) | 0x80);
			out.push_back((codepoint & 0x3F) | 0x80);
		}
		else if (codepoint < 0x110000)
		{
			out.push_back((codepoint >> 18) | 0xF0);
			out.push_back(((codepoint >> 12) & 0x3F) | 0x80);
			out.push_back(((codepoint >> 6) & 0x3F) | 0x80);
			out.push_back((codepoint & 0x3F) | 0x80);
		}
		return out;
	}

	std::string utf16_to_utf8(const std::wstring& utf16)
	{
		// UTF-16 functions rewritten from public domain source: https://github.com/wareya/unishim/blob/master/unishim.h

		if (utf16.size() == 0) return std::string();
		std::string utf8;
		utf8.reserve(utf16.size());

		for (size_t i = 0; i < utf16.length();)
		{
			if ((uint16_t)utf16[i] < 0xD800 || (uint16_t)utf16[i] >= 0xE000)
			{
				if ((uint16_t)utf16[i] < 0x80)
				{
					utf8.push_back((byte)utf16[i]);
				}
				else if ((uint16_t)utf16[i] < 0x800)
				{
					uint8_t low = (uint16_t)utf16[i] & 0x3F;
					uint8_t high = ((uint16_t)utf16[i] >> 6) & 0x1F;
					low |= 0x80;
					high |= 0xC0;
					utf8.push_back(high);
					utf8.push_back(low);
				}
				else
				{
					uint8_t low = (uint16_t)utf16[i] & 0x3F;
					uint8_t mid = ((uint16_t)utf16[i] >> 6) & 0x3F;
					uint8_t high = ((uint16_t)utf16[i] >> 12) & 0x0F;
					low |= 0x80;
					mid |= 0x80;
					high |= 0xE0;
					utf8.push_back(high);
					utf8.push_back(mid);
					utf8.push_back(low);
				}
				i++;
			}
			else
			{
				if (i + 1 >= utf16.length()) break; //cut-off
				uint32_t in_low = (uint16_t)utf16[i + 1] & 0x03FF;
				uint32_t in_high = (uint16_t)utf16[i] & 0x03FF;
				uint32_t codepoint = (in_high << 10) | in_low;
				codepoint += 0x10000;

				uint8_t low = codepoint & 0x3F;
				uint8_t mid = (codepoint >> 6) & 0x3F;
				uint8_t high = (codepoint >> 12) & 0x3F;
				uint8_t top = (codepoint >> 18) & 0x07;
				low |= 0x80;
				mid |= 0x80;
				high |= 0x80;
				top |= 0xF0;

				utf8.push_back(top);
				utf8.push_back(high);
				utf8.push_back(mid);
				utf8.push_back(low);

				i += 2;
			}
		}

		return utf8;
	}

	std::wstring utf16_to_wstring(const std::string& utf16, bool littleEndian)
	{
		if (utf16.size() <= 1) return std::wstring();
		std::wstring wutf16;
		if (utf16.size() > 2) wutf16.reserve(utf16.size() / 2);
		const byte* unsignedBytes = (byte*)utf16.data();
		for (size_t i = 0; i < utf16.size(); i += 2)
		{
			// If there is an uneven number of bytes in the input string, the last byte is ignored.
			if (i + 1 >= utf16.size()) break;
			uint16_t v;
			if (littleEndian)
			{
				v = ((uint16_t)unsignedBytes[i]) + (((uint16_t)unsignedBytes[i + 1]) * 0x0100);
			}
			else
			{
				v = ((uint16_t)unsignedBytes[i + 1]) + (((uint16_t)unsignedBytes[i]) * 0x0100);
			}
			wutf16.push_back((wchar_t)v);
		}
		return wutf16;
	}

	std::u32string utf32_to_u32string(const std::string& utf32, bool littleEndian)
	{
		if (utf32.size() <= 3) return std::u32string();
		std::u32string u32_utf32;
		if (utf32.size() > 4) u32_utf32.reserve(utf32.size() / 4);
		const byte* unsignedBytes = (byte*)utf32.data();
		for (size_t i = 0; i < utf32.size(); i += 4)
		{
			// If there are an extra 1-3 bytes at the end, they are ignored.
			if (i + 3 >= utf32.size()) break;
			uint32_t v;
			if (littleEndian)
			{
				v = ((uint32_t)unsignedBytes[i]) +
					(((uint32_t)unsignedBytes[i + 1]) * 0x0100) +
					(((uint32_t)unsignedBytes[i + 2]) * 0x010000) +
					(((uint32_t)unsignedBytes[i + 3]) * 0x01000000);
			}
			else
			{
				v = ((uint32_t)unsignedBytes[i + 3]) +
					(((uint32_t)unsignedBytes[i + 2]) * 0x0100) +
					(((uint32_t)unsignedBytes[i + 1]) * 0x010000) +
					(((uint32_t)unsignedBytes[i]) * 0x01000000);
			}
			u32_utf32.push_back((char32_t)v);
		}
		return u32_utf32;
	}

	std::wstring utf8_to_utf16(const std::string& utf8)
	{
		if (utf8.size() == 0) return std::wstring();
		std::wstring utf16;
		if (utf8.size() > 4) utf16.reserve(utf8.length() / 4);
		for (size_t i = 0; i < utf8.length();)
		{
			if ((byte)utf8[i] < 0x80)
			{
				utf16.push_back((byte)utf8[i]);
				i++;
			}
			else if ((byte)utf8[i] < 0xE0)
			{
				if (i + 1 >= utf8.length()) break; //cut-off
				uint16_t high = (byte)utf8[i] & 0x1F;
				uint16_t low = (byte)utf8[i + 1] & 0x3F;
				uint16_t codepoint = (high << 6) | low;
				// overlong codepoint
				if (codepoint < 0x80)
				{
					i++;
					continue;
				}
				else
				{
					utf16.push_back(codepoint);
				}
				i += 2;
			}
			else if ((byte)utf8[i] < 0xF0)
			{
				if (i + 2 >= utf8.length()) break; //cut-off
				uint16_t high = (byte)utf8[i] & 0x0F;
				uint16_t mid = (byte)utf8[i + 1] & 0x3F;
				uint16_t low = (byte)utf8[i + 2] & 0x3F;

				uint16_t codepoint = (high << 12) | (mid << 6) | low;
				// overlong codepoint
				if (codepoint < 0x800)
				{
					i++;
					continue;
				}
				// encoded surrogate
				else if (codepoint >= 0xD800 && codepoint < 0xE000)
				{
					i++;
					continue;
				}
				else
				{
					utf16.push_back(codepoint);
				}
				i += 3;
			}
			else if ((byte)utf8[i] < 0xF8)
			{
				if (i + 3 >= utf8.length()) break; //cut-off
				uint32_t top = (byte)utf8[i] & 0x07;
				uint32_t high = (byte)utf8[i + 1] & 0x3F;
				uint32_t mid = (byte)utf8[i + 2] & 0x3F;
				uint32_t low = (byte)utf8[i + 3] & 0x3F;

				uint32_t codepoint = (top << 18) | (high << 12) | (mid << 6) | low;
				// overlong codepoint
				if (codepoint < 0x10000)
				{
					i++;
					continue;
				}
				// codepoint too large for utf-16
				else if (codepoint >= 0x110000)
				{
					i++;
					continue;
				}
				else
				{
					codepoint -= 0x10000;
					uint16_t upper = codepoint >> 10;
					uint16_t lower = codepoint & 0x3FF;
					upper += 0xD800;
					lower += 0xDC00;
					utf16.push_back(upper);
					utf16.push_back(lower);
				}
				i += 4;
			}
			else
			{
				i++;
				continue;
			}
		}
		return utf16;
	}

	std::string CP1252_to_utf8(const std::string& cp1252)
	{
		if (cp1252.size() == 0) return std::string();
		std::string utf8;
		utf8.reserve(cp1252.size());
		for (size_t i = 0; i < cp1252.size(); i++)
		{
			if ((byte)cp1252[i] <= 127)
			{
				utf8.push_back(cp1252[i]);
			}
			else
			{
				utf8 += utf32_to_utf8(cp1252_codepoint_table[(byte)cp1252[i]]);
			}
		}
		return utf8;
	}

	std::string CP437_to_utf8(const std::string& cp437)
	{
		if (cp437.size() == 0) return std::string();
		std::string utf8;
		utf8.reserve(cp437.size());
		for (size_t i = 0; i < cp437.size(); i++)
		{
			if ((byte)cp437[i] >= 32 && (byte)cp437[i] <= 126)
			{
				utf8.push_back(cp437[i]);
			}
			else
			{
				utf8 += utf32_to_utf8(cp437_codepoint_table[(byte)cp437[i]]);
			}
		}
		return utf8;
	}

	std::string utf8_to_CP1252(const std::string& utf8, bool stripOutOfRangeCodepoints, byte cp1252OutOfRangeReplacement)
	{
		if (utf8.size() == 0) return std::string();
		std::string out;
		// Only reserve if it's very likely all space is used.
		// Stripping codepoints that are out of range could lead to very little of the original string remaining.
		if (utf8.size() > 4 && !stripOutOfRangeCodepoints) out.reserve(utf8.size() / 4);
		uint32_t codepoint;
		size_t i = 0;
		while (utf8_next_codepoint(utf8, i, utf8.size(), &i, &codepoint))
		{
			// In CP1252, characters from 0-127 and 160-255 match unicode codepoints directly.
			if (codepoint < 256)
			{
				if (codepoint <= 127 || codepoint >= 160)
				{
					out.push_back((byte)codepoint);
					continue;
				}
			}

			// Search the table for matching codepoint, starting at 128 to 159.
			bool found = false;
			for (uint32_t tableIndex = 128; tableIndex <= 159; tableIndex++)
			{
				if (cp1252_codepoint_table[tableIndex] == codepoint)
				{
					out.push_back((byte)tableIndex); // Found matching codepoint!
					found = true;
					break;
				}
			}
			if (found) continue;

			if (!stripOutOfRangeCodepoints)
			{
				// If this codepoint doesn't exist in CP1252, replace it with something else.
				out.push_back(cp1252OutOfRangeReplacement);
			}
		}
		return out;
	}

	std::string utf8_to_CP437(const std::string& utf8, bool stripOutOfRangeCodepoints, byte cp437OutOfRangeReplacement)
	{
		if (utf8.size() == 0) return std::string();
		std::string out;
		// Only reserve if it's very likely all space is used.
		// Stripping codepoints that are out of range could lead to very little of the original string remaining.
		if (utf8.size() > 4 && !stripOutOfRangeCodepoints) out.reserve(utf8.size() / 4);
		uint32_t codepoint;
		size_t i = 0;
		while (utf8_next_codepoint(utf8, i, utf8.size(), &i, &codepoint))
		{
			// In CP437, characters from 32-126  match unicode codepoints directly.
			if (codepoint >= 32 && codepoint <= 126)
			{
				out.push_back((byte)codepoint);
				continue;
			}

			// Search the table for matching codepoint.
			bool found = false;
			for (uint32_t tableIndex = 0; tableIndex < 256; tableIndex++)
			{
				if (codepoint >= 32 && codepoint <= 126) continue;
				if (cp437_codepoint_table[tableIndex] == codepoint)
				{
					out.push_back((byte)tableIndex); // Found matching codepoint!
					found = true;
					break;
				}
			}
			if (found) continue;

			if (!stripOutOfRangeCodepoints)
			{
				// If this codepoint doesn't exist in CP437, replace it with something else.
				out.push_back(cp437OutOfRangeReplacement);
			}
		}
		return out;
	}

	std::string GetFileExtension(const std::string& filename)
	{
		return std::filesystem::u8path(filename).extension().u8string();
	}
	bool FileExists(const std::string& filename)
	{
		return std::filesystem::exists(std::filesystem::u8path(filename));
	}
	bool DirectoryExists(const std::string& directoryName)
	{
		return std::filesystem::is_directory(std::filesystem::u8path(directoryName));
	}
	bool CreateDirectory(const std::string& directoryName)
	{
		return std::filesystem::create_directory(std::filesystem::u8path(directoryName));
	}
	std::string GetCurrentPath()
	{
		return std::filesystem::current_path().u8string();
	}

	uint32_t utf32_to_lower(uint32_t codepoint)
	{
		// case conversion functions rewritten from public domain source: https://github.com/sheredom/utf8.h/blob/master/utf8.h

		if (((0x0041 <= codepoint) && (0x005a >= codepoint)) ||
			((0x00c0 <= codepoint) && (0x00d6 >= codepoint)) ||
			((0x00d8 <= codepoint) && (0x00de >= codepoint)) ||
			((0x0391 <= codepoint) && (0x03a1 >= codepoint)) ||
			((0x03a3 <= codepoint) && (0x03ab >= codepoint)) ||
			((0x0410 <= codepoint) && (0x042f >= codepoint)))
		{
			codepoint += 32;
		}
		else if ((0x0400 <= codepoint) && (0x040f >= codepoint))
		{
			codepoint += 80;
		}
		else if (((0x0100 <= codepoint) && (0x012f >= codepoint)) ||
			((0x0132 <= codepoint) && (0x0137 >= codepoint)) ||
			((0x014a <= codepoint) && (0x0177 >= codepoint)) ||
			((0x0182 <= codepoint) && (0x0185 >= codepoint)) ||
			((0x01a0 <= codepoint) && (0x01a5 >= codepoint)) ||
			((0x01de <= codepoint) && (0x01ef >= codepoint)) ||
			((0x01f8 <= codepoint) && (0x021f >= codepoint)) ||
			((0x0222 <= codepoint) && (0x0233 >= codepoint)) ||
			((0x0246 <= codepoint) && (0x024f >= codepoint)) ||
			((0x03d8 <= codepoint) && (0x03ef >= codepoint)) ||
			((0x0460 <= codepoint) && (0x0481 >= codepoint)) ||
			((0x048a <= codepoint) && (0x04ff >= codepoint)))
		{
			codepoint |= 0x1;
		}
		else if (((0x0139 <= codepoint) && (0x0148 >= codepoint)) ||
			((0x0179 <= codepoint) && (0x017e >= codepoint)) ||
			((0x01af <= codepoint) && (0x01b0 >= codepoint)) ||
			((0x01b3 <= codepoint) && (0x01b6 >= codepoint)) ||
			((0x01cd <= codepoint) && (0x01dc >= codepoint)))
		{
			codepoint += 1;
			codepoint &= ~0x1;
		}
		else
		{
			switch (codepoint)
			{
			default:
				break;
			case 0x0178:codepoint = 0x00ff; break;
			case 0x0243:codepoint = 0x0180; break;
			case 0x018e:codepoint = 0x01dd; break;
			case 0x023d:codepoint = 0x019a; break;
			case 0x0220:codepoint = 0x019e; break;
			case 0x01b7:codepoint = 0x0292; break;
			case 0x01c4:codepoint = 0x01c6; break;
			case 0x01c7:codepoint = 0x01c9; break;
			case 0x01ca:codepoint = 0x01cc; break;
			case 0x01f1:codepoint = 0x01f3; break;
			case 0x01f7:codepoint = 0x01bf; break;
			case 0x0187:codepoint = 0x0188; break;
			case 0x018b:codepoint = 0x018c; break;
			case 0x0191:codepoint = 0x0192; break;
			case 0x0198:codepoint = 0x0199; break;
			case 0x01a7:codepoint = 0x01a8; break;
			case 0x01ac:codepoint = 0x01ad; break;
			case 0x01af:codepoint = 0x01b0; break;
			case 0x01b8:codepoint = 0x01b9; break;
			case 0x01bc:codepoint = 0x01bd; break;
			case 0x01f4:codepoint = 0x01f5; break;
			case 0x023b:codepoint = 0x023c; break;
			case 0x0241:codepoint = 0x0242; break;
			case 0x03fd:codepoint = 0x037b; break;
			case 0x03fe:codepoint = 0x037c; break;
			case 0x03ff:codepoint = 0x037d; break;
			case 0x037f:codepoint = 0x03f3; break;
			case 0x0386:codepoint = 0x03ac; break;
			case 0x0388:codepoint = 0x03ad; break;
			case 0x0389:codepoint = 0x03ae; break;
			case 0x038a:codepoint = 0x03af; break;
			case 0x038c:codepoint = 0x03cc; break;
			case 0x038e:codepoint = 0x03cd; break;
			case 0x038f:codepoint = 0x03ce; break;
			case 0x0370:codepoint = 0x0371; break;
			case 0x0372:codepoint = 0x0373; break;
			case 0x0376:codepoint = 0x0377; break;
			case 0x03f4:codepoint = 0x03b8; break;
			case 0x03cf:codepoint = 0x03d7; break;
			case 0x03f9:codepoint = 0x03f2; break;
			case 0x03f7:codepoint = 0x03f8; break;
			case 0x03fa:codepoint = 0x03fb; break;
			}
		}

		return codepoint;
	}

	uint32_t utf32_to_upper(uint32_t codepoint)
	{
		if (((0x0061 <= codepoint) && (0x007a >= codepoint)) ||
			((0x00e0 <= codepoint) && (0x00f6 >= codepoint)) ||
			((0x00f8 <= codepoint) && (0x00fe >= codepoint)) ||
			((0x03b1 <= codepoint) && (0x03c1 >= codepoint)) ||
			((0x03c3 <= codepoint) && (0x03cb >= codepoint)) ||
			((0x0430 <= codepoint) && (0x044f >= codepoint)))
		{
			codepoint -= 32;
		}
		else if ((0x0450 <= codepoint) && (0x045f >= codepoint))
		{
			codepoint -= 80;
		}
		else if (((0x0100 <= codepoint) && (0x012f >= codepoint)) ||
			((0x0132 <= codepoint) && (0x0137 >= codepoint)) ||
			((0x014a <= codepoint) && (0x0177 >= codepoint)) ||
			((0x0182 <= codepoint) && (0x0185 >= codepoint)) ||
			((0x01a0 <= codepoint) && (0x01a5 >= codepoint)) ||
			((0x01de <= codepoint) && (0x01ef >= codepoint)) ||
			((0x01f8 <= codepoint) && (0x021f >= codepoint)) ||
			((0x0222 <= codepoint) && (0x0233 >= codepoint)) ||
			((0x0246 <= codepoint) && (0x024f >= codepoint)) ||
			((0x03d8 <= codepoint) && (0x03ef >= codepoint)) ||
			((0x0460 <= codepoint) && (0x0481 >= codepoint)) ||
			((0x048a <= codepoint) && (0x04ff >= codepoint)))
		{
			codepoint &= ~0x1;
		}
		else if (((0x0139 <= codepoint) && (0x0148 >= codepoint)) ||
			((0x0179 <= codepoint) && (0x017e >= codepoint)) ||
			((0x01af <= codepoint) && (0x01b0 >= codepoint)) ||
			((0x01b3 <= codepoint) && (0x01b6 >= codepoint)) ||
			((0x01cd <= codepoint) && (0x01dc >= codepoint)))
		{
			codepoint -= 1;
			codepoint |= 0x1;
		}
		else {
			switch (codepoint)
			{
			default:
				break;
			case 0x00ff:codepoint = 0x0178; break;
			case 0x0180:codepoint = 0x0243; break;
			case 0x01dd:codepoint = 0x018e; break;
			case 0x019a:codepoint = 0x023d; break;
			case 0x019e:codepoint = 0x0220; break;
			case 0x0292:codepoint = 0x01b7; break;
			case 0x01c6:codepoint = 0x01c4; break;
			case 0x01c9:codepoint = 0x01c7; break;
			case 0x01cc:codepoint = 0x01ca; break;
			case 0x01f3:codepoint = 0x01f1; break;
			case 0x01bf:codepoint = 0x01f7; break;
			case 0x0188:codepoint = 0x0187; break;
			case 0x018c:codepoint = 0x018b; break;
			case 0x0192:codepoint = 0x0191; break;
			case 0x0199:codepoint = 0x0198; break;
			case 0x01a8:codepoint = 0x01a7; break;
			case 0x01ad:codepoint = 0x01ac; break;
			case 0x01b0:codepoint = 0x01af; break;
			case 0x01b9:codepoint = 0x01b8; break;
			case 0x01bd:codepoint = 0x01bc; break;
			case 0x01f5:codepoint = 0x01f4; break;
			case 0x023c:codepoint = 0x023b; break;
			case 0x0242:codepoint = 0x0241; break;
			case 0x037b:codepoint = 0x03fd; break;
			case 0x037c:codepoint = 0x03fe; break;
			case 0x037d:codepoint = 0x03ff; break;
			case 0x03f3:codepoint = 0x037f; break;
			case 0x03ac:codepoint = 0x0386; break;
			case 0x03ad:codepoint = 0x0388; break;
			case 0x03ae:codepoint = 0x0389; break;
			case 0x03af:codepoint = 0x038a; break;
			case 0x03cc:codepoint = 0x038c; break;
			case 0x03cd:codepoint = 0x038e; break;
			case 0x03ce:codepoint = 0x038f; break;
			case 0x0371:codepoint = 0x0370; break;
			case 0x0373:codepoint = 0x0372; break;
			case 0x0377:codepoint = 0x0376; break;
			case 0x03d1:codepoint = 0x0398; break;
			case 0x03d7:codepoint = 0x03cf; break;
			case 0x03f2:codepoint = 0x03f9; break;
			case 0x03f8:codepoint = 0x03f7; break;
			case 0x03fb:codepoint = 0x03fa; break;
			}
		}

		return codepoint;
	}
	std::u32string utf32_to_lower(const std::u32string& utf32)
	{
		std::u32string out(utf32);
		for (size_t i = 0; i < out.size(); i++)
		{
			out[i] = utf32_to_lower(utf32[i]);
		}
		return out;
	}
	std::u32string utf32_to_upper(const std::u32string& utf32)
	{
		std::u32string out(utf32);
		for (size_t i = 0; i < out.size(); i++)
		{
			out[i] = utf32_to_upper(utf32[i]);
		}
		return out;
	}

	std::string ToLowercase(const std::string& s)
	{
		return utf32_to_utf8(utf32_to_lower(utf8_to_utf32(s)));
	}
	std::string ToUppercase(const std::string& s)
	{
		return utf32_to_utf8(utf32_to_upper(utf8_to_utf32(s)));
	}

	ListDirectoryResult ListDirectory(const std::string& directoryPath, bool listSubDirectories)
	{
		ListDirectoryResult out;
		out.filenames.clear();
		out.foldernames.clear();
		out.success = false;
		try
		{
			if (listSubDirectories)
			{
				for (const auto& entry : std::filesystem::recursive_directory_iterator(std::filesystem::u8path(directoryPath)))
				{
					if (entry.is_directory())
					{
						out.foldernames.push_back(entry.path().u8string());
					}
					else
					{
						out.filenames.push_back(entry.path().u8string());
					}

				}
			}
			else
			{
				for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::u8path(directoryPath)))
				{
					if (entry.is_directory())
					{
						out.foldernames.push_back(entry.path().u8string());
					}
					else
					{
						out.filenames.push_back(entry.path().u8string());
					}
				}
			}
		}
		catch (...)
		{
			out.filenames.clear();
			out.foldernames.clear();
			out.success = false;
			return out;
		}
		out.success = true;
		return out;
	}

	enum class ByteOrderMark
	{
		None = 0,
		UTF8,
		UTF16LE,
		UTF16BE,

		// The BOM for UTF16 and UTF32 are ambiguous, and as a result,
		// if BOM for UTF32 is detected it's possible that the BOM
		// is actually UTF16 followed by a null character.
		UTF32LE_Or_UTF16LE_WithNullChar,
		UTF32BE_Or_UTF16BE_WithNullChar,
	};

	// Looks for a byte order mark in the contents of a file.
	ByteOrderMark GetByteOrderMark(const byte* fileContents, std::size_t numBytes)
	{
		if (numBytes >= 4)
		{
			// UTF-32 LE
			if (fileContents[0] == 0xFF &&
				fileContents[1] == 0xFE &&
				fileContents[2] == 0x00 &&
				fileContents[3] == 0x00)
			{
				// This is either UTF-32 LE or UTF16-LE followed by a null character.
				return ByteOrderMark::UTF32LE_Or_UTF16LE_WithNullChar;
			}
			// UTF-32 BE
			if (fileContents[0] == 0x00 &&
				fileContents[1] == 0x00 &&
				fileContents[2] == 0xFE &&
				fileContents[3] == 0xFF)
			{
				// This is either UTF-32 BE or UTF16-BE followed by a null character.
				return ByteOrderMark::UTF32BE_Or_UTF16BE_WithNullChar;
			}
		}
		if (numBytes >= 3)
		{
			// UTF-8
			if (fileContents[0] == 0xEF &&
				fileContents[1] == 0xBB &&
				fileContents[2] == 0xBF)
			{
				return ByteOrderMark::UTF8;
			}
		}
		if (numBytes > 2)
		{
			// UTF-16 LE
			if (fileContents[0] == 0xFF &&
				fileContents[1] == 0xFE)
			{
				return ByteOrderMark::UTF16LE;
			}
			// UTF-16 BE
			if (fileContents[0] == 0xFE &&
				fileContents[1] == 0xFF)
			{
				return ByteOrderMark::UTF16BE;
			}
		}
		return ByteOrderMark::None;
	}
	uint32_t GetByteOrderMarkLength(ByteOrderMark bom)
	{
		if (bom == ByteOrderMark::UTF8) return 3;
		else if (bom == ByteOrderMark::UTF16LE) return 2;
		else if (bom == ByteOrderMark::UTF16BE) return 2;
		else if (bom == ByteOrderMark::UTF32LE_Or_UTF16LE_WithNullChar) return 4;
		else if (bom == ByteOrderMark::UTF32BE_Or_UTF16BE_WithNullChar) return 4;
		return 0;
	}

	ReadTextFileResult ReadTextFile(const std::string& filename, Encoding encoding)
	{
		ReadTextFileResult out;
		out.success = false;

		ReadFileResult in = ReadFile(filename);
		if (!in.success) return out;

		if (in.fileContent.size() == 0)
		{
			// Length of file is 0. Still counts as success.
			out.success = true;
			return out;
		}

		ByteOrderMark bom = GetByteOrderMark(in.fileContent.data(), in.fileContent.size());
		Encoding determinedEncoding = encoding;

		// Unknown encoding.
		if (determinedEncoding == Encoding::Unknown)
		{
			// UTF-8 BOM
			if (bom == ByteOrderMark::UTF8)
			{
				determinedEncoding = Encoding::UTF8;
			}
			else
			{
				// UTF-16 LE BOM
				if (bom == ByteOrderMark::UTF16LE ||
					bom == ByteOrderMark::UTF32LE_Or_UTF16LE_WithNullChar)
				{
					determinedEncoding = Encoding::UTF16_LE;
				}
				else
				{
					// UTF-16 BE BOM
					if (bom == ByteOrderMark::UTF16BE ||
						bom == ByteOrderMark::UTF32BE_Or_UTF16BE_WithNullChar)
					{
						determinedEncoding = Encoding::UTF16_BE;
					}
					else
					{
						// No relevant BOM found. Test UTF-8 validity.
						if (utf8_is_valid((const char*)in.fileContent.data(), in.fileContent.size()))
						{
							determinedEncoding = Encoding::UTF8;
						}
						else
						{
							// As a last resort, assume it's CP1252.
							determinedEncoding = Encoding::CP1252;
						}
					}
				}
			}
		}

		uint32_t skipBytes = 0; // How many bytes to skip (for skipping byte order mark)

		if (determinedEncoding == Encoding::UTF32_LE)
		{
			// Has byte order mark
			if (bom == ByteOrderMark::UTF32LE_Or_UTF16LE_WithNullChar) skipBytes = GetByteOrderMarkLength(bom);
			// Remove byte order mark if exists
			out.text = std::string((char*)&in.fileContent[skipBytes], in.fileContent.size() - skipBytes);
			// Convert UTF-32 LE --> u32string --> UTF-8
			out.text = utf32_to_utf8(utf32_to_u32string(out.text, true));
			out.success = true;
			return out;
		}
		else if (determinedEncoding == Encoding::UTF32_BE)
		{
			// Has byte order mark
			if (bom == ByteOrderMark::UTF32BE_Or_UTF16BE_WithNullChar) skipBytes = GetByteOrderMarkLength(bom);
			// Remove byte order mark if exists
			out.text = std::string((char*)&in.fileContent[skipBytes], in.fileContent.size() - skipBytes);
			// Convert UTF-32 BE --> u32string --> UTF-8
			out.text = utf32_to_utf8(utf32_to_u32string(out.text, false));
			out.success = true;
			return out;
		}
		else if (determinedEncoding == Encoding::UTF16_LE)
		{
			// Has byte order mark
			if (bom == ByteOrderMark::UTF16LE) skipBytes = GetByteOrderMarkLength(bom);
			// Remove byte order mark if exists
			out.text = std::string((char*)&in.fileContent[skipBytes], in.fileContent.size() - skipBytes);
			// Convert UTF-16 LE --> wstring --> UTF-8
			out.text = utf16_to_utf8(utf16_to_wstring(out.text, true));
			out.success = true;
			return out;
		}
		else if (determinedEncoding == Encoding::UTF16_BE)
		{
			// Has byte order mark
			if (bom == ByteOrderMark::UTF16BE) skipBytes = GetByteOrderMarkLength(bom);
			// Remove byte order mark if exists
			out.text = std::string((char*)&in.fileContent[skipBytes], in.fileContent.size() - skipBytes);
			// Convert UTF-16 BE --> wstring --> UTF-8
			out.text = utf16_to_utf8(utf16_to_wstring(out.text, false));
			out.success = true;
			return out;
		}
		else if (determinedEncoding == Encoding::UTF8)
		{
			// Has byte order mark
			if (bom == ByteOrderMark::UTF8) skipBytes = GetByteOrderMarkLength(bom);
			// Remove byte order mark if exists
			out.text = std::string((char*)&in.fileContent[skipBytes], in.fileContent.size() - skipBytes);
			// It's already UTF-8, so nothing more needs to be done.
			out.success = true;
			return out;
		}
		else if (determinedEncoding == Encoding::CP437)
		{
			// Include all bytes
			out.text = std::string((char*)in.fileContent.data(), in.fileContent.size());
			// Convert to CP437
			out.text = CP437_to_utf8(out.text);
			out.success = true;
			return out;
		}
		else
		{
			// CP1252

			// Include all bytes
			out.text = std::string((char*)in.fileContent.data(), in.fileContent.size());
			// Convert to Windows-1252 (aka CP1252).
			out.text = CP1252_to_utf8(out.text);
			out.success = true;
			return out;
		}
	}

	ReadFileResult ReadFile(const std::string& filename)
	{
		ReadFileResult out;
		out.success = false;
		std::ifstream f(std::filesystem::u8path(filename), std::ios::in | std::ios::binary | std::ios::ate);
		if (!f) return out; // Failed
		std::streampos end = f.tellg();
		f.seekg(0, std::ios::beg);
		std::size_t length = std::size_t(end - f.tellg());
		out.success = true;
		if (length == 0) return out; // Length of file is 0. Still counts as success.
		out.fileContent.clear();
		out.fileContent.resize(length);
		f.read(((char*)out.fileContent.data()), length);
		return out;
	}


	bool WriteFile(const std::string& filename, const byte* fileContent, size_t numBytes)
	{
		std::ofstream outFile(std::filesystem::u8path(filename), std::ios::out | std::ios::binary);
		if (!outFile) return false; // Failed to open file
		outFile.write((char*)fileContent, numBytes);
		outFile.close();
		return true;
	}
	bool WriteFile(const std::string& filename, const std::vector<byte>& fileContent)
	{
		return WriteFile(filename, fileContent.data(), fileContent.size());
	}
	bool WriteFile(const std::string& filename, const std::string& fileContent)
	{
		return WriteFile(filename, (byte*)fileContent.data(), fileContent.size());
	}

	bool AppendToFile(const std::string& filename, const byte* fileContent, size_t numBytes)
	{
		std::ofstream outFile(std::filesystem::u8path(filename), std::ios::out | std::ios::binary | std::ios_base::app);
		if (!outFile) return false; // Failed to open file
		outFile.write((char*)fileContent, numBytes);
		outFile.close();
		return true;
	}
	bool AppendToFile(const std::string& filename, const std::vector<byte>& fileContent)
	{
		return AppendToFile(filename, fileContent.data(), fileContent.size());
	}
	bool AppendToFile(const std::string& filename, const std::string& fileContent)
	{
		return AppendToFile(filename, (byte*)fileContent.data(), fileContent.size());
	}

	void Print(const std::string& text)
	{
#if defined IGLO_FORCE_CONSOLE_OUTPUT || !defined _WIN32
		std::cout << text;
#else
		OutputDebugStringA(text.c_str());
#endif
	}
	void Print(const char* text)
	{
#if defined IGLO_FORCE_CONSOLE_OUTPUT || !defined _WIN32
		std::cout << text;
#else
		OutputDebugStringA(text);
#endif
	}
	void Print(const std::stringstream& text)
	{
#if defined IGLO_FORCE_CONSOLE_OUTPUT || !defined _WIN32
		std::cout << text.str();
#else
		OutputDebugStringA(text.str().c_str());
#endif
	}
#ifdef _WIN32
	void Print(const std::wstring& text)
	{
#ifdef IGLO_FORCE_CONSOLE_OUTPUT
		std::wcout << text;
#else
		OutputDebugStringW(text.c_str());
#endif
	}
	void Print(const wchar_t* text)
	{
#ifdef IGLO_FORCE_CONSOLE_OUTPUT
		std::wcout << text;
#else
		OutputDebugStringW(text);
#endif
	}
	void Print(const std::wstringstream& text)
	{
#ifdef IGLO_FORCE_CONSOLE_OUTPUT
		std::wcout << text.str();
#else
		OutputDebugStringW(text.str().c_str());
#endif
	}
#endif

	float UniformRandom::NextFloat(float min, float max)
	{
		std::uniform_real_distribution<float> d(min, max);
		return d(randomGenerator);
	}

	double UniformRandom::NextDouble(double min, double max)
	{
		std::uniform_real_distribution<double> d(min, max);
		return d(randomGenerator);
	}

	int32_t UniformRandom::NextInt32(int32_t min, int32_t max)
	{
		std::uniform_int_distribution<int32_t> d(min, max);
		return d(randomGenerator);
	}

	uint32_t UniformRandom::NextUInt32()
	{
		std::uniform_int_distribution<uint32_t> d(0, 0xFFFFFFFF);
		return d(randomGenerator);
	}

	bool UniformRandom::NextBool()
	{
		return (NextInt32(0, 1) == 0);
	}

	bool UniformRandom::NextProbability(float probability)
	{
		if (probability >= 1.0f) return true;
		if (probability <= 0.0f) return false;
		float d = NextFloat(0, 1);
		return d < probability;
	}

	void UniformRandom::SetSeed(unsigned int seed)
	{
		randomGenerator.seed(seed);
	}

	void UniformRandom::SetSeedUsingRandomDevice()
	{
		std::random_device r;
		std::seed_seq seed{ r(), r(), r(), r(), r(), r(), r(), r() };
		randomGenerator.seed(seed);
	}

	namespace Random
	{
		int32_t NextInt32(int32_t min, int32_t max)
		{
			return (rand() % ((max - min) + 1)) + min;
		}

		uint32_t NextUInt32()
		{
			uint32_t a = rand() & 0xff;
			a |= (rand() & 0xff) << 8;
			a |= (rand() & 0xff) << 16;
			a |= (rand() & 0xff) << 24;
			return a;
		}

		bool NextBool()
		{
			return NextProbability(0.5f);
		}

		bool NextProbability(float probability)
		{
			if (probability >= 1.0f) return true;
			if (probability <= 0.0f) return false;
			float d = (float)rand() / (RAND_MAX + 1); // d can never be 1.0f
			return d < probability;
		}

		float NextFloat(float min, float max)
		{
			float d = (float)rand() / RAND_MAX;
			return min + d * (max - min);
		}

		double NextDouble(double min, double max)
		{
			double d = (double)rand() / RAND_MAX;
			return min + d * (max - min);
		}

		void SetSeed(unsigned int seed)
		{
			srand(seed);
		}
	}

	uint64_t AlignUp(uint64_t value, uint64_t alignment)
	{
		if (alignment == 0) return value;
		if (alignment & (alignment - 1)) throw std::invalid_argument("alignment not a power of 2.");

		uint64_t alignMask = alignment - 1;
		if (value & alignMask) // Not aligned
		{
			return (value & ~alignMask) + alignment;
		}
		else
		{
			return value; // Already aligned
		}
	}

	bool IsPowerOf2(uint64_t value)
	{
		if (value == 0) return false;
		return ((value & (value - 1)) == 0);
	}

} // namespace ig