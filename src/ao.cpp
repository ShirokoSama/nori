#include <nori/integrator.h>
#include <nori/scene.h>
#include <nori/sampler.h>
#include <nori/warp.h>

NORI_NAMESPACE_BEGIN

class AoIntegrator : public Integrator {
public:
	AoIntegrator(const PropertyList &props) {}

	Color3f Li(const Scene *scene, Sampler *sampler, const Ray3f &ray) const {
		Intersection its;
		if (!scene->rayIntersect(ray, its))
			return Color3f(0.0f);

		float sum = 0.0f;
		Frame shFrame = its.shFrame;
		Point3f fragPosition = its.p;
		for (size_t i = 0; i < sampler->getSampleCount(); i++) {
			Vector3f sample = Warp::squareToCosineHemisphere(sampler->next2D());
			Vector3f dir = shFrame.toWorld(sample);
			Ray3f shadowRay(fragPosition, dir);
			if (!scene->rayIntersect(shadowRay))
				sum += 1.f;
		}
		return Color3f(sum / static_cast<float>(sampler->getSampleCount()));
	}

	std::string toString() const {
		return std::string("AoIntegrator[]");
	}
};

NORI_REGISTER_CLASS(AoIntegrator, "ao")
NORI_NAMESPACE_END