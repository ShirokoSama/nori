#include <nori/emitter.h>

NORI_NAMESPACE_BEGIN

class AreaLight : public Emitter{
public:
	AreaLight(const PropertyList &props) {
		m_radiance = props.getColor("radiance");
	}

	SampleOnEmitter sample(Point2f &sample) const {
		SampleOnMesh sm = m_mesh->samplePosition(sample);
		SampleOnEmitter se;
		se.position = sm.position;
		se.normal = sm.normal;
		se.lightEnergy = m_radiance / sm.probabilityDensity;
		return se;
	}

	Color3f le() const {
		return m_radiance;
	}

	std::string toString() const {
		return tfm::format(
			"AreaLight[\n"
			"  radiance = \"%s\"\n"
			"]",
			m_radiance.toString()
		);
	}
private:
	Color3f m_radiance;
};

NORI_REGISTER_CLASS(AreaLight, "area")
NORI_NAMESPACE_END