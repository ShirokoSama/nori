#include <nori/integrator.h>
#include <nori/scene.h>
#include <nori/sampler.h>
#include <nori/emitter.h>
#include <nori/bsdf.h>

NORI_NAMESPACE_BEGIN

class WhittedIntegrator : public Integrator {
public:
	WhittedIntegrator(const PropertyList &props) {}

	Color3f Li(const Scene *scene, Sampler *sampler, const Ray3f &ray) const {
		Intersection its;
		if (!scene->rayIntersect(ray, its))
			return Color3f(0.0f);

		Color3f li(0.0f);
		if (its.mesh->isEmitter()) {
			const Emitter *emitter = its.mesh->getEmitter();
			//li += emitter->le() * its.shFrame.n.dot(-ray.d.normalized());
			li += emitter->le();
		}
		
		const BSDF *bsdf = its.mesh->getBSDF();
		if (bsdf->isDiffuse()) {
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
		}
		else {
			Point2f sample = sampler->next2D();
			BSDFQueryRecord bsdfQueryRecord(
				its.shFrame.toLocal(-ray.d)
			);
			Color3f c = bsdf->sample(bsdfQueryRecord, sample);
			float u = sampler->next1D();
			if (u < 0.95f) {
				Ray3f rRay(its.p, its.shFrame.toWorld(bsdfQueryRecord.wo));
				li += (Li(scene, sampler, rRay) / 0.95f);
			}
			else {
				return Color3f(0.0f);
			}
		}

		return li;
	}

	std::string toString() const {
		return std::string("WhittedIntegrator[]");
	}
};

NORI_REGISTER_CLASS(WhittedIntegrator, "whitted")
NORI_NAMESPACE_END