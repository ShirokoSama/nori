#include <nori/integrator.h>
#include <nori/scene.h>
#include <nori/sampler.h>
#include <nori/emitter.h>
#include <nori/bsdf.h>

NORI_NAMESPACE_BEGIN

class PathMisIntegrator : public Integrator {
public:
	PathMisIntegrator(const PropertyList &props) {}

	Color3f Li(const Scene *scene, Sampler *sampler, const Ray3f &ray) const {
		SampleOnEmitter soe;
		return Li(scene, sampler, ray, 0, soe);
	}

	Color3f Li(const Scene *scene, Sampler *sampler, const Ray3f &ray, int depth, SampleOnEmitter &soeFlag) const {
		if (depth >= 3)
			return Color3f(0.0f);

		Intersection its;
		if (!scene->rayIntersect(ray, its))
			return Color3f(0.0f);

		Color3f li(0.0f);
		if (its.mesh->isEmitter()) {
			const Emitter *emitter = its.mesh->getEmitter();
			soeFlag.position = its.p;
			soeFlag.probabilityDensity = 1.f / its.mesh->getSurfaceArea();
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
				SampleOnEmitter soe;
				li += (Li(scene, sampler, rRay, depth, soe) / 0.95f);
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
							// multiple importance sample weight when sample on light
							float pBSDF = bsdf->pdf(bsdfQueryRecord);
							pBSDF /= dir.dot(dir);
							float wLight = soe.probabilityDensity / (soe.probabilityDensity + pBSDF);
							lr += wLight * gxy * fr * soe.lightEnergy;
						}
					}
					li += (lr / static_cast<float>(sampler->getSampleCount()));
				}
			}
			// indirect illumination part
			Color3f lii(0.0f);
			for (size_t i = 0; i < sampler->getSampleCount(); i++) {
				Point2f sample = sampler->next2D();
				BSDFQueryRecord bsdfQueryRecord(its.shFrame.toLocal(-ray.d));
				Color3f sampleRslt = bsdf->sample(bsdfQueryRecord, sample);
				SampleOnEmitter soe;
				soe.probabilityDensity = 0;
				Color3f liRslt = Li(scene, sampler, Ray3f(its.p, its.shFrame.toWorld(bsdfQueryRecord.wo)), depth + 1, soe);
				// multiple importance sample weight when hit a light
				float pBSDF = bsdf->pdf(bsdfQueryRecord);
				Vector3f dir = soe.position - its.p;
				pBSDF /= dir.dot(dir);
				float wBSDF = 1.f;
				if (soe.probabilityDensity != 0)
					wBSDF = pBSDF / (pBSDF + soe.probabilityDensity);
				lii += wBSDF * sampleRslt * liRslt;
			}
			li += lii / static_cast<float>(sampler->getSampleCount());
		}

		return li;
	}

	std::string toString() const {
		return std::string("PathMisIntegrator[]");
	}
};

NORI_REGISTER_CLASS(PathMisIntegrator, "path_mis")
NORI_NAMESPACE_END