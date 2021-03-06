#include <nori/integrator.h>
#include <nori/scene.h>
#include <nori/sampler.h>
#include <nori/emitter.h>
#include <nori/bsdf.h>

NORI_NAMESPACE_BEGIN

class PathEmsIntegrator : public Integrator {
public:
	PathEmsIntegrator(const PropertyList &props) {}

	Color3f Li(const Scene *scene, Sampler *sampler, const Ray3f &ray) const {
		bool indirectFlag = false;
		return Li(scene, sampler, ray, 0, &indirectFlag);
	}

	Color3f Li(const Scene *scene, Sampler *sampler, const Ray3f &ray, int depth, bool *indirect) const {
		if (depth >= 3)
			return Color3f(0.0f);

		Intersection its;
		if (!scene->rayIntersect(ray, its))
			return Color3f(0.0f);

		Color3f li(0.0f);
		if (its.mesh->isEmitter()) {
			*indirect = false;
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
				bool indirectFlag = false;
				li += (Li(scene, sampler, rRay, depth, &indirectFlag) / 0.95f);
			}
			else {
				return Color3f(0.0f);
			}
		}
		else {
			// direct illumination part
			std::vector<Mesh *> meshes = scene->getMeshes();
			for (auto itr = meshes.cbegin(); itr != meshes.cend(); itr++) {
				Mesh *curMesh = *itr;
				if (curMesh->isEmitter()) {
					const Emitter *curEmitter = curMesh->getEmitter();
					Color3f lr(0.0f);
					for (size_t i = 0; i < sampler->getSampleCount(); i++) {
						Point2f sample = sampler->next2D();
						SampleOnEmitter soe = curEmitter->sample(sample);
						Vector3f dir = soe.position - its.p;
						Ray3f tRay(its.p, dir);
						Intersection tIst;
						if (scene->rayIntersect(tRay, tIst)
							&& tIst.p.isApprox(soe.position)
							&& soe.normal.dot(-dir) > 0) {
							BSDFQueryRecord bsdfQueryRecord(
								its.shFrame.toLocal(dir),
								its.shFrame.toLocal(-ray.d),
								ESolidAngle
							);
							Color3f fr = bsdf->eval(bsdfQueryRecord);
							float gxy =
								std::abs(its.shFrame.n.dot(dir.normalized())) *
								std::abs(soe.normal.dot(-dir.normalized())) /
								dir.dot(dir);
							lr += gxy * fr * soe.lightEnergy;
						}
					}
					li += (lr / static_cast<float>(sampler->getSampleCount()));
				}
			}
			// indirect illumination part
			size_t discardCount = 0;
			Color3f lii(0.0f);
			for (size_t i = 0; i < sampler->getSampleCount(); i++) {
				Point2f sample = sampler->next2D();
				BSDFQueryRecord bsdfQueryRecord(its.shFrame.toLocal(-ray.d));
				Color3f sampleRslt = bsdf->sample(bsdfQueryRecord, sample);
				bool indirectFlag = true;
				Color3f liRslt = Li(scene, sampler, Ray3f(its.p, its.shFrame.toWorld(bsdfQueryRecord.wo)), depth + 1, &indirectFlag);
				if (indirectFlag)
					lii += sampleRslt * liRslt;
				else
					discardCount++;
			}
			if (sampler->getSampleCount() > discardCount)
				li += lii / static_cast<float>(sampler->getSampleCount() - discardCount);
		}

		return li;
	}

	std::string toString() const {
		return std::string("PathEMSIntegrator[]");
	}
};

NORI_REGISTER_CLASS(PathEmsIntegrator, "path_ems")
NORI_NAMESPACE_END