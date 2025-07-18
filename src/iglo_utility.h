#pragma once

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <string>
#include <random>
#include <chrono>
#include <filesystem>
#ifdef __linux__
#include <memory>
#endif


#define IGLO_PI	(3.14159265358979323846)
#define IGLO_SQR2 (1.41421356237309504880)
#define IGLO_ToRadian(degree) ((degree) * (IGLO_PI / 180.0))
#define IGLO_ToDegree(radian) ((radian) * (180.0 / IGLO_PI))
#define IGLO_FLOAT32_MAX (3.402823466e+38f)
#define IGLO_UINT16_MAX (0xffff)
#define IGLO_UINT32_MAX (0xffffffff)
#define IGLO_UINT64_MAX (0xffffffffffffffff)
#define IGLO_MEGABYTE (1024 * 1024)

// 'byte' is a more intuitive name than 'uint8_t'.
typedef uint8_t byte;

namespace ig
{
	class PackedBoolArray;
	class Timer;
	class FrameRateLimiter;
	struct Vector2;
	struct Vector3;
	struct Vector4;
	struct Quaternion;
	struct Matrix4x4;
	struct Extent2D;
	struct Extent3D;
	struct IntPoint;
	struct IntRect;
	struct FloatRect;
	struct Color32;
	struct Color;
	enum class Encoding;
	struct ReadTextFileResult;
	struct ReadFileResult;
	class UniformRandom;


	// An array of boolean values. This class was written as an alternative to std::vector<bool>.
	// Unlike std::vector<bool>, this class guarantees that all booleans are efficiently packed in memory (1 bit per boolean).
	class PackedBoolArray
	{
	public:
		PackedBoolArray() = default;

		PackedBoolArray& operator = (const PackedBoolArray&);
		PackedBoolArray(const PackedBoolArray&);

		// Returns the boolean value at given index.
		bool GetAt(uint64_t index) const;

		// Assigns a value to the boolean at 'index'.
		void Set(uint64_t index, bool value);

		// The boolean at 'index' is set to true.
		void SetTrue(uint64_t index);

		// The boolean at 'index' is set to false.
		void SetFalse(uint64_t index);

		// Erases all booleans from the array. (The array is deallocated)
		void Clear();

		// Resizes the array. This reduces or expands the number of booleans this array contains.
		// Any booleans not removed will retain their value.
		// If expanded, new booleans will be assigned an initial value specified with 'initialValue'.
		// 'size' will be the new size of this array (it's the number of boolean values it can store).
		void Resize(uint64_t size, bool initialValue = false);

		// Assigns a value to all booleans in the array.
		void AssignValueToAll(bool value);

		// Gets the number of booleans stored in this array.
		uint64_t GetSize() const { return booleanCount; };

	private:
		static uint64_t GetElementCount(uint64_t numBooleans, uint32_t sizeOfEachElement);

		static constexpr unsigned int elementSize = 8;
		uint64_t booleanCount = 0;
		std::unique_ptr<uint8_t[]> data;
	};


	// Stopwatch timer
	class Timer
	{
	public:
		Timer() { Reset(); }
		// Starts/Resets the timer.
		void Reset();
		// Gets the amount of time that have passed since Reset() was called.
		double GetSeconds();
		double GetMilliseconds();
		double GetMicroseconds();
		uint64_t GetNanoseconds();
		// Retreives elapsed time and resets the timer simultaneously, using only a single call to chrono::steady_clock::now().
		double GetSecondsAndReset();
		double GetMillisecondsAndReset();
		double GetMicrosecondsAndReset();
		uint64_t GetNanosecondsAndReset();

	private:
		std::chrono::steady_clock::time_point t1;
	};

	class FrameRateLimiter
	{
	public:
		// The thread will wait inside this function until enough time as passed.
		// Call one of these functions once per frame to limit the frame rate of your app.
		// Example: LimitFPS(60.0) or LimitElapsedSeconds(1.0 / 60.0) to limit frame rate to 60 frames per second.
		// A combination of microsecond sleeps and CPU spinning is used to accurately limit the time passed between each call to this function.
		void LimitFPS(double maxFramesPerSecond);
		void LimitElapsedSeconds(double seconds);
	private:
		bool passedFirstFrame = false;
		typedef std::chrono::duration<double, std::chrono::seconds::period> myDur;
		std::chrono::time_point<std::chrono::steady_clock, myDur> t1;
	};

	// 2D Vector
	struct Vector2
	{
		Vector2() :x(0), y(0) {}
		Vector2(float x, float y) :x(x), y(y) {};

		// Operators
		Vector2 operator - () const { return Vector2(-x, -y); }
		Vector2& operator += (const Vector2& a) { x += a.x; y += a.y; return *this; }
		Vector2& operator -= (const Vector2& a) { x -= a.x; y -= a.y; return *this; }
		Vector2& operator *= (const float& a) { x *= a; y *= a; return *this; }
		Vector2& operator /= (const float& a) { x /= a; y /= a; return *this; }
		Vector2 operator + (const Vector2& a) const { return Vector2(x + a.x, y + a.y); }
		Vector2 operator - (const Vector2& a) const { return Vector2(x - a.x, y - a.y); }
		Vector2 operator * (float a) const { return Vector2(x * a, y * a); }
		Vector2 operator / (float a) const { return Vector2(x / a, y / a); }
		bool operator == (const Vector2& a) const { return (a.x == x && a.y == y); }
		bool operator != (const Vector2& a) const { return !(a.x == x && a.y == y); }

		static float Distance(float x1, float y1, float x2, float y2);
		static float Distance(const Vector2& p0, const Vector2& p1);

		// Returns the length of this vector.
		float GetMagnitude() const;
		float GetSquaredMagnitude() const;
		Vector2 GetNormalized() const;
		//TODO: i want the option of rotating by degrees 0-360.
		Vector2 GetRotated(float rotationInRadians) const;
		//TODO: More serious rotation functions for 2D and 3D vectors.

		std::string ToString() const;

		float x;
		float y;
	};

	// 3D Vector
	struct Vector3
	{
		Vector3() :x(0), y(0), z(0) {}
		Vector3(float x, float y, float z) :x(x), y(y), z(z) {};

		// Operators
		Vector3 operator - () const { return Vector3(-x, -y, -z); }
		Vector3& operator += (const Vector3& a) { x += a.x; y += a.y; z += a.z; return *this; }
		Vector3& operator -= (const Vector3& a) { x -= a.x; y -= a.y; z -= a.z; return *this; }
		Vector3& operator *= (const float& a) { x *= a; y *= a; z *= a; return *this; }
		Vector3& operator /= (const float& a) { x /= a; y /= a; z /= a; return *this; }
		Vector3 operator + (const Vector3& a) const { return Vector3(x + a.x, y + a.y, z + a.z); }
		Vector3 operator - (const Vector3& a) const { return Vector3(x - a.x, y - a.y, z - a.z); }
		Vector3 operator * (const float& a) const { return Vector3(x * a, y * a, z * a); }
		Vector3 operator / (const float& a) const { return Vector3(x / a, y / a, z / a); }
		bool operator == (const Vector3& a) const { return  (x == a.x && y == a.y && z == a.z); }
		bool operator != (const Vector3& a) const { return !(x == a.x && y == a.y && z == a.z); }

		// Assuming clockwise culling
		static Vector3 CalculateNormal(const Vector3& a, const Vector3& b, const Vector3& c);
		static float Distance(float x1, float y1, float z1, float x2, float y2, float z2);
		static float Distance(const Vector3& p0, const Vector3& p1);
		static Vector3 CrossProduct(const Vector3& a, const Vector3& b);
		static float DotProduct(const Vector3& a, const Vector3& b);
		static Vector3 TransformCoord(const Vector3& v, const Matrix4x4& m);
		static Vector3 TransformNormal(const Vector3& v, const Matrix4x4& m);

		// Returns the length of this vector.
		float GetMagnitude() const;
		float GetSquaredMagnitude() const;
		Vector3 GetNormalized() const;

		std::string ToString() const;

		float x;
		float y;
		float z;
	};

	// 4D Vector
	struct Vector4
	{
		Vector4() :x(0), y(0), z(0), w(0) {}
		Vector4(float x, float y, float z, float w) :x(x), y(y), z(z), w(w) {};

		// Operators
		Vector4 operator - () const { return Vector4(-x, -y, -z, -w); }
		Vector4& operator += (const Vector4& a) { x += a.x; y += a.y; z += a.z; w += a.w; return *this; }
		Vector4& operator -= (const Vector4& a) { x -= a.x; y -= a.y; z -= a.z; w -= a.w; return *this; }
		Vector4& operator *= (const float& a) { x *= a; y *= a; z *= a; w *= a; return *this; }
		Vector4& operator /= (const float& a) { x /= a; y /= a; z /= a; w /= a; return *this; }
		Vector4 operator + (const Vector4& a) const { return Vector4(x + a.x, y + a.y, z + a.z, w + a.w); }
		Vector4 operator - (const Vector4& a) const { return Vector4(x - a.x, y - a.y, z - a.z, w - a.w); }
		Vector4 operator * (const float& a) const { return Vector4(x * a, y * a, z * a, w * a); }
		Vector4 operator / (const float& a) const { return Vector4(x / a, y / a, z / a, w / a); }
		bool operator == (const Vector4& a) const { return  (x == a.x && y == a.y && z == a.z && w == a.w); }
		bool operator != (const Vector4& a) const { return !(x == a.x && y == a.y && z == a.z && w == a.w); }

		static Vector4 Transform(const Vector4& v, const Matrix4x4& m);

		// Returns the length of this vector.
		float GetMagnitude() const;
		float GetSquaredMagnitude() const;
		Vector4 GetNormalized() const;

		std::string ToString() const;

		float x;
		float y;
		float z;
		float w;
	};


	struct Quaternion
	{
		// Quaternion identity
		Quaternion() :x(0), y(0), z(0), w(1) {}

		Quaternion(float x, float y, float z, float w) :x(x), y(y), z(z), w(w) {};

		const static Quaternion Identity;

		bool operator == (const Quaternion& a) const;
		bool operator != (const Quaternion& a) const;

		Quaternion& operator *= (const Quaternion& a);
		Quaternion& operator *= (const float& a);
		Quaternion operator * (const Quaternion& a) const { return Quaternion(*this) *= a; }
		Quaternion operator * (const float& a) const { return Quaternion(*this) *= a; }

		static Quaternion AxisAngle(const Vector3& axis, float angleRadians);
		// Rotation is in radians
		static Quaternion Euler(float x, float y, float z);
		static Quaternion Euler(const Vector3& e) { return Euler(e.x, e.y, e.z); }

		Vector3 GetEulerAngles() const;
		Quaternion GetNormalized() const;

		std::string ToString() const;

		float x;
		float y;
		float z;
		float w;
	};


	struct Matrix4x4
	{
		// Matrix4x4 identity
		Matrix4x4() :
			a1(1), a2(0), a3(0), a4(0),
			b1(0), b2(1), b3(0), b4(0),
			c1(0), c2(0), c3(1), c4(0),
			d1(0), d2(0), d3(0), d4(1)
		{}

		Matrix4x4(
			float a1, float a2, float a3, float a4,
			float b1, float b2, float b3, float b4,
			float c1, float c2, float c3, float c4,
			float d1, float d2, float d3, float d4) :
			a1(a1), a2(a2), a3(a3), a4(a4),
			b1(b1), b2(b2), b3(b3), b4(b4),
			c1(c1), c2(c2), c3(c3), c4(c4),
			d1(d1), d2(d2), d3(d3), d4(d4)
		{}

		Matrix4x4(float elements[16]) :
			a1(elements[0]), a2(elements[1]), a3(elements[2]), a4(elements[3]),
			b1(elements[4]), b2(elements[5]), b3(elements[6]), b4(elements[7]),
			c1(elements[8]), c2(elements[9]), c3(elements[10]), c4(elements[11]),
			d1(elements[12]), d2(elements[13]), d3(elements[14]), d4(elements[15])
		{}

		const static Matrix4x4 Identity;

		bool operator == (const Matrix4x4& a) const;
		bool operator != (const Matrix4x4& a) const;
		Matrix4x4& operator *= (const Matrix4x4& a);
		Matrix4x4& operator *= (const float& a);
		Matrix4x4& operator += (const Matrix4x4& a);
		Matrix4x4& operator -= (const Matrix4x4& a);
		Matrix4x4 operator * (const Matrix4x4& a) const { return Matrix4x4(*this) *= a; }
		Matrix4x4 operator * (const float& a) const { return Matrix4x4(*this) *= a; }
		Matrix4x4 operator + (const Matrix4x4& a) const { return Matrix4x4(*this) += a; }
		Matrix4x4 operator - (const Matrix4x4& a) const { return Matrix4x4(*this) -= a; }

		Matrix4x4 GetTransposed() const;

		static Matrix4x4 Translation(const Vector3& translation);
		static Matrix4x4 Scaling(float width, float height, float depth);

		// Pitch rotation
		static Matrix4x4 RotationX(float radians);

		// Yaw rotation
		static Matrix4x4 RotationY(float radians);

		// Roll rotation
		static Matrix4x4 RotationZ(float radians);

		static Matrix4x4 QuaternionRotation(const Quaternion& q);

		// Creates a projection matrix (left handed)
		static Matrix4x4 PerspectiveFovLH(float aspectRatio, float fovInDegrees, float zNear, float zFar);

		// Creates a projection matrix (right handed)
		static Matrix4x4 PerspectiveFovRH(float aspectRatio, float fovInDegrees, float zNear, float zFar);

		// Creates a view matrix (left handed)
		static Matrix4x4 LookAtLH(const Vector3& eyePosition, const Vector3& lookAt, const Vector3& up);

		// Creates a view matrix (left handed)
		static Matrix4x4 LookToLH(const Vector3& eyePosition, const Vector3& toDirection, const Vector3& up);

		// Creates a view matrix (right handed)
		static Matrix4x4 LookAtRH(const Vector3& eyePosition, const Vector3& lookAt, const Vector3& up);

		// Creates a view matrix (right handed)
		static Matrix4x4 LookToRH(const Vector3& eyePosition, const Vector3& toDirection, const Vector3& up);

		// Creates an orthogonal projection matrix (left handed)
		static Matrix4x4 OrthoLH(float width, float height, float zNear, float zFar);

		// Creates an orthogonal projection matrix (right handed)
		static Matrix4x4 OrthoRH(float width, float height, float zNear, float zFar);

		// Creates a world matrix with this multiplication order: Translation * Rotation * Scale.
		static Matrix4x4 WorldTRS(Vector3 translation, Quaternion rotation, Vector3 scale);

		std::string ToString() const;

		union
		{
			struct
			{
				float a1, a2, a3, a4;
				float b1, b2, b3, b4;
				float c1, c2, c3, c4;
				float d1, d2, d3, d4;
			};
			float grid[4][4];
			float elements[16];
		};
	};

	struct Extent2D
	{
		Extent2D() :width(0), height(0) {};
		Extent2D(uint32_t width, uint32_t height) :width(width), height(height) {};

		bool operator == (const Extent2D& a) const;
		bool operator != (const Extent2D& a) const;

		std::string ToString() const;

		uint32_t width;
		uint32_t height;
	};


	struct Extent3D
	{
		Extent3D() :width(0), height(0), depth(0) {};
		Extent3D(uint32_t width, uint32_t height, uint32_t depth) :width(width), height(height), depth(depth) {};

		bool operator == (const Extent3D& a) const;
		bool operator != (const Extent3D& a) const;

		std::string ToString() const;

		uint32_t width;
		uint32_t height;
		uint32_t depth;
	};


	struct IntPoint
	{
		IntPoint() :x(0), y(0) {}
		IntPoint(int32_t x, int32_t y) :x(x), y(y) {}

		bool operator == (const IntPoint& a) const;
		bool operator != (const IntPoint& a) const;

		IntPoint& operator += (const IntPoint& a) { x += a.x; y += a.y; return *this; }
		IntPoint& operator -= (const IntPoint& a) { x -= a.x; y -= a.y; return *this; }
		IntPoint operator + (const IntPoint& a) const { return IntPoint(x + a.x, y + a.y); }
		IntPoint operator - (const IntPoint& a) const { return IntPoint(x - a.x, y - a.y); }

		std::string ToString() const;

		int32_t x;
		int32_t y;
	};


	struct IntRect
	{
		IntRect() :left(0), top(0), right(0), bottom(0) {};
		IntRect(int32_t left, int32_t top, int32_t right, int32_t bottom) :left(left), top(top), right(right), bottom(bottom) {};

		int32_t GetWidth() const { return right - left; }
		int32_t GetHeight() const { return bottom - top; }

		// Returns a rectangle with normalized sides so that top is always at the top, and left is always to the left etc..
		// Ensures that GetWidth() and GetHeight() do not return negative values.
		// A rectangle must be normalized so that the functions that check for intersections can give accurate results.
		IntRect GetNormalized() const;

		bool InclusiveContainsPoint(IntPoint point) const; // Inclusive
		bool InclusiveContainsPoint(int32_t pointX, int32_t pointY) const; // Inclusive
		bool ContainsPoint(int32_t pointX, int32_t pointY) const; // Exclusive
		bool ContainsPoint(IntPoint point) const; // Exclusive
		bool ContainsRect(IntRect rect) const; // Exclusive for top-left point and inclusive for bottom-right point.
		bool OverlapsWithRect(IntRect rect) const;

		bool operator == (const IntRect& a) const;
		bool operator != (const IntRect& a) const;

		// Translation
		IntRect& operator += (const IntPoint& a) { left += a.x; top += a.y; right += a.x; bottom += a.y; return *this; }
		IntRect& operator -= (const IntPoint& a) { left -= a.x; top -= a.y; right -= a.x; bottom -= a.y; return *this; }
		IntRect operator + (const IntPoint& a) const { return IntRect(left + a.x, top + a.y, right + a.x, bottom + a.y); }
		IntRect operator - (const IntPoint& a) const { return IntRect(left - a.x, top - a.y, right - a.x, bottom - a.y); }

		int32_t left;
		int32_t top;
		int32_t right;
		int32_t bottom;
	};


	struct FloatRect
	{
		FloatRect() :left(0), top(0), right(0), bottom(0) {};
		FloatRect(float left, float top, float right, float bottom) :left(left), top(top), right(right), bottom(bottom) {};
		FloatRect(const IntRect& intRect) :
			left((float)intRect.left),
			top((float)intRect.top),
			right((float)intRect.right),
			bottom((float)intRect.bottom) {};

		float GetWidth() const { return right - left; }
		float GetHeight() const { return bottom - top; }

		FloatRect GetNormalized() const;

		bool InclusiveContainsPoint(Vector2 point) const; // Inclusive
		bool InclusiveContainsPoint(float pointX, float pointY) const; // Inclusive
		bool ContainsPoint(float pointX, float pointY) const; // Exclusive
		bool ContainsPoint(Vector2 point) const; // Exclusive
		bool ContainsRect(FloatRect rect) const; // Exclusive for top-left point and inclusive for bottom-right point.
		bool OverlapsWithRect(FloatRect rect) const;

		bool operator == (const FloatRect& a) const;
		bool operator != (const FloatRect& a) const;

		// Translation
		FloatRect& operator += (const Vector2& a) { left += a.x; top += a.y; right += a.x; bottom += a.y; return *this; }
		FloatRect& operator -= (const Vector2& a) { left -= a.x; top -= a.y; right -= a.x; bottom -= a.y; return *this; }
		FloatRect operator + (const Vector2& a) const { return FloatRect(left + a.x, top + a.y, right + a.x, bottom + a.y); }
		FloatRect operator - (const Vector2& a) const { return FloatRect(left - a.x, top - a.y, right - a.x, bottom - a.y); }

		float left;
		float top;
		float right;
		float bottom;
	};

	// 32-bit RGBA color (unsigned integer).
	struct Color32
	{
		Color32() :rgba(0) {}
		Color32(uint32_t _0xAABBGGRR) :rgba(_0xAABBGGRR) {}
		Color32(byte red, byte green, byte blue) :red(red), green(green), blue(blue), alpha(255) {};
		Color32(byte red, byte green, byte blue, byte alpha) :red(red), green(green), blue(blue), alpha(alpha) {};

		bool operator == (const Color32& a) const;
		bool operator != (const Color32& a) const;

		union
		{
			uint32_t rgba;
			struct
			{
				byte red, green, blue, alpha;
			};
		};
	};

	// Floating point RGBA color
	struct Color
	{
		Color() :red(0), green(0), blue(0), alpha(0) {};
		Color(float red, float green, float blue) :red(red), green(green), blue(blue), alpha(1.0f) {};
		Color(float red, float green, float blue, float alpha) :red(red), green(green), blue(blue), alpha(alpha) {};
		Color(Color32 color32) :
			red((float)color32.red / 255.0f),
			green((float)color32.green / 255.0f),
			blue((float)color32.blue / 255.0f),
			alpha((float)color32.alpha / 255.0f)
		{};

		bool operator == (const Color& a) const;
		bool operator != (const Color& a) const;

		Color32 ToColor32() const;

		float red;
		float green;
		float blue;
		float alpha;
	};

	// 32 bit RGBA colors
	namespace Colors
	{
		const static Color32 Transparent = Color32(0, 0, 0, 0);
		const static Color32 White = Color32(255, 255, 255);
		const static Color32 Black = Color32(0, 0, 0);
		const static Color32 Red = Color32(255, 0, 0);
		const static Color32 Green = Color32(0, 255, 0);
		const static Color32 Blue = Color32(0, 0, 255);
		const static Color32 LightGray = Color32(192, 192, 192);
		const static Color32 Gray = Color32(128, 128, 128);
		const static Color32 DarkGray = Color32(64, 64, 64);
		const static Color32 Yellow = Color32(255, 255, 0);
		const static Color32 Pink = Color32(255, 0, 255);
		const static Color32 Cyan = Color32(0, 255, 255);
	};

	/////////////////////////// Sleep /////////////////////////////

	// Sleeps using the standard 'sleep_for' method.
	// On Windows it's not very accurate, expect an accuracy of roughly 15ms.
	void BasicSleep(uint32_t milliseconds);
	// Sleeps using a combination of many high resolution sleeps and CPU spinning. Highly accurate and low CPU usage.
	void PreciseSleep(double seconds);

	/////////////////////////// Strings /////////////////////////////

	// Converts a utf8 string to a utf32 string.
	// Given an invalid utf8 string it may produce incorrect codepoints,
	// but will not throw exceptions and will not do undefined behaviour.
	std::u32string utf8_to_utf32(const std::string& utf8);
	// Returns true if given utf8 string does not contain any invalid byte sequences.
	bool utf8_is_valid(const std::string& utf8);
	bool utf8_is_valid(const std::string& utf8, size_t startIndex, size_t endIndex);
	bool utf8_is_valid(const char* utf8, size_t length);
	// Returns true if next byte sequence is valid.
	// If returns true, 'out_stepForward' is set to the number of bytes to move forward to reach next byte sequence.
	// If returns false, 'out_stepForward' is not modified.
	// Argument 'out_stepForward' is optional (set to nullptr to ignore it).
	bool utf8_is_next_sequence_valid(const char* utf8, size_t length, size_t* out_stepForward);
	// Strips all invalid byte sequences from specified utf8 string.
	std::string utf8_make_valid(const std::string& utf8);
	// Replaces all invalid byte sequences from given utf8 string with a 1 byte ASCII character ranging from 1-127.
	// If given 'asciiCharacter' is 0 or higher than 127, then it is set to 63 (which is ASCII for '?').
	std::string utf8_replace_invalid(const std::string& utf8, byte asciiCharacter);
	std::string utf32_to_utf8(const std::u32string& utf32);
	std::string utf32_to_utf8(uint32_t codepoint);
	uint32_t utf32_to_lower(uint32_t codepoint);
	uint32_t utf32_to_upper(uint32_t codepoint);
	std::u32string utf32_to_lower(const std::u32string& utf32);
	std::u32string utf32_to_upper(const std::u32string& utf32);
	std::wstring utf8_to_utf16(const std::string& utf8);
	std::string utf16_to_utf8(const std::wstring& utf16);
	std::wstring utf16_to_wstring(const std::string& utf16, bool littleEndian);
	std::u32string utf32_to_u32string(const std::string& utf32, bool littleEndian);
	std::string CP1252_to_utf8(const std::string& cp1252);
	std::string CP437_to_utf8(const std::string& cp437);
	std::string utf8_to_CP1252(const std::string& utf8, bool stripOutOfRangeCodepoints = false, byte cp1252OutOfRangeReplacement = '?');
	std::string utf8_to_CP437(const std::string& utf8, bool stripOutOfRangeCodepoints = false, byte cp437OutOfRangeReplacement = '?');

	// Returns the number of codepoints found in given utf8 string.
	// Given an invalid utf8 string, invalid codepoints may count towards the total number of codepoints found.
	size_t utf8_length(const std::string& utf8);

	// Iterates codepoints in a utf8 string starting at 'startIndex' and stops at 'endIndex'. Can only iterate forwards.
	// 'out_codepoint' is set to the first codepoint found, and 'out_location' is set to the end location of that codepoint.
	// A codepoint whose bytes are cut off prematurely by 'endIndex' is ignored.
	// Returns false if no codepoint is found after reaching 'endIndex', and in that case, 'out_location' and 'out_codepoint' are not modified.
	// 'out_location' and 'out_codepoint' are optional. Specify nullptr to not retreive those values.
	// Given an invalid utf8 string, can produce incorrect codepoints, but will not throw exceptions and will not do undefined behaviour.
	bool utf8_next_codepoint(const std::string& utf8, size_t startIndex, size_t endIndex, size_t* out_location, uint32_t* out_codepoint);

	// Iterates codepoints in a utf8 string, similar to utf8_next_codepoint.
	// This function is more readable and idiomatic, but runs 29% slower in comparison.
	template<typename CodepointAction>
	void utf8_foreach_codepoint(const std::string& utf8, size_t startIndex, size_t endIndex, CodepointAction action)
	{
		uint32_t codepoint;
		size_t i = startIndex;
		while (utf8_next_codepoint(utf8, i, endIndex, &i, &codepoint))
		{
			action(codepoint);
		}
	}

	// Transforms a utf8 string to all lowercase
	std::string utf8_to_lower(const std::string& utf8);

	// Transforms a utf8 string to all uppercase
	std::string utf8_to_upper(const std::string& utf8);

	// Converts a utf8 string to filesystem::path. Needed for C++20.
	std::filesystem::path utf8_to_path(const std::string& utf8);

	// Converts u8string to string. Needed for C++20.
	std::string u8string_to_string(const std::u8string& u8str);


	//////////////////////// File system ////////////////////////

	// iglo uses UTF-8 encoding by default.
	// 'std::string' always implies UTF-8 encoding unless stated otherwise.

	// Returns the file extension of a filename string. An example of what this function may return: ".jpg"
	std::string GetFileExtension(const std::string& filename);
	bool FileExists(const std::string& filename);
	bool DirectoryExists(const std::string& directoryName);
	bool CreateDirectory(const std::string& directoryName);

	// Gets the current working directory
	std::string GetCurrentPath();

	struct ListDirectoryResult
	{
		std::vector<std::string> filenames;
		std::vector<std::string> foldernames;
		bool success = false; // If the operation completed successfully.
	};
	// Lists all files and folders in a directory.
	// If 'listSubDirectories' is true, all files and folders in all subdirectories will also be listed.
	ListDirectoryResult ListDirectory(const std::string& directoryPath, bool listSubDirectories = false);

	// Character encoding
	enum class Encoding
	{
		Unknown = 0,
		UTF8,
		UTF16_LE, // little-endian
		UTF16_BE, // big-endian
		UTF32_LE, // 4 bytes per character. little-endian. 
		UTF32_BE, // 4 bytes per character. big-endian. 
		CP1252, // Windows-1252. One byte per character. Range is 00-FF.
		CP437, // DOS Latin US. One byte per character. Range is 00-FF.
	};

	struct ReadTextFileResult
	{
		std::string text; // The contents of the file, encoded as UTF-8 string.
		bool success = false; // If the file could be successfully read or not.
	};

	// Reads file contents into a UTF-8 string. An appropriate string conversion function is used
	// depending on the file encoding to ensure the returned string is always encoded as UTF-8.
	// If you don't specify the encoding of the file, this function will determine its encoding by checking
	// the byte order mark and string validity. Specifying the file encoding may speed up this function by x2.
	ReadTextFileResult ReadTextFile(const std::string& filename, Encoding encoding = Encoding::Unknown);

	struct ReadFileResult
	{
		std::vector<byte> fileContent; // The contents of the file, stored as raw bytes.
		bool success = false; // If the file could be successfully read or not.
	};

	// Reads byte contents from a file.
	ReadFileResult ReadFile(const std::string& filename);

	// Writes a file. If file already exists, it is replaced.
	// Returns true if success.
	bool WriteFile(const std::string& filename, const byte* fileContent, size_t numBytes);
	bool WriteFile(const std::string& filename, const std::vector<byte>& fileContent);
	bool WriteFile(const std::string& filename, const std::string& fileContent);

	// Appends contents to existing file if file already exists.
	// If file doesn't already exist, will write to a new file.
	// Returns true if success.
	bool AppendToFile(const std::string& filename, const byte* fileContent, size_t numBytes);
	bool AppendToFile(const std::string& filename, const std::vector<byte>& fileContent);
	bool AppendToFile(const std::string& filename, const std::string& fileContent);

	//////////////////////// Debug/Output printing ////////////////////////

	template<typename T>
	std::string ToString(const T& t)
	{
		if constexpr (std::is_convertible_v<T, std::string>) // If it can be converted to std::string
		{
			return t;
		}
		else if constexpr (std::is_pointer_v<T>) // If it's a pointer
		{
			return std::to_string((uint64_t)t);
		}
		else
		{
			return std::to_string(t);
		}
	}

	// A string conversion function that extends std::to_string() to accept unlimited arguments
	// and can accept 'char*' and 'const char*' arguments.
	template<typename T, typename ...Args>
	constexpr std::string ToString(const T& t, Args&&... args)
	{
		std::string s = ToString(t);
		((s += ToString(std::forward<Args>(args))), ...);
		return s;
	}

	// Prints text to console
	void Print(const std::string& text);
	void Print(const std::stringstream& text);
	void Print(const char* text);
#ifdef _WIN32
	void Print(const std::wstring& text);
	void Print(const std::wstringstream& text);
	void Print(const wchar_t* text);
#endif

	///////////////////////////// Randomness  /////////////////////////////

	/*
		This is a wrapper class to make std::mt19937 easier to use. An instance of this class will start with
		a predetermined seed, so it is recommended to call SetSeed() or SetSeedUsingRandomDevice() before using it.

		Example usage:
		ig::UniformRandom random;
		random.SetSeedUsingRandomDevice();
		float someRandomFloat = random.NextFloat(0, 1);
		uint32_t someRandomInt = random.NextUInt32();
	*/
	class UniformRandom
	{
	public:
		UniformRandom() = default;

		// min and max are inclusive bounds. The random number can be max, and it can be min.
		int32_t NextInt32(int32_t min, int32_t max);
		uint32_t NextUInt32();

		// 50% chance of returning true.
		bool NextBool();

		// 'probability' must be in the range 0.0f to 1.0f. Higher probability means higher chance of returning true.
		// Example: 0.2f = returns true 20% of the time.
		bool NextProbability(float probability);

		// Produces a random float in the range [min, max).
		// Due to rounding issues, this may in fact produce a value in an inclusive max range instead.
		float NextFloat(float min, float max);

		// Produces a random double in the range [min, max)
		// Due to rounding issues, this may in fact produce a value in an inclusive max range instead.
		double NextDouble(double min, double max);

		void SetSeed(unsigned int seed);
		// Seeds the random generator using std::random_device and std::seed_seq.
		// This is generally considered the 'default' way to seed this thing.
		void SetSeedUsingRandomDevice();

		std::mt19937* GetRandomGenerator() { return &this->randomGenerator; }

	private:
		std::mt19937 randomGenerator;

	};

	namespace Random
	{
		/*
			These are wrapper functions to make rand() easier to use.
			These functions are fast but have low quality randomness, so don't use for important stuff.
		*/

		// min and max are inclusive bounds. The random number can be max, and it can be min.
		int32_t NextInt32(int32_t min, int32_t max);
		uint32_t NextUInt32();
		// 50% chance of returning true.
		bool NextBool();
		// 'probability' must be in the range 0.0f to 1.0f. Higher probability means higher chance of returning true.
		// Example: 0.2f = returns true 20% of the time.
		bool NextProbability(float probability);
		float NextFloat(float min, float max);
		double NextDouble(double min, double max);

		void SetSeed(unsigned int seed);

	}

	///////////////////////////// Math /////////////////////////////

	// The alignment is required to be a power of 2.
	uint64_t AlignUp(uint64_t value, uint64_t alignment);
	bool IsPowerOf2(uint64_t value);

} // namespace ig

