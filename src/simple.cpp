#include <nori/integrator.h>
#include <nori/scene.h>

NORI_NAMESPACE_BEGIN

class SimpleIntegrator : public Integrator {
public:
	SimpleIntegrator(const PropertyList &props) {
		m_myProperty = props.getString("myProperty");
		m_position = props.getPoint("position");
		m_energy = props.getColor("energy");
	}

	Color3f Li(const Scene *scene, Sampler *sampler, const Ray3f &ray) const {
		Intersection its;
		if (!scene->rayIntersect(ray, its))
			return Color3f(0.0f);

		Vector3f dir = m_position - its.p;
		Ray3f shadowRay(its.p, dir);
		if (scene->rayIntersect(shadowRay))
			return Color3f(0.0f);

		Vector3f normal = its.shFrame.n.normalized();
		float coefficient = std::max(0.0f, normal.dot(dir.normalized())) 
			/ (4 * M_PI * M_PI * dir.dot(dir));
		return m_energy * coefficient;
	}

	std::string toString() const {
		return tfm::format(
			"SimpleIntegrator[\n"
			"  myProperty = \"%s\"\n"
			"  position = \"%s\"\n"
			"  m_energy = \"%s\"\n"
			"]",
			m_myProperty,
			m_position.toString(),
			m_energy.toString()
		);
	}
private:
	std::string m_myProperty;
	Point3f m_position;
	Color3f m_energy;
};

NORI_REGISTER_CLASS(SimpleIntegrator, "simple")
NORI_NAMESPACE_END