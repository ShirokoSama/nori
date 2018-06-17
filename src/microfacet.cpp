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

#include <nori/bsdf.h>
#include <nori/frame.h>
#include <nori/warp.h>

NORI_NAMESPACE_BEGIN

class Microfacet : public BSDF {
public:
    Microfacet(const PropertyList &propList) {
        /* RMS surface roughness */
        m_alpha = propList.getFloat("alpha", 0.1f);

        /* Interior IOR (default: BK7 borosilicate optical glass) */
        m_intIOR = propList.getFloat("intIOR", 1.5046f);

        /* Exterior IOR (default: air) */
        m_extIOR = propList.getFloat("extIOR", 1.000277f);

        /* Albedo of the diffuse base material (a.k.a "kd") */
        m_kd = propList.getColor("kd", Color3f(0.5f));

        /* To ensure energy conservation, we must scale the 
           specular component by 1-kd. 

           While that is not a particularly realistic model of what 
           happens in reality, this will greatly simplify the 
           implementation. Please see the course staff if you're 
           interested in implementing a more realistic version 
           of this BRDF. */
        m_ks = 1 - m_kd.maxCoeff();
    }

	inline Vector3f getWh(const Vector3f &wi, const Vector3f &wo) const {
		return (wi + wo) / std::sqrt((wi + wo).dot(wi + wo));
	}

	inline float ndf(const Vector3f &wh) const {
		return Warp::squareToBeckmannPdf(wh, m_alpha);
	}

	inline float geometryFunc(const Vector3f &wi, const Vector3f &wo, const Vector3f &wh) const {
		return g1(wi, wh) * g1(wo, wh);
	}

	inline float g1(const Vector3f &wv, const Vector3f &wh) const {
		Vector3f normal(0.0f, 0.0f, 1.0f);
		float c = wv.dot(wh) / wv.dot(normal);
		if (c > 0) {
			float cosThetaV = wv.dot(normal);
			float tanTehtaV = std::sqrt(1 - cosThetaV * cosThetaV) / cosThetaV;
			float b = 1.f / (m_alpha * tanTehtaV);
			if (b < 1.6) {
				return (3.535f * b + 2.181f * b * b) / (1.f + 2.276f * b + 2.577f * b * b);
			}
			else
				return 1.f;
		}
		else
			return 0.0f;
	}

	inline float getJh(const Vector3f &wh, const Vector3f &wo) const {
		return 1.f / (4.f * wh.dot(wo));
	}

    /// Evaluate the BRDF for the given pair of directions
    Color3f eval(const BSDFQueryRecord &bRec) const {
		Vector3f wi = bRec.wi.normalized();
		Vector3f wo = bRec.wo.normalized();
		Vector3f wh = getWh(wi, wo);
		Color3f diffuse = m_kd * INV_PI;
		Color3f specular = m_ks * 
			((ndf(wh) * fresnel(wh.dot(wi), m_extIOR, m_intIOR) * geometryFunc(wi, wo, wh)) 
				/ (4.f * wi.z() * wo.z() * wh.z()));
    }

    /// Evaluate the sampling density of \ref sample() wrt. solid angles
    float pdf(const BSDFQueryRecord &bRec) const {
		Vector3f wi = bRec.wi.normalized();
		Vector3f wo = bRec.wo.normalized();
		Vector3f wh = getWh(wi, wo);
		float d = ndf(wh);
		float jh = getJh(wh, wo);
		float cosThetaO = wo.z();
		return m_ks * d * jh + (1 - m_ks) * cosThetaO * INV_PI;
    }

    /// Sample the BRDF
    Color3f sample(BSDFQueryRecord &bRec, const Point2f &_sample) const {
    	throw NoriException("MicrofacetBRDF::sample(): not implemented!");

        // Note: Once you have implemented the part that computes the scattered
        // direction, the last part of this function should simply return the
        // BRDF value divided by the solid angle density and multiplied by the
        // cosine factor from the reflection equation, i.e.
        // return eval(bRec) * Frame::cosTheta(bRec.wo) / pdf(bRec);
    }

    bool isDiffuse() const {
        /* While microfacet BRDFs are not perfectly diffuse, they can be
           handled by sampling techniques for diffuse/non-specular materials,
           hence we return true here */
        return true;
    }

    std::string toString() const {
        return tfm::format(
            "Microfacet[\n"
            "  alpha = %f,\n"
            "  intIOR = %f,\n"
            "  extIOR = %f,\n"
            "  kd = %s,\n"
            "  ks = %f\n"
            "]",
            m_alpha,
            m_intIOR,
            m_extIOR,
            m_kd.toString(),
            m_ks
        );
    }
private:
    float m_alpha;
    float m_intIOR, m_extIOR;
    float m_ks;
    Color3f m_kd;
};

NORI_REGISTER_CLASS(Microfacet, "microfacet");
NORI_NAMESPACE_END
