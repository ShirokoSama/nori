/*
    This file is part of Nori, a simple educational ray tracer

    Copyright (c) 2015 by Wenzel Jakob

    Nori is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License Version 3
    as published by the Free Software Foundation.

    Nori is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <nori/warp.h>
#include <nori/vector.h>
#include <nori/frame.h>

NORI_NAMESPACE_BEGIN

Point2f Warp::squareToUniformSquare(const Point2f &sample) {
    return sample;
}

float Warp::squareToUniformSquarePdf(const Point2f &sample) {
    return ((sample.array() >= 0).all() && (sample.array() <= 1).all()) ? 1.0f : 0.0f;
}

Point2f Warp::squareToTent(const Point2f &sample) {
	float ux = sample.x();
	float uy = sample.y();
	auto inverseCDF = [](float uniform) -> float {
		if (uniform >= 0 && uniform < 0.5f)
			return std::sqrt(2 * uniform) - 1.f;
		else if (uniform >= 0.5f && uniform <= 1)
			return 1.f - std::sqrt(2 - 2 * uniform);
		else
			throw NoriException("uniform samples are not between 0 and 1!");
	};
	return Point2f(inverseCDF(ux), inverseCDF(uy));
}

float Warp::squareToTentPdf(const Point2f &p) {
	return ((p.array() >= -1).all() && (p.array() <= 1).all()) ? 
		(1 - std::abs(p.x())) * (1 - std::abs(p.y())) : 0.0f;
}

Point2f Warp::squareToUniformDisk(const Point2f &sample) {
	float ux = sample.x();
	float uy = sample.y();
	float radius = std::sqrt(ux);
	float theta = 2.f * M_PI * uy;
	return Point2f(std::cos(theta) * radius, std::sin(theta) * radius);
}

float Warp::squareToUniformDiskPdf(const Point2f &p) {
	float x = p.x();
	float y = p.y();
	return (x * x + y * y <= 1.f) ? 1.f / M_PI : 0.0f;
}

Vector3f Warp::squareToUniformSphere(const Point2f &sample) {
	float ux = sample.x();
	float uy = sample.y();
	return Vector3f(
		std::cos(2 * M_PI * uy) * std::sqrt(ux - ux * ux) * 2.f,
		std::sin(2 * M_PI * uy) * std::sqrt(ux - ux * ux) * 2.f,
		1.f - 2 * ux
	);
}

float Warp::squareToUniformSpherePdf(const Vector3f &v) {
	return 1.f / (4 * M_PI);
}

Vector3f Warp::squareToUniformHemisphere(const Point2f &sample) {
	float ux = sample.x();
	float uy = sample.y();
	return Vector3f(
		std::cos(2 * M_PI * uy) * std::sqrt(1 - ux * ux),
		std::sin(2 * M_PI * uy) * std::sqrt(1 - ux * ux),
		ux
	);
}

float Warp::squareToUniformHemispherePdf(const Vector3f &v) {
	return v.z() >= 0 ? 1.f / (2 * M_PI) : 0;
}

Vector3f Warp::squareToCosineHemisphere(const Point2f &sample) {
/* directly method by calculate p(theta) and p(phi), use inverse method map uniform variables
	float ux = sample.x();
	float uy = sample.y();
	float theta = 0.5f * std::acos(1.f - 2.f * ux);
	float phi = 2 * M_PI * uy;
	return Vector3f(
		std::sin(theta) * std::cos(phi),
		std::sin(theta) * std::sin(phi),
		std::cos(theta)
	);
*/
	// use malley's method: generate points on unit disk 
	// and project them up to the hemisphere above it
	Point2f sampleOnDisk = squareToUniformDisk(sample);
	return Vector3f(
		sampleOnDisk.x(),
		sampleOnDisk.y(),
		std::sqrt(1.f - sampleOnDisk.x() * sampleOnDisk.x() - sampleOnDisk.y() * sampleOnDisk.y())
	);
}

float Warp::squareToCosineHemispherePdf(const Vector3f &v) {
	return v.z() >= 0 ? v.z() / M_PI : 0;
}

Vector3f Warp::squareToBeckmann(const Point2f &sample, float alpha) {
	float ux = sample.x();
	float uy = sample.y();
	float phi = 2 * M_PI * ux;
	float theta = std::atan(std::sqrt(-alpha * alpha*std::log(uy)));
	return Vector3f(
		std::sin(theta) * std::cos(phi),
		std::sin(theta) * std::sin(phi),
		std::cos(theta)
	);
}

float Warp::squareToBeckmannPdf(const Vector3f &m, float alpha) {
	float cosTheta = Frame::cosTheta(m);
	float tanTheta = Frame::tanTheta(m);
	return cosTheta > 0 ? 
		(1.f / M_PI) * 
		(std::exp(-std::pow(tanTheta, 2) / std::pow(alpha, 2)) /
		(std::pow(alpha, 2) * std::pow(cosTheta, 3)))
		: 0;
}

NORI_NAMESPACE_END
