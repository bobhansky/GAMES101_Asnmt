//
// Created by Göksu Güvendiren on 2019-05-14.
//

#include "Scene.hpp"


void Scene::buildBVH() {
    printf(" - Generating BVH...\n\n");
    this->bvh = new BVHAccel(objects, 1, BVHAccel::SplitMethod::NAIVE);
}

Intersection Scene::intersect(const Ray &ray) const
{
    return this->bvh->Intersect(ray);
}


// 很简单，我们可以每次在面光源上采样一个点（Q点，面积dA的区域），
// 然后不再是随机采样一个方向射出光线，而是直接将光线射向Q点，
// https://zhuanlan.zhihu.com/p/370162390
void Scene::sampleLight(Intersection &pos, float &pdf) const
{
    float emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()){
            emit_area_sum += objects[k]->getArea();
        }
    }
    float p = get_random_float() * emit_area_sum;
    emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()){
            emit_area_sum += objects[k]->getArea();
            if (p <= emit_area_sum){
                objects[k]->Sample(pos, pdf);
                break;
            }
        }
    }
}

bool Scene::trace(
        const Ray &ray,
        const std::vector<Object*> &objects,
        float &tNear, uint32_t &index, Object **hitObject)
{
    *hitObject = nullptr;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        float tNearK = kInfinity;
        uint32_t indexK;
        Vector2f uvK;
        if (objects[k]->intersect(ray, tNearK, indexK) && tNearK < tNear) {
            *hitObject = objects[k];
            tNear = tNearK;
            index = indexK;
        }
    }


    return (*hitObject != nullptr);
}

// Implementation of Path Tracing
Vector3f Scene::castRay(const Ray &ray, int depth) const
{
    // TO DO Implement Path Tracing Algorithm here

    Vector3f L_dir = { 0,0,0 };
    Vector3f L_indir = { 0,0,0 };
   
    // get intersecion point p    intersection between eyes ray and object plane
    Intersection p_inter = this->intersect(ray);

    if (p_inter.happened) {
        // if hit the light source
        if (p_inter.m->hasEmission())
            return p_inter.m->getEmission();


        
        // *****************direct contribution from the light source*****************
        float pdf_light = 0.0f;
        Intersection light_inter;     // x_prime      inter between ray and light plane   A RANDOM SAMPLE from light area
        sampleLight(light_inter, pdf_light);    // sample from lights -> get light infos & pdf
        

        //  Get x, ws, NN, emit from inter(my notes: inter with light plane)
        Vector3f p = p_inter.coords;
        Vector3f p_to_light_dir = (light_inter.coords - p).normalized();   // ws
        float dis_square = dotProduct(light_inter.coords - p_inter.coords, light_inter.coords - p_inter.coords);
        Vector3f p_normal = p_inter.normal.normalized();        // N
        Vector3f light_normal = light_inter.normal.normalized();    // NN
        Vector3f emit = light_inter.emit;           // Li from light

        
        // Shoot a ray from p to x
        Ray ray_ws(p, p_to_light_dir);  // ray from p to light
        Intersection ws_inter = this->intersect(ray_ws);    // intersection from p to light

        // If the ray is not blocked in the middle
        if (ws_inter.happened && fabs(ws_inter.distance - sqrt(dis_square)) < EPSILON) {
            // L_dir = emit * eval(wo , ws , N) * dot(ws , N) *             dot(ws , N)== cos theta
            // dot(ws , NN) / |x - p | ^ 2 / pdf_light                      dot(ws , NN) == cos theta prime
            // wo: ray_to_p

            Vector3f L_i = light_inter.emit;
            Vector3f f_r = p_inter.m->eval(ray.direction, p_to_light_dir, p_normal);
            float cos_theta = dotProduct(p_normal, p_to_light_dir);
            float cos_theta_prime = dotProduct(light_normal, -p_to_light_dir);
            L_dir = L_i * f_r * cos_theta * cos_theta_prime /dis_square / pdf_light;
        }
        //  *****************direct contribution from the light source ENDS*****************
       
        
        
        // ******************************indirect illumination*************************************

        // test Russian Roulette
        if (get_random_float() > RussianRoulette) {
            return L_dir;
        }

        // get a random Wo direction
        Vector3f p_to_object_dir = p_inter.m->sample(ray.direction, p_inter.normal.normalized()).normalized();
        Ray ray_p_object(p_inter.coords, p_to_object_dir);

        // if ray intersects with a non emitting object at q
        Intersection q = this->intersect(ray_p_object);
        if (q.happened && !q.m->hasEmission()) {
            Vector3f f_r = p_inter.m->eval(ray.direction, p_to_object_dir, p_normal);
            float cos_theta = dotProduct(p_normal, p_to_object_dir);
            float pdf = p_inter.m->pdf(ray.direction, p_to_object_dir, p_normal);

            L_indir = castRay(ray_p_object, depth + 1) * f_r * cos_theta / pdf / RussianRoulette;
        }
        
    }

    return L_dir + L_indir;
}


// https://blog.csdn.net/qq_41765657/article/details/121942469
// https://blog.csdn.net/weixin_44142774/article/details/116888543


// https://zhuanlan.zhihu.com/p/488882096  ***

// https://zhuanlan.zhihu.com/p/370162390  ***