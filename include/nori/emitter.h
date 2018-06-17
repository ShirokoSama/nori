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

#pragma once

#include <nori/object.h>
#include <nori/mesh.h>

NORI_NAMESPACE_BEGIN

struct SampleOnEmitter {
	Point3f position;
	Vector3f normal;
	Color3f lightEnergy;
};

/**
 * \brief Superclass of all emitters
 */
class Emitter : public NoriObject {
public:
	void setParent(NoriObject *parent) {
		if (NoriObject::classTypeName(parent->getClassType()) == "mesh") {
			Mesh *mesh = static_cast<Mesh *>(parent);
			if (mesh == m_mesh)
				return;
			if (m_mesh != nullptr)
				throw NoriException("An emitter cannot have multiple mesh parents");
			m_mesh = mesh;
		}
		else
			throw NoriException("An emitter must be a child of mesh");
	}

	virtual SampleOnEmitter sample(Point2f &sample) const = 0;

	virtual Color3f le() const = 0;

    /**
     * \brief Return the type of object (i.e. Mesh/Emitter/etc.) 
     * provided by this instance
     * */
    EClassType getClassType() const { return EEmitter; }
protected:
	Mesh * m_mesh = nullptr;
};

NORI_NAMESPACE_END
