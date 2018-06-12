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

#include <nori/mesh.h>

NORI_NAMESPACE_BEGIN

/**
 * \brief Acceleration data structure for ray intersection queries
 *
 * The current implementation falls back to a brute force loop
 * through the geometry.
 */
class Accel {
public:

	// Create a new and empty BVH
	Accel() { m_meshOffset.push_back(0u); }

	// Release all resources
	virtual ~Accel() {
		for (auto mesh : m_meshes)
			delete mesh;
		m_meshes.clear();
		m_meshes.shrink_to_fit();
		m_indices.clear();
		m_indices.shrink_to_fit();
		m_meshOffset.clear();
		m_meshOffset.push_back(0u);
		m_meshOffset.shrink_to_fit();
		m_bbox.reset();
	};

    /**
     * \brief Register a triangle mesh for inclusion in the acceleration
     * data structure
     *
     * This function can only be used before \ref build() is called
     */
    void addMesh(Mesh *mesh);

    // Build the acceleration data structure (currently a no-op)
    void build();

    /**
     * \brief Intersect a ray against all triangles stored in the scene and
     * return detailed intersection information
     *
     * \param ray
     *    A 3-dimensional ray data structure with minimum/maximum extent
     *    information
     *
     * \param its
     *    A detailed intersection record, which will be filled by the
     *    intersection query
     *
     * \param shadowRay
     *    \c true if this is a shadow ray query, i.e. a query that only aims to
     *    find out whether the ray is blocked or not without returning detailed
     *    intersection information.
     *
     * \return \c true if an intersection was found
     */
    bool rayIntersect(const Ray3f &ray, Intersection &its, bool shadowRay = false) const;

	/// Return the total number of meshes registered with Octree
	uint32_t getMeshCount() const { return static_cast<uint32_t>(m_meshes.size()); }

	/// Return the total number of internally represented triangles
	uint32_t getTriangleCount() const { return static_cast<uint32_t>(m_meshOffset.back()); }

	/// Return an axis-aligned bounding box containing the entire tree
	const BoundingBox3f &getBoundingBox() const { return m_bbox; }

protected:
	/**
	* \brief Compute the mesh and triangle indices corresponding to
	* a primitive index used by the underlying generic Octree implementation.
	*/
	uint32_t findMeshIndex(uint32_t &idx) const {
		auto it = std::lower_bound(m_meshOffset.begin(), m_meshOffset.end(), idx + 1) - 1;
		idx -= *it;
		return (uint32_t)(it - m_meshOffset.begin());
	}

	// Return an axis-aligned bounding box containing the given triangle
	BoundingBox3f getBoundingBox(uint32_t index) const {
		uint32_t meshIdx = findMeshIndex(index);
		return m_meshes[meshIdx]->getBoundingBox(index);
	}

	/// Return the centroid of the given triangle
	Point3f getCentroid(uint32_t index) const {
		uint32_t meshIdx = findMeshIndex(index);
		return m_meshes[meshIdx]->getCentroid(index);
	}

	struct OctreeNode {
		union {
			struct {
				uint16_t depth;
				unsigned flag : 1;
				uint16_t size : 15;
				uint32_t start;
			} node;
			
			uint64_t data;
		};
		uint32_t axisOffset[3];

		bool isLeaf() const { return node.flag == 1; }

		bool isInner() const { return node.flag == 0; }

		bool isUnused() const { return data == 0; }

		uint32_t getStart() const { return node.start; }

		uint32_t getEnd() const { return node.start + node.size; }

		// calculate node's bounding box by bbox of the entire scene
		const BoundingBox3f getBoundingBox(BoundingBox3f &bbox) const {
			Point3f size = (bbox.max - bbox.min) / (1 << node.depth);
			Point3f min(
				bbox.min[0] + size[0] * axisOffset[0],
				bbox.min[1] + size[1] * axisOffset[1], 
				bbox.min[2] + size[2] * axisOffset[2]
			);
			Point3f max = min + size;
			return BoundingBox3f(min, max);
		}
	};

private:
	std::vector<Mesh *> m_meshes;			///< List of meshes registered with Octree
	std::vector<uint32_t> m_indices;		///< Index references by Octree nodes
	std::vector<uint32_t> m_meshOffset;		///< Index of the first triangle for each shape
    BoundingBox3f m_bbox;					///< Bounding box of the entire scene
};

NORI_NAMESPACE_END
