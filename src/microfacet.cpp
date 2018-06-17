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
		return (wi + wo).normalized();
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
		return 1.f / (4 * wh.dot(wo));
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
		return diffuse + specular;
    }

    /// Evaluate the sampling density of \ref sample() wrt. solid angles
    float pdf(const BSDFQueryRecord &bRec) const {
		Vector3f wi = bRec.wi.normalized();
		Vector3f wo = bRec.wo.normalized();
		Vector3f wh = getWh(wi, wo);
		if (wh.z() <= 0)
			return 0.f;
		float d = ndf(wh);
		float jh = getJh(wh, wo);
		return m_ks * d * jh + (1 - m_ks) * Warp::squareToCosineHemispherePdf(wo);
    }

    /// Sample the BRDF
    Color3f sample(BSDFQueryRecord &bRec, const Point2f &_sample) const {
		bRec.eta = 1.0f;
		bRec.measure = ESolidAngle;

		float ux = _sample.x();
		float uy = _sample.y();

		if (ux < m_ks) {
			// specular case
			Vector3f normal = Warp::squareToBeckmann(Point2f(ux / m_ks, uy), m_alpha);
			Frame nf(normal);
			Vector3f localWi = nf.toLocal(bRec.wi);
			bRec.wo = nf.toWorld(Vector3f(-localWi.x(), -localWi.y(), localWi.z())).normalized();
		}
		else {
			if (Frame::cosTheta(bRec.wi) <= 0)
				return Color3f(0.0f);
			bRec.wo = Warp::squareToCosineHemisphere(Point2f((ux - m_ks) / (1 - m_ks), uy));
		}

        return eval(bRec) * Frame::cosTheta(bRec.wo) / pdf(bRec);
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
