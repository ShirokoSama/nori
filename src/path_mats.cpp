#include <nori/integrator.h>
#include <nori/scene.h>
#include <nori/sampler.h>
#include <nori/emitter.h>
#include <nori/bsdf.h>

NORI_NAMESPACE_BEGIN

class PathMatsIntegrator : public Integrator {
public:
	PathMatsIntegrator(const PropertyList &props) {}

	Color3f Li(const Scene *scene, Sampler *sampler, const Ray3f &ray) const {
		return Li(scene, sampler, ray, 0);
	}

	Color3f Li(const Scene *scene, Sampler *sampler, const Ray3f &ray, int depth) const {
		if (depth >= 3)
			return Color3f(0.0f);

		Intersection its;
		if (!scene->rayIntersect(ray, its))
			return Color3f(0.0f);

		Color3f li(0.0f);
		if (its.mesh->isEmitter()) {
			const Emitter *emitter = its.mesh->getEmitter();
			return emitter->le();
		}

		const BSDF *bsdf = its.mesh->getBSDF();
		if (!bsdf->isDiffuse()) {
			Point2f sample = sampler->next2D();
			BSDFQueryRecord bsdfQueryRecord(
				its.shFrame.toLocal(-ray.d)
			);
			Color3f c = bsdf->sample(bsdfQueryRecord, sample);
			float u = sampler->next1D();
			if (u < 0.95f) {
				Ray3f rRay(its.p, its.shFrame.toWorld(bsdfQueryRecord.wo));
				li += (Li(scene, sampler, rRay, depth) / 0.95f);
			}
			else {
				return Color3f(0.0f);
			}
		}
		else {
			Color3f lo(0.0f);
			for (size_t i = 0; i < sampler->getSampleCount(); i++) {
				Point2f sample = sampler->next2D();
				BSDFQueryRecord bsdfQueryRecord(its.shFrame.toLocal(-ray.d));
				Color3f sampleRslt = bsdf->sample(bsdfQueryRecord, sample);
				Color3f liRslt = Li(scene, sampler, Ray3f(its.p, its.shFrame.toWorld(bsdfQueryRecord.wo)), depth + 1);
				lo += sampleRslt * liRslt;
			}
			li += lo / static_cast<float>(sampler->getSampleCount());
		}

		return li;
	}

	std::string toString() const {
		return std::string("PathMatsIntegrator[]");
	}

};

NORI_REGISTER_CLASS(PathMatsIntegrator, "path_mats")
NORI_NAMESPACE_END