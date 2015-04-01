#include "../../../../include/odfaeg/Graphics/2D/ponctualLight.h"

#include "../../../../include/odfaeg/Graphics/transformMatrix.h"
#include "../../../../include/odfaeg/Physics/boundingPolyhedron.h"
namespace odfaeg {
    namespace graphic {
        namespace g2d {
            /*Cr�e une lumi�re de position poistion, suivant la direction dir, de rayon radius,
             et d'intensit� intensity, de type Type*/
            using namespace std;
            using namespace sf;
            //Cr�e une lumi�re avec sa position, son intensit� et son type.
            PonctualLight::PonctualLight (math::Vec3f center, float r1, float r2, float r3, float intensity, Color color, int quality, Entity *parent) :
                EntityLight (center, color, r1, r2, r3, height, "E_PONCTUAL_LIGHT", parent) {
                this->littleRadius = r1;
                this->bigRadius = r2;
                this->quality = quality;
                this->intensity = intensity;
                this->color.r = color.r * intensity / 255;
                this->color.g = color.g * intensity / 255;
                this->color.b = color.b * intensity / 255;

                this->intensity = intensity;
                initTriangles ();
            }

            //On initialise les triangles lumineux pour former une �lipse.
            void PonctualLight::initTriangles () {

                for (unsigned int i = 0; i < triangles.size(); i++) {
                    delete triangles[i];
                }
                triangles.clear();
                float angle = math::Math::toRadians(360) / quality;

                Color color2 (color.r, color.g, color.b, 0);
                int end = quality;

                for (int i = 0; i < end; i++) {
                    Vector3f v1, v2, v3;
                    v1 = Vector3f (0, 0, 0);
                    v2 = Vector3f (getSize().x * 0.5f * math::Math::cosinus (i * angle), getSize().y * 0.5f * math::Math::sinus (i * angle), 0);
                    v3 = Vector3f (getSize().x * 0.5f * math::Math::cosinus ((i + 1) * angle), getSize().y * 0.5f * math::Math::sinus ((i + 1) * angle), 0);
                    VertexArray *triangle = new VertexArray (Triangles);
                    triangle->append(Vertex(v1, color));
                    triangle->append(Vertex(v2, color2));
                    triangle->append(Vertex(v3, color2));
                    addTriangle(triangle);
                }
            }
            //Ajoute un triangle � la source lumineuse.
            void PonctualLight::addTriangle (VertexArray *triangle) {
                //A faire sur la render texture.
                //triangle->SetBlendMode(Blend::Add);
                //Plus n�cessaire avec la SFML 2.0.
                /*triangle->EnableFill(true);
                triangle->EnableOutline(false);*/
                triangles.push_back(triangle);
            }

            int PonctualLight::getIntensity() {
                return intensity;
            }
            vector<VertexArray*> PonctualLight::getTris () {
                return triangles;
            }
            void PonctualLight::onDraw(RenderTarget &target, RenderStates states) {
                for (unsigned int i = 0; i < triangles.size(); i++) {
                     states.transform = getTransform();
                     target.draw(*triangles[i], states);
                }
            }
            //Recherche l'intersection entre le triangle num�ro n de la lumi�re et le segment s1.
            void PonctualLight::checkInterTri (int n, math::Ray &s1) {

                int  i1 = -1, i2 = -1;
                math::Vec3f pointI1, pointI2;
                math::Ray s2;
                math::Vec3f a, b, c;
                c = getCenter();
                //On cr�e un polygon qui est le triangle lumineux et on cherche si les c�t�s du triangle intersecte le segment.
                for (unsigned int k = 0; k < triangles[n]->getVertexCount(); k++) {

                    math::Vec3f a = math::Vec3f ((*triangles[n])[k].position.x, (*triangles[n])[k].position.y, 0);
                    if (k + 1 == triangles[n]->getVertexCount()) {

                        b = math::Vec3f ((*triangles[n])[0].position.x, (*triangles[n])[0].position.y, 0);

                    } else {

                        b = math::Vec3f ((*triangles[n])[k + 1].position.x, (*triangles[n])[k + 1].position.y, 0);
                    }
                    s2 = math::Ray (a, b);
                    float iseg = s1.intersectsWhere(s2);
                    //Si les segments sont en intersection, on recherche le(s) point(s) d'intersection. (2 points d'intersection maximum.)
                    if (iseg != -1) {
                        if (i1 == -1) {
                            i1 = k;
                            pointI1 = s1.getOrig() + s1.getDir() * iseg;
                        } else if (i2 == -1) {
                            i2 = k;
                            pointI2 = s1.getOrig() + s1.getDir() * iseg;
                        }
                    }
                }
                physic::BoundingPolyhedron bp (a, b, c, true);
                //Si il y a 2 intersection alors le segment intersecte le triangle compl�tement.
                //Sinon l'extr�mit� du segment est dans le triangle.
                if (i1 != -1 && i2 != -1) {
                    //On d�coupe les triangles.
                    intersectsWallCompletely(pointI1, pointI2, i1, i2, n);
                } else if (i1 != -1 && i2 == -1) {
                    //On recherche quel extr�mit� du segment est dans le triangle.
                    math::Vec3f orig = s1.getOrig();
                    math::Vec3f ext = s1.getExt();
                    if (bp.isPointInside(orig)) {
                        pointI2 = orig;
                        intersectsWallCorner(pointI1, pointI2, n, s1, i1);
                    } else if (bp.isPointInside(ext)) {
                        pointI2 = ext;
                        intersectsWallCorner(pointI1, pointI2, n, s1, i1);
                    }
                    //On d�coupe les triangles.


                }
            }
            /*
            * Modifie ou ajoute des triangles suites aux intersections des triangles de lumi�res avec le segment du mur.
            * (Plusieurs cas sont � examiner.)
            * pointI1 = point d'intersection 1.
            * pointI2 = point d'intersection 2, si pas de deuxi�me intersection, il vaut l'extr�mit� du segement qui est
            * dans le triangles!
            * i1 et i2 sont les num�ros des c�t�s qui intersectent le segment. (-1 si pas d'intersections.)
            * numTris est le num�ro du triangle dans lequel � lieu l'/les intersection(s).
            */
            void PonctualLight::intersectsWallCompletely(math::Vec3f &pointI1, math::Vec3f &pointI2, int i1, int i2, int numTris) {
                /*
                * Cas 1 : le segment intersecte les deux c�t� du triangle. (mais pas le c�t� extr�mit�.)
                * on ne fait que de d�placer deux points extr�mes du triangle au points d'intersections.
                *         | /| |\ |
                *         |/ | | \|
                *         |  | |  |
                *        /|  | |  |\
                *       / |  | |  | \
                *       \ |  | |  | /
                *        \|  | |  |/
                *         |  | |  |
                *         |\ | | /|
                *         | \| |/ |
                */


                Color color = (*triangles[numTris])[0].color;
                Color color2 (color.r, color.g, color.b, 0);
                //std::cout<<pointI1<<" "<<pointI2<<std::endl;
                if (i1 == 0 && i2 == 2) {

                    (*triangles[numTris])[1].position = Vector3f(pointI1.x, pointI1.y, center.z);
                    int alpha = compAlphaFromPoint(pointI1);
                    color2.a = alpha;
                    (*triangles[numTris])[1].color = color2;

                    (*triangles[numTris])[2].position = Vector3f(pointI2.x, pointI2.y, center.z);
                    alpha = compAlphaFromPoint(pointI2);
                    color2.a = alpha;
                    (*triangles[numTris])[2].color = color2;


                /*
                *  Cas 2 : le segment intersecte le 1er c�t� du triangle et le c�t� extr�me.
                *  il faut d�placer le 1 er point au 2�me point d'intersection et rajout� un triangle.
                *
                *      \  /|     \|\
                *       \/ |      | \
                *       /\ |      |\ \
                *       \ \|      | \/
                *        \ |      | /\
                *         \|\     |/  \
                *
                *
                */
                } else if (i1 == 0 && i2 == 1) {

                    Vector3f a ((*triangles[numTris])[0].position.x, (*triangles[numTris])[0].position.y, center.z);
                    Vector3f b (pointI1.x, pointI1.y, center.z);
                    Vector3f c (pointI2.x, pointI2.y, center.z);
                    VertexArray *triangle = new VertexArray (Triangles);
                    triangle->append(Vertex(a, color));
                    int alpha = compAlphaFromPoint(pointI1);
                    color2.a = alpha;

                    triangle->append(Vertex(b, color2));
                    alpha = compAlphaFromPoint(pointI2);

                    color2.a = alpha;
                    triangle->append(Vertex(c, color2));
                    addTriangle(triangle);
                    (*triangles[numTris])[1].position = Vector3f(pointI2.x, pointI2.y, center.z);
                    (*triangles[numTris])[1].color = color2;

                    /*
                    *  Cas 3 : le segment intersecte le 3 �me c�t� du triangle et le c�t� extr�me.
                    *  il faut d�placer le points 2 � la 1 �re intersection et rajouter un triangle.
                    *
                    *         /| /    |\  /
                    *        / |/     | \/
                    *       /  |      | /\
                    *       \ /|      |/ /
                    *        \ |      | /
                    *       / \|     /|/
                    *
                    */

                } else if (i1 == 1 && i2 == 2) {
                    Vector3f a = Vector3f((*triangles[numTris])[0].position.x, (*triangles[numTris])[0].position.y, center.z);
                    Vector3f b (pointI1.x, pointI1.y, center.z);
                    Vector3f c (pointI2.x, pointI2.y, center.z);
                    VertexArray *triangle = new VertexArray (Triangles);
                    triangle->append(Vertex(a, color));
                    int alpha1 = compAlphaFromPoint(pointI1);

                    color2.a = alpha1;
                    triangle->append(Vertex(b, color2));
                    int alpha2 = compAlphaFromPoint(pointI2);

                    color2.a = alpha2;
                    triangle->append(Vertex(c, color2));
                    addTriangle(triangle);
                    (*triangles[numTris])[2].position = Vector3f(pointI1.x, pointI1.y, center.z);
                    color2.a = alpha1;
                    (*triangles[numTris])[2].color = color2;
                }
            }
            void PonctualLight::update() {
                for(unsigned int i = 0; i < triangles.size(); i++) {
                    triangles[i]->updateVBOBuffer();
                }
            }
            /*
            * Modifie ou ajoute des triangles suites aux intersections des triangles de lumi�res avec une extr�mit� du segment du mur.
            * (Plusieurs cas sont � examiner.)
            * pointI1 = point d'intersection 1.
            * pointI2 = point d'intersection 2, si pas de deuxi�me intersection, il vaut l'extr�mit� du segement qui est
            * dans le triangles!
            * i1 et i2 sont les num�ros des c�t�s qui intersectent le segment. (-1 si pas d'intersections.)
            * numTris est le num�ro du triangle dans lequel � lieu l'/les intersection(s).
            */
            void PonctualLight::intersectsWallCorner (math::Vec3f &pointI1, math::Vec3f &pointI2, int n, math::Ray &s1, int i) {
                /*
                *  Cas 4  : le segment intersecte 1 seul c�t� du triangle.
                *  il faut d�couper le triangle en 2.
                *
                *|\       /|      |\        /|
                *| \     / |      | \      / |
                *|  \   /  |      |  \    /  |
                *| \/   \/ |      |/ /    \ \|
                *| /\   /\ |      | /      \ |
                *|/  \ /  \|     /|/        \|\
                *
                *|\ /   \ /|     \|\        /|/
                *| \     / |      | \      / |
                *|/ \   / \|      |\ \    / /|
                *|  /   \  |      |  /    \  |
                *| /     \ |      | /      \ |
                *|/       \|      |/        \|
                *
                */
                //On prend le vecteur qu'il y a entre le 1er point du triangle et l'extr�mit� du segment.
                Color color = (*triangles[n])[0].color;
                Color color2 (color.r, color.g, color.b, 0);
                math::Vec3f a ((*triangles[n])[0].position.x, (*triangles[n])[0].position.y, center.z);
                //Vecteur entre le centre de la lumi�re et le coin du mur.
                math::Vec3f result = pointI2 - math::Vec3f(a.x, a.y, 0);
                //Si le coin du mur n'est pas au au centre de la lumi�re.
                if (!result.isNulVector()) {
                    //Si le c�t� de l'intersection n'est pas le c�t� extr�me du triangle.
                    if (i != 1) {
                        //Si det > 0 alors l'intersection est du c�t� du 2�me c�t� du triangle, sinon, l'intersection est de l'autre c�t�.
                        if (i == 0) {
                            int alphaP1 = (*triangles[n])[1].color.a;
                            int alphaP2 = (*triangles[n])[2].color.a;
                            int alphaPI1 = compAlphaFromPoint(pointI1);
                            int alphaPI2 = compAlphaFromPoint(pointI2);
                            VertexArray *quad = new VertexArray(Quads);
                            quad->append(Vertex((*triangles[n])[0].position, color));
                            color2.a = alphaPI2;
                            quad->append(Vertex(Vector3f(pointI2.x, pointI2.y,center.z), color2));
                            color2.a = alphaP1;
                            quad->append(Vertex((*triangles[n])[1].position, color2));
                            color2.a = alphaP2;
                            quad->append(Vertex((*triangles[n])[2].position, color2));
                            color2.a = alphaPI1;
                            (*triangles[n])[1].position = Vector3f(pointI1.x,pointI1.y, center.z);
                            (*triangles[n])[1].color = color2;
                            color2.a = alphaPI2;
                            (*triangles[n])[2].position = Vector3f(pointI2.x,pointI2.y, center.z);
                            (*triangles[n])[1].color = color2;
                            addTriangle(quad);
                        } else {
                            int alphaP1 = (*triangles[n])[1].color.a;
                            int alphaP2 = (*triangles[n])[2].color.a;
                            int alphaPI1 = compAlphaFromPoint(pointI1);
                            int alphaPI2 = compAlphaFromPoint(pointI2);
                            VertexArray *quad = new VertexArray(Quads);
                            quad->append(Vertex((*triangles[n])[0].position, color));
                            color2.a = alphaP1;
                            quad->append(Vertex((*triangles[n])[1].position, color2));
                            color2.a = alphaP2;
                            quad->append(Vertex((*triangles[n])[2].position, color2));
                            color2.a = alphaPI2;
                            quad->append(Vertex(Vector3f(pointI2.x, pointI2.y, center.z), color2));
                            color2.a = alphaPI2;
                            (*triangles[n])[1].position = Vector3f(pointI2.x,pointI2.y, center.z);
                            (*triangles[n])[1].color = color2;
                            color2.a = alphaPI1;
                            (*triangles[n])[2].position = Vector3f(pointI1.x,pointI1.y, center.z);
                            (*triangles[n])[1].color = color2;
                            addTriangle(quad);
                        }
                    } else {
                        /*Sinon, si le c�t� d'intersection est le c�t� extr�me du triangle, cela est un peu plus compliqu�.
                        *Il faut v�rifier quel point du c�t� extr�me du triangle est le plus proche de l'extr�mit� du segment.
                        *Ceci nous permet de savoir dans quel sens on doit d�couper le triangle.
                        */
                        float dist1 = math::Vec3f((*triangles[n])[1].position.x,(*triangles[n])[1].position.y, 0).computeDist(pointI2);
                        float dist2 = math::Vec3f((*triangles[n])[1].position.x,(*triangles[n])[1].position.y, 0).computeDist(pointI1);
                        if (dist1 < dist2) {
                            int alphaP1 = (*triangles[n])[1].color.a;
                            int alphaPI1 = compAlphaFromPoint(pointI1);
                            int alphaPI2 = compAlphaFromPoint(pointI2);
                            VertexArray *quad = new VertexArray(Quads);
                            quad->append(Vertex(Vector3f(a.x, a.y, center.z), color));
                            color2.a = alphaP1,
                            quad->append(Vertex((*triangles[n])[1].position, color2));
                            color2.a = alphaPI1;
                            quad->append(Vertex(Vector3f(pointI1.x, pointI1.y, center.z), color2));
                            color2.a = alphaPI2;
                            quad->append(Vertex(Vector3f(pointI2.x, pointI2.y, center.z), color2));
                            (*triangles[n])[1].position = Vector3f(pointI2.x, pointI2.y, center.z);
                            (*triangles[n])[1].color = color2;
                            addTriangle(quad);
                        } else {
                            int alphaP2 = (*triangles[n])[2].color.a;
                            int alphaPI1 = compAlphaFromPoint(pointI1);
                            int alphaPI2 = compAlphaFromPoint(pointI2);
                            VertexArray *quad = new VertexArray(Quads);
                            quad->append(Vertex(Vector3f(a.x, a.y, a.z), color));
                            color2.a = alphaPI2;
                            quad->append(Vertex(Vector3f(pointI2.x, pointI2.y, center.z), color2));
                            color2.a = alphaPI1;
                            quad->append(Vertex(Vector3f(pointI1.x, pointI1.y, center.z), color2));
                            color2.a = alphaP2;
                            quad->append(Vertex((*triangles[n])[2].position, color2));
                            color2.a = alphaPI2;
                            (*triangles[n])[2].position = Vector3f(pointI2.x, pointI2.y, center.z);
                            (*triangles[n])[2].color = color2;
                        }
                    }
                }
            }

            bool PonctualLight::operator== (Entity &other) {
                   if (other.getType() != "E_PONCTUAL_LIGHT")
                       return false;
                   PonctualLight &light = static_cast<PonctualLight&> (other);
                   return getPosition() == light.getPosition() &&
                       getLightCenter() == light.getLightCenter() &&
                       color == light.getColor() &&
                       intensity == light.getIntensity() &&
                       height == light.getHeight() &&
                       getSize() == light.getSize();

            }

            int PonctualLight::compAlphaFromPoint (math::Vec3f &point) {
                math::Vec3f v1 = math::Vec3f(point.x, point.y, 0) - math::Vec3f(getCenter().x, getCenter().y, 0);
                math::Vec3f v2 = v1.normalize();
                v2.x *= littleRadius * 0.5f;
                v2.y *= bigRadius * 0.5f;
                float m = v1.magnSquared() / v2.magnSquared();
                float f = 255.f * v2.magnitude();
                return f * m;
            }

            PonctualLight::~PonctualLight () {
                for (unsigned int i = 0; i < triangles.size(); i++) {
                    delete triangles[i];
                }
                triangles.clear();

            }
        }
    }
}

