#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Vec4.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/MathUtils.hpp"

//------------------------------------------------------------------------------------------------------------------
Mat44::Mat44()
{

	for(int index = 0; index < NUM_INDEXES; ++index)
	{
		m_values[index] = 0.f;
	}

	m_values[Ix] = 1.f;
	m_values[Jy] = 1.f;
	m_values[Kz] = 1.f;
	m_values[Tw] = 1.f;



}

//------------------------------------------------------------------------------------------------------------------
Mat44::Mat44(Vec2 const& iBasis2D, Vec2 const& jBasis2D, Vec2 const& translation2D)
{

	for(int index = 0; index < NUM_INDEXES; ++index)
	{
		m_values[index] = 0;
	}

	m_values[Ix] = iBasis2D.x;
	m_values[Iy] = iBasis2D.y;

	m_values[Jx] = jBasis2D.x;
	m_values[Jy] = jBasis2D.y;

	m_values[Kz] = 1;

	m_values[Tx] = translation2D.x;
	m_values[Ty] = translation2D.y;

	m_values[Tw] = 1;

}


//------------------------------------------------------------------------------------------------------------------
Mat44::Mat44(Vec3 const& iBasis3D, Vec3 const& jBasis3D, Vec3 const& kBasis3D, Vec3 const& translation3D)
{

	for(int index = 0; index < NUM_INDEXES; ++index)
	{
		m_values[index] = 0;
	}

	m_values[Ix] = iBasis3D.x;
	m_values[Iy] = iBasis3D.y;
	m_values[Iz] = iBasis3D.z;

	m_values[Jx] = jBasis3D.x;
	m_values[Jy] = jBasis3D.y;
	m_values[Jz] = jBasis3D.z;


	m_values[Kx] = kBasis3D.x;
	m_values[Ky] = kBasis3D.y;
	m_values[Kz] = kBasis3D.z;

	m_values[Tx] = translation3D.x;
	m_values[Ty] = translation3D.y;
	m_values[Tz] = translation3D.z;
	m_values[Tw] = 1;


}


//------------------------------------------------------------------------------------------------------------------
Mat44::Mat44(Vec4 const& iBasis4D, Vec4 const& jBasis4D, Vec4 const& kBasis4D, Vec4 const& translation4D)
{

	m_values[Ix] = iBasis4D.x;
	m_values[Iy] = iBasis4D.y;
	m_values[Iz] = iBasis4D.z;
	m_values[Iw] = iBasis4D.w;

	m_values[Jx] = jBasis4D.x;
	m_values[Jy] = jBasis4D.y;
	m_values[Jz] = jBasis4D.z;
	m_values[Jw] = jBasis4D.w;


	m_values[Kx] = kBasis4D.x;
	m_values[Ky] = kBasis4D.y;
	m_values[Kz] = kBasis4D.z;
	m_values[Kw] = kBasis4D.w;

	m_values[Tx] = translation4D.x;
	m_values[Ty] = translation4D.y;
	m_values[Tz] = translation4D.z;
	m_values[Tw] = translation4D.w;
	
}


//------------------------------------------------------------------------------------------------------------------
Mat44::Mat44(float const* sixteenValuesBasisMajor)
{
	for(int index = 0; index < NUM_INDEXES; ++index)
	{
		m_values[index] = sixteenValuesBasisMajor[index];
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Mat44::Mat44(double const* sixteenValuesBasisMajor)
{
	for(int index = 0; index < NUM_INDEXES; ++index)
	{
		m_values[index] = static_cast<float>(sixteenValuesBasisMajor[index]);
	}
}


//------------------------------------------------------------------------------------------------------------------
Mat44 const Mat44::MakeTranslation2D(Vec2 const& translationXY)
{
	Vec2 iBasis = Vec2(1.f, 0.f);
	Vec2 jBasis = Vec2(0.f, 1.f);
	return Mat44(iBasis, jBasis, translationXY);

}


//------------------------------------------------------------------------------------------------------------------
Mat44 const Mat44::MakeTranslation3D(Vec3 const& translationXYZ)
{
	Vec3 iBasis = Vec3(1.f, 0.f, 0.f);
	Vec3 jBasis = Vec3(0.f, 1.f, 0.f);
	Vec3 kBasis = Vec3(0.f, 0.f, 1.f);
	return Mat44(iBasis, jBasis, kBasis, translationXYZ);
}


//------------------------------------------------------------------------------------------------------------------
Mat44 const Mat44::MakeUniformScale2D(float uniformScaleXY)
{
	Vec2 iBasis = Vec2(1.f, 0.f) * uniformScaleXY;
	Vec2 jBasis = Vec2(0.f, 1.f) * uniformScaleXY;

	Vec2 translationXY = Vec2(0.f, 0.f);
	return Mat44(iBasis, jBasis, translationXY);
}


//------------------------------------------------------------------------------------------------------------------
Mat44 const Mat44::MakeUniformScale3D(float uniformScaleXYZ)
{
	Vec3 iBasis = Vec3(1.f, 0.f, 0.f) * uniformScaleXYZ;
	Vec3 jBasis = Vec3(0.f, 1.f, 0.f) * uniformScaleXYZ;
	Vec3 kBasis = Vec3(0.f, 0.f, 1.f) * uniformScaleXYZ;
	   
	Vec3 translationXYZ = Vec3(0.f, 0.f, 0.f);
	return Mat44(iBasis, jBasis, kBasis, translationXYZ);
}


//------------------------------------------------------------------------------------------------------------------
Mat44 const Mat44::MakeNonUniformScale2D(Vec2 const& nonUniformScaleXY)
{
	Vec2 iBasis = Vec2(1.f, 0.f) * nonUniformScaleXY.x;
	Vec2 jBasis = Vec2(0.f, 1.f) * nonUniformScaleXY.y;

	Vec2 translationXY = Vec2(0.f, 0.f);
	return Mat44(iBasis, jBasis, translationXY);
}


//------------------------------------------------------------------------------------------------------------------
Mat44 const Mat44::MakeNonUniformScale3D(Vec3 const& nonUniformScaleXYZ)
{
	Vec3 iBasis = Vec3(1.f, 0.f, 0.f) * nonUniformScaleXYZ.x;
	Vec3 jBasis = Vec3(0.f, 1.f, 0.f) * nonUniformScaleXYZ.y;
	Vec3 kBasis = Vec3(0.f, 0.f, 1.f) * nonUniformScaleXYZ.z;

	Vec3 translationXYZ = Vec3(0.f, 0.f, 0.f);
	return Mat44(iBasis, jBasis, kBasis, translationXYZ);
}

//------------------------------------------------------------------------------------------------------------------
Mat44 const Mat44::MakeZRotationDegrees(float rotationDegreesAboutZ)
{

	Vec2 polarIBasis = Vec2::MakeFromPolarDegrees(rotationDegreesAboutZ);
	Vec2 polarJBasis = polarIBasis.GetRotated90Degrees();
	Vec2 translationXY = Vec2(0.f, 0.f);

	return Mat44(polarIBasis, polarJBasis, translationXY);
}


//------------------------------------------------------------------------------------------------------------------
Mat44 const Mat44::MakeYRotationDegrees(float rotationDegreesAboutY)
{
	Vec2 polarBasis = Vec2::MakeFromPolarDegrees(rotationDegreesAboutY);

	Vec3 iBasis = Vec3(polarBasis.x, 0.f, -polarBasis.y);
	Vec3 jBasis = Vec3(0.f, 1.f, 0.f);
	Vec3 kBasis = Vec3(polarBasis.y, 0.f, polarBasis.x);
	Vec3 translationXYZ = Vec3(0.f, 0.f, 0.f);

	return Mat44(iBasis, jBasis, kBasis, translationXYZ);
}


//------------------------------------------------------------------------------------------------------------------
Mat44 const Mat44::MakeXRotationDegrees(float rotationDegreesAboutX)
{
	Vec2 polarBasis = Vec2::MakeFromPolarDegrees(rotationDegreesAboutX);

	Vec3 iBasis = Vec3(1.f, 0.f, 0.f);
	Vec3 jBasis = Vec3(0.f, polarBasis.x, polarBasis.y);
	Vec3 kBasis = Vec3(0.f, -polarBasis.y, polarBasis.x);
	Vec3 translationXYZ = Vec3(0.f, 0.f, 0.f);

	return Mat44(iBasis, jBasis, kBasis, translationXYZ);
}

//------------------------------------------------------------------------------------------------------------------
Mat44 const Mat44::MakeOrthoProjection(float left, float right, float bottom, float top, float zNear, float zFar)
{
	// If I were ever to comeback to this and not understand a single line, go refer page 15 of MP2 notes

	// iBasis for transforming X
	float xScale = 1.f / (left - right);
	float iX = -2.f * xScale;
	float tX = (left + right) * xScale;
	Vec4 iBasis(iX, 0.f, 0.f, 0.f);

	// jBasis for Transforming Y
	float yScale = 1.f / (bottom - top);
	float jY = -2.f * yScale;
	float tY = (bottom + top) * yScale;
	Vec4 jBasis(0.f, jY, 0.f, 0.f);

	// kBasis for Transforming Z
	float zScale = 1 / (zNear - zFar);
	float kZ = -1.f * zScale;
	float tZ = zNear * zScale;
	Vec4 kBasis(0.f, 0.f, kZ, 0.f);

	// t part
	Vec4 translation(tX, tY, tZ, 1.f);

	return Mat44(iBasis, jBasis, kBasis, translation);
}

//------------------------------------------------------------------------------------------------------------------
Mat44 const Mat44::MakePerspectiveProjection(float fovYDegrees, float aspect, float zNear, float zFar)
{

	Mat44 perspective;

	float c = CosDegrees(fovYDegrees * 0.5f);
	float s = SinDegrees(fovYDegrees * 0.5f);

	float scaleY = c / s;
	float scaleX = scaleY / aspect;
	float scaleZ = zFar / (zFar - zNear);
	float translateZ = (zNear * zFar) / (zNear - zFar);

	perspective.m_values[Ix] = scaleX;
	perspective.m_values[Jy] = scaleY;
	perspective.m_values[Kz] = scaleZ;
	perspective.m_values[Kw] = 1.f;
	perspective.m_values[Tz] = translateZ;
	perspective.m_values[Tw] = 0.f;


	return perspective;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Mat44 const Mat44::MakeFromQuaternion(float x, float y, float z, float w)
{
	Mat44 result;

	// Column 1 (I-Basis)
	float iX = 1.f - 2.f * ((y * y) - (z * z));
	float iY = 2.f * ((x * y) - (w * z));
	float iZ = 2.f * ((x * z) + (w * y));
	float iW = 0.f;

	// Column 2 (J-Basis)
	float jX = 2.f * ((x * y) + (w * z));
	float jY = 1.f - 2.f * ((x * x) - (z * z));
	float jZ = 2.f * ((y * z) - (w * x));
	float jW = 0.f;

	// Column 3 (K-Basis)
	float kX = 2.f * ((x * z) - (w * y));
	float kY = 2.f * ((y * z) + (w * x));
	float kZ = 1.f - 2.f * ((x * x) - (y * y));
	float kW = 0.f;

	// Column 4 (Translation)
	float tX = 0.f;
	float tY = 0.f;
	float tZ = 0.f;
	float tW = 1.f;

	result.m_values[Ix] = iX;
	result.m_values[Iy] = iY;
	result.m_values[Iz] = iZ;
	result.m_values[Iw] = iW;

	result.m_values[Jx] = jX;
	result.m_values[Jy] = jY;
	result.m_values[Jz] = jZ;
	result.m_values[Jw] = jW;

	result.m_values[Kx] = kX;
	result.m_values[Ky] = kY;
	result.m_values[Kz] = kZ;
	result.m_values[Kw] = kW;

	result.m_values[Tx] = tX;
	result.m_values[Ty] = tY;
	result.m_values[Tz] = tZ;
	result.m_values[Tw] = tW;

	return result;
}

// -------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Mat44 const Mat44::CreateFromQuaternion(float x, float y, float z, float w)
// {
// 	Mat44 result;
// 
// 	float xx = x * x;
// 	float yy = y * y;
// 	float zz = z * z;
// 	float xy = x * y;
// 	float xz = x * z;
// 	float yz = y * z;
// 	float wx = w * x;
// 	float wy = w * y;
// 	float wz = w * z;
// 
// 	// Column i (forward/X-axis)
// 	result.m_values[Ix] = 1.0f - 2.0f * (yy + zz);
// 	result.m_values[Iy] = 2.0f * (xy + wz);
// 	result.m_values[Iz] = 2.0f * (xz - wy);
// 	result.m_values[Iw] = 0.0f;
// 
// 	// Column j (left/Y-axis)
// 	result.m_values[Jx] = 2.0f * (xy - wz);
// 	result.m_values[Jy] = 1.0f - 2.0f * (xx + zz);
// 	result.m_values[Jz] = 2.0f * (yz + wx);
// 	result.m_values[Jw] = 0.0f;
// 
// 	// Column k (up/Z-axis)
// 	result.m_values[Kx] = 2.0f * (xz + wy);
// 	result.m_values[Ky] = 2.0f * (yz - wx);
// 	result.m_values[Kz] = 1.0f - 2.0f * (xx + yy);
// 	result.m_values[Kw] = 0.0f;
// 
// 	// Column t (translation)
// 	result.m_values[Tx] = 0.0f;
// 	result.m_values[Ty] = 0.0f;
// 	result.m_values[Tz] = 0.0f;
// 	result.m_values[Tw] = 1.0f;
// 
// 	return result;
// }

//------------------------------------------------------------------------------------------------------------------
float* Mat44::GetAsFloatArray()
{
	return m_values;
}


//------------------------------------------------------------------------------------------------------------------
float const* Mat44::GetAsFloatArray() const
{
	return m_values;
}


//------------------------------------------------------------------------------------------------------------------
Vec2 const Mat44::GetIBasis2D() const
{
	return Vec2(m_values[Ix], m_values[Iy]);
}


//------------------------------------------------------------------------------------------------------------------
Vec2 const Mat44::GetJBasis2D() const
{
	return Vec2(m_values[Jx], m_values[Jy]);
}


//------------------------------------------------------------------------------------------------------------------
Vec2 const Mat44::GetTranslation2D() const
{
	return Vec2(m_values[Tx], m_values[Ty]);
}


//------------------------------------------------------------------------------------------------------------------
Vec3 const Mat44::GetIBasis3D() const
{
	return Vec3(m_values[Ix], m_values[Iy], m_values[Iz]);
}


//------------------------------------------------------------------------------------------------------------------
Vec3 const Mat44::GetJBasis3D() const
{
	return Vec3(m_values[Jx], m_values[Jy], m_values[Jz]);
}


//------------------------------------------------------------------------------------------------------------------
Vec3 const Mat44::GetKBasis3D() const
{
	return Vec3(m_values[Kx], m_values[Ky], m_values[Kz]);
}


//------------------------------------------------------------------------------------------------------------------
Vec3 const Mat44::GetTranslation3D() const
{
	return Vec3(m_values[Tx], m_values[Ty], m_values[Tz]);
}


//------------------------------------------------------------------------------------------------------------------
Vec4 const Mat44::GetIBasis4D() const
{
	return Vec4(m_values[Ix], m_values[Iy], m_values[Iz], m_values[Iw]);
}


//------------------------------------------------------------------------------------------------------------------
Vec4 const Mat44::GetJBasis4D() const
{
	return Vec4(m_values[Jx], m_values[Jy], m_values[Jz], m_values[Jw]);
}


//------------------------------------------------------------------------------------------------------------------
Vec4 const Mat44::GetKBasis4D() const
{
	return Vec4(m_values[Kx], m_values[Ky], m_values[Kz], m_values[Kw]);
}


//------------------------------------------------------------------------------------------------------------------
Vec4 const Mat44::GetTranslation4D() const
{
	return Vec4(m_values[Tx], m_values[Ty], m_values[Tz], m_values[Tw]);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void const Mat44::GetIJKT3D(Vec3& iBasis3D, Vec3& jBasis3D, Vec3& kBasis3D, Vec3& translation3D) const
{
	iBasis3D	  = Vec3(m_values[Ix], m_values[Iy], m_values[Iz]);
	jBasis3D	  = Vec3(m_values[Jx], m_values[Jy], m_values[Jz]);
	kBasis3D	  = Vec3(m_values[Kx], m_values[Ky], m_values[Kz]);
	translation3D = Vec3(m_values[Tx], m_values[Ty], m_values[Tz]);
}

//------------------------------------------------------------------------------------------------------------------
Mat44 const Mat44::GetOrthonormalInverse() const
{

	Vec3 iBasis = Vec3(m_values[Ix], m_values[Jx], m_values[Kx]);
	Vec3 jBasis = Vec3(m_values[Iy], m_values[Jy], m_values[Ky]);
	Vec3 kBasis = Vec3(m_values[Iz], m_values[Jz], m_values[Kz]);

	float tX = -(m_values[Tx] * m_values[Ix] + m_values[Ty] * m_values[Iy] + m_values[Tz] * m_values[Iz]);
	float tY = -(m_values[Tx] * m_values[Jx] + m_values[Ty] * m_values[Jy] + m_values[Tz] * m_values[Jz]);
	float tZ = -(m_values[Tx] * m_values[Kx] + m_values[Ty] * m_values[Ky] + m_values[Tz] * m_values[Kz]);
	Vec3 translations = Vec3(tX, tY, tZ);

	Mat44 inverseMat = Mat44(iBasis, jBasis, kBasis, translations);

	return inverseMat;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Mat44 const Mat44::GetInverse() const
{
	float detIJy = m_values[Jy] * ((m_values[Kz] * m_values[Tw]) - (m_values[Tz] * m_values[Kw]));
	float detIKy = m_values[Ky] * ((m_values[Jz] * m_values[Tw]) - (m_values[Tz] * m_values[Jw]));
	float detITy = m_values[Ty] * ((m_values[Jz] * m_values[Kw]) - (m_values[Kz] * m_values[Jw]));
	float detIx = detIJy - detIKy + detITy;

	// Jx
	float detJIy = m_values[Iy] * ((m_values[Kz] * m_values[Tw]) - (m_values[Tz] * m_values[Kw]));
	float detJKy = m_values[Ky] * ((m_values[Iz] * m_values[Tw]) - (m_values[Tz] * m_values[Iw]));
	float detJTy = m_values[Ty] * ((m_values[Iz] * m_values[Kw]) - (m_values[Kz] * m_values[Iw]));
	float detJx = detJIy - detJKy + detJTy;

	// Kx
	float detKIy = m_values[Iy] * ((m_values[Jz] * m_values[Tw]) - (m_values[Tz] * m_values[Jw]));
	float detKJy = m_values[Jy] * ((m_values[Iz] * m_values[Tw]) - (m_values[Tz] * m_values[Iw]));
	float detKTy = m_values[Ty] * ((m_values[Iz] * m_values[Jw]) - (m_values[Jz] * m_values[Iw]));
	float detKx = detKIy - detKJy + detKTy;

	// Tx
	float detTIy = m_values[Iy] * ((m_values[Jz] * m_values[Kw]) - (m_values[Kz] * m_values[Jw]));
	float detTJy = m_values[Jy] * ((m_values[Iz] * m_values[Kw]) - (m_values[Kz] * m_values[Iw]));
	float detTKy = m_values[Ky] * ((m_values[Iz] * m_values[Jw]) - (m_values[Jz] * m_values[Iw]));
	float detTx = detTIy - detTJy + detTKy;

	float det4x4 = detIx - detJx + detKx - detTx;

	if(fabs(det4x4) < 1e-8f)
	{
		// Singular matrix; return identity or handle error
		return Mat44();
	}

	float invDet = 1.0f / det4x4;

	// --- Build adjugate (all cofactors, transposed) ---
	Mat44 adj;
	for(int r = 0; r < 4; ++r)
	{
		for(int c = 0; c < 4; ++c)
		{
			// Minor: determinant of 3x3 submatrix without row r, col c
			float minor[9];
			int mi = 0;
			for(int rr = 0; rr < 4; ++rr)
			{
				if(rr == r) continue;
				for(int cc = 0; cc < 4; ++cc)
				{
					if(cc == c) continue;
					minor[mi++] = m_values[rr * 4 + cc];
				}
			}

			// Compute det of 3x3 minor
			float detMinor =
				minor[0] * (minor[4] * minor[8] - minor[5] * minor[7]) -
				minor[1] * (minor[3] * minor[8] - minor[5] * minor[6]) +
				minor[2] * (minor[3] * minor[7] - minor[4] * minor[6]);

			// Cofactor = sign * det(minor)
			float sign = ((r + c) % 2 == 0) ? 1.0f : -1.0f;
			adj.m_values[c * 4 + r] = sign * detMinor; // transpose here
		}
	}

	// --- Scale by 1/det ---
	for(int i = 0; i < 16; ++i)
	{
		adj.m_values[i] *= invDet;
	}

	return adj;

}


//------------------------------------------------------------------------------------------------------------------
void Mat44::SetTranslation2D(Vec2 const& translation2D)
{

	m_values[Tx] = translation2D.x;
	m_values[Ty] = translation2D.y;
	m_values[Tz] = 0.f;
	m_values[Tw] = 1.f;
}


//------------------------------------------------------------------------------------------------------------------
void Mat44::SetTranslation3D(Vec3 const& translation3D)
{

	m_values[Tx] = translation3D.x;
	m_values[Ty] = translation3D.y;
	m_values[Tz] = translation3D.z;
	m_values[Tw] = 1;

}


//------------------------------------------------------------------------------------------------------------------
void Mat44::SetIJ2D(Vec2 const& iBasis2D, Vec2 const& jBasis2D)
{

	m_values[Ix] = iBasis2D.x;
	m_values[Iy] = iBasis2D.y;
	m_values[Iz] = 0.f;
	m_values[Iw] = 0.f;

	m_values[Jx] = jBasis2D.x;
	m_values[Jy] = jBasis2D.y;
	m_values[Jz] = 0.f;
	m_values[Jw] = 0.f;

}


//------------------------------------------------------------------------------------------------------------------
void Mat44::SetIJT2D(Vec2 const& iBasis2D, Vec2 const& jBasis2D, Vec2 const& translation2D)
{

	m_values[Ix] = iBasis2D.x;
	m_values[Iy] = iBasis2D.y;
	m_values[Iz] = 0.f;
	m_values[Iw] = 0.f;

	m_values[Jx] = jBasis2D.x;
	m_values[Jy] = jBasis2D.y;
	m_values[Jz] = 0.f;
	m_values[Jw] = 0.f;
	
	m_values[Tx] = translation2D.x;
	m_values[Ty] = translation2D.y;
	m_values[Tz] = 0.f;
	m_values[Tw] = 1.f;

}


//------------------------------------------------------------------------------------------------------------------
void Mat44::SetIJK3D(Vec3 const& iBasis3D, Vec3 const& jBasis3D, Vec3 const& kBasis3D)
{

	m_values[Ix] = iBasis3D.x;
	m_values[Iy] = iBasis3D.y;
	m_values[Iz] = iBasis3D.z;
	m_values[Iw] = 0.f;

	m_values[Jx] = jBasis3D.x;
	m_values[Jy] = jBasis3D.y;
	m_values[Jz] = jBasis3D.z;
	m_values[Jw] = 0.f;

	m_values[Kx] = kBasis3D.x;
	m_values[Ky] = kBasis3D.y;
	m_values[Kz] = kBasis3D.z;
	m_values[Kw] = 0.f;


}


//------------------------------------------------------------------------------------------------------------------
void Mat44::SetIJKT3D(Vec3 const& iBasis3D, Vec3 const& jBasis3D, Vec3 const& kBasis3D, Vec3 const& translation3D)
{

	m_values[Ix] = iBasis3D.x;
	m_values[Iy] = iBasis3D.y;
	m_values[Iz] = iBasis3D.z;
	m_values[Iw] = 0.f;

	m_values[Jx] = jBasis3D.x;
	m_values[Jy] = jBasis3D.y;
	m_values[Jz] = jBasis3D.z;
	m_values[Jw] = 0.f;

	m_values[Kx] = kBasis3D.x;
	m_values[Ky] = kBasis3D.y;
	m_values[Kz] = kBasis3D.z;
	m_values[Kw] = 0.f;

	m_values[Tx] = translation3D.x;
	m_values[Ty] = translation3D.y;
	m_values[Tz] = translation3D.z;
	m_values[Tw] = 1.f;

}


//------------------------------------------------------------------------------------------------------------------
void Mat44::SetIJKT4D(Vec4 const& iBasis4D, Vec4 const& jBasis4D, Vec4 const& kBasis4D, Vec4 const& translation4D)
{

	m_values[Ix] = iBasis4D.x;
	m_values[Iy] = iBasis4D.y;
	m_values[Iz] = iBasis4D.z;
	m_values[Iw] = iBasis4D.w;

	m_values[Jx] = jBasis4D.x;
	m_values[Jy] = jBasis4D.y;
	m_values[Jz] = jBasis4D.z;
	m_values[Jw] = jBasis4D.w;


	m_values[Kx] = kBasis4D.x;
	m_values[Ky] = kBasis4D.y;
	m_values[Kz] = kBasis4D.z;
	m_values[Kw] = kBasis4D.w;

	m_values[Tx] = translation4D.x;
	m_values[Ty] = translation4D.y;
	m_values[Tz] = translation4D.z;
	m_values[Tw] = translation4D.w;

}

//------------------------------------------------------------------------------------------------------------------
void Mat44::Transpose()
{

	float tempArray[NUM_INDEXES];

	for(int index = 0; index < NUM_INDEXES; ++index)
	{
		tempArray[index] = m_values[index];
	}

	m_values[Ix] = tempArray[Ix];
	m_values[Iy] = tempArray[Jx];
	m_values[Iz] = tempArray[Kx];
	m_values[Iw] = tempArray[Tx];
				 
	m_values[Jx] = tempArray[Iy];
	m_values[Jy] = tempArray[Jy];
	m_values[Jz] = tempArray[Ky];
	m_values[Jw] = tempArray[Ty];
				 
	m_values[Kx] = tempArray[Iz];
	m_values[Ky] = tempArray[Jz];
	m_values[Kz] = tempArray[Kz];
	m_values[Kw] = tempArray[Tz];
				 
	m_values[Tx] = tempArray[Iw];
	m_values[Ty] = tempArray[Jw];
	m_values[Tz] = tempArray[Kw];
	m_values[Tw] = tempArray[Tw];

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Mat44 Mat44::GetTranspose() const
{
	float transpose[NUM_INDEXES];

	transpose[Ix] = m_values[Ix];
	transpose[Iy] = m_values[Jx];
	transpose[Iz] = m_values[Kx];
	transpose[Iw] = m_values[Tx];
	
	transpose[Jx] = m_values[Iy];
	transpose[Jy] = m_values[Jy];
	transpose[Jz] = m_values[Ky];
	transpose[Jw] = m_values[Ty];
	
	transpose[Kx] =m_values[Iz];
	transpose[Ky] =m_values[Jz];
	transpose[Kz] =m_values[Kz];
	transpose[Kw] =m_values[Tz];
	
	transpose[Tx] = m_values[Iw];
	transpose[Ty] = m_values[Jw];
	transpose[Tz] = m_values[Kw];
	transpose[Tw] = m_values[Tw];

	return Mat44(transpose);
}

//------------------------------------------------------------------------------------------------------------------
void Mat44::Orthonormalize_XFwd_YLeft_ZUp()
{

	// gram schmidt
	Vec4 iBasis = GetIBasis4D();
	Vec4 jBasis = GetJBasis4D();
	Vec4 kBasis = GetKBasis4D();

	// normalize I
	iBasis = iBasis.GetNormalized();

	// fix k by removing i parts
	float dotKI = DotProduct4D(kBasis, iBasis);
	Vec4 iPartsInK = dotKI * iBasis;

	kBasis -= iPartsInK;

	kBasis = kBasis.GetNormalized();

	// fix J
	float dotJI = DotProduct4D(jBasis, iBasis);
	Vec4 iPartsInJ = dotJI * iBasis;
	jBasis -= iPartsInJ;

	float dotJK = DotProduct4D(jBasis, kBasis);
	Vec4 kPartsInJ = dotJK * kBasis;
	jBasis -= kPartsInJ;

	jBasis = jBasis.GetNormalized();

	SetIJKT4D(iBasis, jBasis, kBasis, GetTranslation4D());
}


//------------------------------------------------------------------------------------------------------------------
Vec2 const Mat44::TransformVectorQuantity2D(Vec2 const& vectorQuantityXY) const
{
	
	float x = (m_values[Ix] * vectorQuantityXY.x) + (m_values[Jx] * vectorQuantityXY.y);
	float y = (m_values[Iy] * vectorQuantityXY.x) + (m_values[Jy] * vectorQuantityXY.y);

	return Vec2(x, y);

}


//------------------------------------------------------------------------------------------------------------------
Vec3 const Mat44::TransformVectorQuantity3D(Vec3 const& vectorQuantityXYZ) const
{
	float x = (m_values[Ix] * vectorQuantityXYZ.x) + (m_values[Jx] * vectorQuantityXYZ.y) + (m_values[Kx] * vectorQuantityXYZ.z);
	float y = (m_values[Iy] * vectorQuantityXYZ.x) + (m_values[Jy] * vectorQuantityXYZ.y) + (m_values[Ky] * vectorQuantityXYZ.z);
	float z = (m_values[Iz] * vectorQuantityXYZ.x) + (m_values[Jz] * vectorQuantityXYZ.y) + (m_values[Kz] * vectorQuantityXYZ.z);

	return Vec3(x, y, z);

}


//------------------------------------------------------------------------------------------------------------------
Vec2 const Mat44::TransformPosition2D(Vec2 const& positionXY) const
{
	float x = (m_values[Ix] * positionXY.x) + (m_values[Jx] * positionXY.y) + m_values[Tx];
	float y = (m_values[Iy] * positionXY.x) + (m_values[Jy] * positionXY.y) + m_values[Ty];

	return Vec2(x, y);
}


//------------------------------------------------------------------------------------------------------------------
Vec3 const Mat44::TransformPosition3D(Vec3 const positionXYZ) const
{
	float x = (m_values[Ix] * positionXYZ.x) + (m_values[Jx] * positionXYZ.y) + (m_values[Kx] * positionXYZ.z) + m_values[Tx];
	float y = (m_values[Iy] * positionXYZ.x) + (m_values[Jy] * positionXYZ.y) + (m_values[Ky] * positionXYZ.z) + m_values[Ty];
	float z = (m_values[Iz] * positionXYZ.x) + (m_values[Jz] * positionXYZ.y) + (m_values[Kz] * positionXYZ.z) + m_values[Tz];

	return Vec3(x, y, z);
}


//------------------------------------------------------------------------------------------------------------------
Vec4 const Mat44::TransformHomogeneous3D(Vec4 const& homogenousPoint3D) const
{
	float x = (m_values[Ix] * homogenousPoint3D.x) + (m_values[Jx] * homogenousPoint3D.y) + (m_values[Kx] * homogenousPoint3D.z) + (m_values[Tx] * homogenousPoint3D.w);
	float y = (m_values[Iy] * homogenousPoint3D.x) + (m_values[Jy] * homogenousPoint3D.y) + (m_values[Ky] * homogenousPoint3D.z) + (m_values[Ty] * homogenousPoint3D.w);
	float z = (m_values[Iz] * homogenousPoint3D.x) + (m_values[Jz] * homogenousPoint3D.y) + (m_values[Kz] * homogenousPoint3D.z) + (m_values[Tz] * homogenousPoint3D.w);
	float w = (m_values[Iw] * homogenousPoint3D.x) + (m_values[Jw] * homogenousPoint3D.y) + (m_values[Kw] * homogenousPoint3D.z) + (m_values[Tw] * homogenousPoint3D.w);

	return Vec4(x, y, z, w);
}


//------------------------------------------------------------------------------------------------------------------
void Mat44::Append(Mat44 const& appendThis)
{


	Mat44 copyMatrix = Mat44(m_values);
	
	// Row X: Ix, Jx, Kx, Tx
	m_values[Ix] = (copyMatrix.m_values[Ix] * appendThis.m_values[Ix]) + (copyMatrix.m_values[Jx] * appendThis.m_values[Iy]) + (copyMatrix.m_values[Kx] * appendThis.m_values[Iz]) + (copyMatrix.m_values[Tx] * appendThis.m_values[Iw]);

	m_values[Jx] = (copyMatrix.m_values[Ix] * appendThis.m_values[Jx]) + (copyMatrix.m_values[Jx] * appendThis.m_values[Jy]) + (copyMatrix.m_values[Kx] * appendThis.m_values[Jz]) + (copyMatrix.m_values[Tx] * appendThis.m_values[Jw]);   

	m_values[Kx] = (copyMatrix.m_values[Ix] * appendThis.m_values[Kx]) + (copyMatrix.m_values[Jx] * appendThis.m_values[Ky]) + (copyMatrix.m_values[Kx] * appendThis.m_values[Kz]) + (copyMatrix.m_values[Tx] * appendThis.m_values[Kw]);

	m_values[Tx] = (copyMatrix.m_values[Ix] * appendThis.m_values[Tx]) + (copyMatrix.m_values[Jx] * appendThis.m_values[Ty]) + (copyMatrix.m_values[Kx] * appendThis.m_values[Tz]) + (copyMatrix.m_values[Tx] * appendThis.m_values[Tw]);

	// Row Y: Iy, Jy, Ky, Ty
	m_values[Iy] = (copyMatrix.m_values[Iy] * appendThis.m_values[Ix]) + (copyMatrix.m_values[Jy] * appendThis.m_values[Iy]) + (copyMatrix.m_values[Ky] * appendThis.m_values[Iz]) + (copyMatrix.m_values[Ty] * appendThis.m_values[Iw]);

	m_values[Jy] = (copyMatrix.m_values[Iy] * appendThis.m_values[Jx]) + (copyMatrix.m_values[Jy] * appendThis.m_values[Jy]) + (copyMatrix.m_values[Ky] * appendThis.m_values[Jz]) + (copyMatrix.m_values[Ty] * appendThis.m_values[Jw]);

	m_values[Ky] = (copyMatrix.m_values[Iy] * appendThis.m_values[Kx]) + (copyMatrix.m_values[Jy] * appendThis.m_values[Ky]) + (copyMatrix.m_values[Ky] * appendThis.m_values[Kz]) + (copyMatrix.m_values[Ty] * appendThis.m_values[Kw]);

	m_values[Ty] = (copyMatrix.m_values[Iy] * appendThis.m_values[Tx]) + (copyMatrix.m_values[Jy] * appendThis.m_values[Ty]) + (copyMatrix.m_values[Ky] * appendThis.m_values[Tz]) + (copyMatrix.m_values[Ty] * appendThis.m_values[Tw]);

	// Row Z: Iz, Jz, Kz, Tz
	m_values[Iz] = (copyMatrix.m_values[Iz] * appendThis.m_values[Ix]) + (copyMatrix.m_values[Jz] * appendThis.m_values[Iy]) + (copyMatrix.m_values[Kz] * appendThis.m_values[Iz]) + (copyMatrix.m_values[Tz] * appendThis.m_values[Iw]);

	m_values[Jz] = (copyMatrix.m_values[Iz] * appendThis.m_values[Jx]) + (copyMatrix.m_values[Jz] * appendThis.m_values[Jy]) + (copyMatrix.m_values[Kz] * appendThis.m_values[Jz]) + (copyMatrix.m_values[Tz] * appendThis.m_values[Jw]);

	m_values[Kz] = (copyMatrix.m_values[Iz] * appendThis.m_values[Kx]) + (copyMatrix.m_values[Jz] * appendThis.m_values[Ky]) + (copyMatrix.m_values[Kz] * appendThis.m_values[Kz]) + (copyMatrix.m_values[Tz] * appendThis.m_values[Kw]);

	m_values[Tz] = (copyMatrix.m_values[Iz] * appendThis.m_values[Tx]) + (copyMatrix.m_values[Jz] * appendThis.m_values[Ty]) + (copyMatrix.m_values[Kz] * appendThis.m_values[Tz]) + (copyMatrix.m_values[Tz] * appendThis.m_values[Tw]);


	// Row W: Iw, Jw, Kw, Tw
	m_values[Iw] = (copyMatrix.m_values[Iw] * appendThis.m_values[Ix]) + (copyMatrix.m_values[Jw] * appendThis.m_values[Iy]) + (copyMatrix.m_values[Kw] * appendThis.m_values[Iz]) + (copyMatrix.m_values[Tw] * appendThis.m_values[Iw]);

	m_values[Jw] = (copyMatrix.m_values[Iw] * appendThis.m_values[Jx]) + (copyMatrix.m_values[Jw] * appendThis.m_values[Jy]) + (copyMatrix.m_values[Kw] * appendThis.m_values[Jz]) + (copyMatrix.m_values[Tw] * appendThis.m_values[Jw]);

	m_values[Kw] = (copyMatrix.m_values[Iw] * appendThis.m_values[Kx]) + (copyMatrix.m_values[Jw] * appendThis.m_values[Ky]) + (copyMatrix.m_values[Kw] * appendThis.m_values[Kz]) + (copyMatrix.m_values[Tw] * appendThis.m_values[Kw]);

	m_values[Tw] = (copyMatrix.m_values[Iw] * appendThis.m_values[Tx]) + (copyMatrix.m_values[Jw] * appendThis.m_values[Ty]) + (copyMatrix.m_values[Kw] * appendThis.m_values[Tz]) + (copyMatrix.m_values[Tw] * appendThis.m_values[Tw]);

}


//------------------------------------------------------------------------------------------------------------------
void Mat44::AppendZRotation(float degreesRotationAboutZ)
{

	Mat44 zRotationMatrix = MakeZRotationDegrees(degreesRotationAboutZ);

	Append(zRotationMatrix);
}



//------------------------------------------------------------------------------------------------------------------
void Mat44::AppendYRotation(float degreesRotationAboutY)
{

	Mat44 yRotationMatrix = MakeYRotationDegrees(degreesRotationAboutY);

	Append(yRotationMatrix);

}


//------------------------------------------------------------------------------------------------------------------
void Mat44::AppendXRotation(float degreesRotationAboutX)
{
	Mat44 xRotationMatrix = MakeXRotationDegrees(degreesRotationAboutX);

	Append(xRotationMatrix);
}


//------------------------------------------------------------------------------------------------------------------
void Mat44::AppendTranslation2D(Vec2 const& translationXY)
{

	Mat44 translation2DMatrix = MakeTranslation2D(translationXY);
	Append(translation2DMatrix);

}


//------------------------------------------------------------------------------------------------------------------
void Mat44::AppendTranslation3D(Vec3 const& translationXYZ)
{

	Mat44 translation3DMatrix = MakeTranslation3D(translationXYZ);
	Append(translation3DMatrix);

}


//------------------------------------------------------------------------------------------------------------------
void Mat44::AppendScaleUniform2D(float uniformScaleXY)
{

	Mat44 uniformScale2DMatrix = MakeUniformScale2D(uniformScaleXY);
	Append(uniformScale2DMatrix);

}


//------------------------------------------------------------------------------------------------------------------
void Mat44::AppendScaleUniform3D(float uniformScaleXYZ)
{

	Mat44 uniformScale3DMatrix = MakeUniformScale3D(uniformScaleXYZ);
	Append(uniformScale3DMatrix);

}


//------------------------------------------------------------------------------------------------------------------
void Mat44::AppendScaleNonUniform2D(Vec2 const& nonUniformScaleXY)
{

	Mat44 nonUniformScale2DMatrix = MakeNonUniformScale2D(nonUniformScaleXY);
	Append(nonUniformScale2DMatrix);

}


//------------------------------------------------------------------------------------------------------------------
void Mat44::AppendScaleNonUniform3D(Vec3 const& nonUniformScaleXYZ)
{
	Mat44 nonUniformScale3DMatrix = MakeNonUniformScale3D(nonUniformScaleXYZ);
	Append(nonUniformScale3DMatrix);
}


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Mat44::GetAsMat34(float out_Mat[3][4])
{
	out_Mat[0][0] = m_values[0];
	out_Mat[0][1] = m_values[1];
	out_Mat[0][2] = m_values[2];
	out_Mat[0][3] = m_values[3];
	out_Mat[1][0] = m_values[4];
	out_Mat[1][1] = m_values[5];
	out_Mat[1][2] = m_values[6];
	out_Mat[1][3] = m_values[7];
	out_Mat[2][0] = m_values[8];
	out_Mat[2][1] = m_values[9];
	out_Mat[2][2] = m_values[10];
	out_Mat[2][3] = m_values[11];
}


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool Mat44::operator==(Mat44 const& compare) const
{
	for(int index = 0; index < NUM_INDEXES; ++index)
	{
		if(m_values[index] != compare.m_values[index])
		{
			return false;
		}
	}
	return true;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool Mat44::operator!=(Mat44 const& compare) const
{
	for(int index = 0; index < NUM_INDEXES; ++index)
	{
		if(m_values[index] != compare.m_values[index])
		{
			return true;
		}
	}
	return false;
}



