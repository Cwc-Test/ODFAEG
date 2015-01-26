#include "../../../include/odfaeg/Graphics/map.h"
#include "../../../include/odfaeg/Graphics/rectangleShape.h"
#include "../../../include/odfaeg/Physics/boundingEllipsoid.h"
#include <iostream>
#include <climits>
#include "../../../include/odfaeg/Core/singleton.h"
#include "../../../include/odfaeg/Graphics/tGround.h"
namespace odfaeg {
    namespace graphic {
    using namespace std;

        Map::Map (FastRenderComponentManager* frcm, std::string name, int cellWidth, int cellHeight) : EntityManager(name), frcm(frcm) {
            gridMap = new GridMap(cellWidth, cellHeight);
            updateComponents = false;
            id = 0;
            version = 1;
            this->name = name;
            sf::Vector3i resolution;
            if (frcm != nullptr) {
                shadowMap = new RenderTexture();
                lightMap = new RenderTexture();
                stencilBuffer = new RenderTexture();
                normalMap = new RenderTexture();
                shadowMap->create(frcm->getWindow().getSize().x, frcm->getWindow().getSize().y,frcm->getWindow().getSettings());
                lightMap->create(frcm->getWindow().getSize().x, frcm->getWindow().getSize().y,frcm->getWindow().getSettings());
                stencilBuffer->create(frcm->getWindow().getSize().x, frcm->getWindow().getSize().y,frcm->getWindow().getSettings());
                normalMap->create(frcm->getWindow().getSize().x, frcm->getWindow().getSize().y,frcm->getWindow().getSettings());
                resolution = sf::Vector3i ((int) frcm->getWindow().getSize().x, (int) frcm->getWindow().getSize().y, frcm->getWindow().getView().getSize().z);
                perPixLightingShader = new Shader();
                buildShadowMapShader = new Shader();
                perPixShadowShader = new Shader();
                buildNormalMapShader = new Shader();
                if (Shader::getShadingLanguageVersionMajor() >= 3 && Shader::getShadingLanguageVersionMinor() >= 3) {
                    std::cout<<"version 330"<<std::endl;
                    const std::string perPixLightingVertexShader =
                    "#version 330 core \n"
                    "layout (location = 0) in vec3 vertex_position;"
                    "layout (location = 1) in vec4 vertex_color;"
                    "layout (location = 2) in vec2 vertex_texCoords0;"
                    "layout (location = 3) in vec3 vertex_normal;"
                    "uniform mat4 p;"
                    "uniform mat4 mv;"
                    "out mat4 projMat;"
                    "out vec2 texCoords;"
                    "out vec4 color;"
                    "void main () {"
                            "gl_Position = mv * p * vec4(vertex_position.xyz, 1);"
                            "texCoords = vertex_texCoords0;"
                            "color = vertex_color;"
                            "projMat = p;"
                    "}";
                    const std::string perPixLightingFragmentShader =
                    "#version 330 core \n"
                    "layout(origin_upper_left, pixel_center_integer) in vec4 gl_FragCoord;"
                    "uniform sampler2D normalMap;"
                    "uniform vec3 resolution;"
                    "uniform vec4 lightColor;"
                    "uniform sampler2D lightMap;"
                    "uniform vec4 lightPos;"
                    "in mat4 projMat;"
                    "in vec2 texCoords;"
                    "in vec4 color;"
                    "out vec4 fragColor;"
                    "void main () { "
                        "vec2 position = (gl_FragCoord.xy / resolution.xy);"
                        "vec4 bump = texture2D(normalMap, position);"
                        "vec4 ndcLightPos = projMat * vec4(0, 0, lightPos.z, 0);"
                        "vec3 pixPos = vec3 (gl_FragCoord.x, cBump.z, resolution.y - gl_FragCoord.y);"
                        "vec4 lightMapColor = texture2D(lightMap, position);"
                        "vec3 nLightPos = vec3 (lightPos.x, resolution.y - lightPos.y, ndcLightPos.z / ndcLightPos.w);"
                        "float radius = lightPos.w;"
                        "vec4 ndc = (gl_FragCoord.w != 1.f) ? projMat * vec4(0, 0, gl_FragCoord.z, 1) : vec4(0, 0, gl_FragCoord.z, 1);"
                        "if (ndc.z / ndc.w >= bump.w && distance(pixPos, nLightPos) <= radius) {"
                            "vec3 vertexToLight = nLightPos - pixPos;"
                            "float attenuation = 1.0f - (length(vertexToLight) / radius);"
                            "if (bump.x != 0 || bump.y != 0 || bump.z != 0 && vertexToLight.z > 0.f) {"
                                "vec3 dirToLight = normalize(vertexToLight.xyz);"
                                "float nDotl = dot (dirToLight, bump.xyz);"
                                "attenuation *= nDotl;"
                            "}"
                            "fragColor = lightColor * /*max(0.0f, attenuation)*/;"
                        "} else {"
                            "fragColor = lightMapColor;"
                        "}"
                    "}";
                    if (!perPixLightingShader->loadFromMemory(perPixLightingVertexShader, perPixLightingFragmentShader)) {
                        throw core::Erreur(53, "Error, failed to load per pixel lighting shader", 3);
                    }

                } else {
                   const std::string  buildNormalMapVertexShader =
                   "#version 130 \n"
                   "void main () {"
                        "gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;"
                        "gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;"
                        "gl_FrontColor = gl_Color;"
                   "}";
                    const std::string buildNormalMapFragmentShader =
                    "#version 130 \n"
                    "const vec2 size = vec2(2.0,0.0);"
                    "const ivec3 off = ivec3(-1,0,1);"
                    "uniform sampler2D texture;"
                    "void main() {"
                        "vec4 depth = texture2D(texture, gl_TexCoord[0].xy);"
                        "float s01 = textureOffset(texture, gl_TexCoord[0].xy, off.xy).z;"
                        "float s21 = textureOffset(texture, gl_TexCoord[0].xy, off.zy).z;"
                        "float s10 = textureOffset(texture, gl_TexCoord[0].xy, off.yx).z;"
                        "float s12 = textureOffset(texture, gl_TexCoord[0].xy, off.yz).z;"
                        "vec3 va = normalize (vec3(size.xy, s21 - s01));"
                        "vec3 vb = normalize (vec3(size.yx, s12 - s10));"
                        "gl_FragColor = vec4(cross(va, vb), depth.z);"
                    "}";
                    const std::string perPixLightingVertexShader =
                    "#version 130 \n"
                    "out mat4 projMat;"
                    "void main () {"
                            "gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;"
                            "gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;"
                            "gl_FrontColor = gl_Color;"
                            "projMat = gl_ProjectionMatrix;"
                    "}";
                     const std::string perPixLightingFragmentShader =
                    "#version 130 \n"
                    "uniform sampler2D normalMap;"
                    "uniform sampler2D lightMap;"
                    "uniform sampler2D heightMap;"
                    "uniform vec3 resolution;"
                    "uniform vec4 lightColor;"
                    "uniform vec4 lightPos;"
                    "uniform float maxM;"
                    "uniform float maxP;"
                    "in mat4 projMat;"
                    "void main () { "
                        "vec2 position = vec2 (gl_FragCoord.x / resolution.x, gl_FragCoord.y / resolution.y);"
                        "vec4 bump = texture2D(normalMap, position);"
                        "vec4 specularInfos = texture2D(heightMap, position);"
                        "vec4 ndcLightPos = projMat * vec4(0, 0, lightPos.z, 1);"
                        "vec3 sLightPos = vec3 (lightPos.x, lightPos.y, -lightPos.z);"
                        "float radius = lightPos.w;"
                        "vec3 pixPos = vec3 (gl_FragCoord.x, gl_FragCoord.y, bump.w);"
                        "vec4 lightMapColor = texture2D(lightMap, position);"
                        "vec3 viewPos = vec3(resolution.x * 0.5f, resolution.y * 0.5f, 1);"
                        "float z = (gl_FragCoord.w != 1.f) ? (inverse(projMat) * vec4(0, 0, 0, gl_FragCoord.w)).w : gl_FragCoord.z;"
                        "if (z >= bump.w) {"
                            "vec4 specularColor = vec4(0, 0, 0, 0);"
                            "vec3 vertexToLight = sLightPos - pixPos;"
                            "float attenuation = 1.f - length(vertexToLight) / radius;"
                            "vec3 pixToView = pixPos - viewPos;"
                            "float normalLength = dot(bump.xyz, vertexToLight);"
                            "vec3 lightReflect = vertexToLight + 2 * (bump.xyz * normalLength - vertexToLight);"
                            "float m = specularInfos.r * maxM;"
                            "float p = specularInfos.g * maxP;"
                            "float specularFactor = dot(pixToView, lightReflect);"
                            "specularFactor = pow (specularFactor, p);"
                            "if (specularFactor > 0) {"
                            "specularColor = vec4(lightColor.rgb, 1) * m * specularFactor;"
                            "}"
                            "if (bump.x != 0 || bump.y != 0 || bump.z != 0 && vertexToLight.z > 0.f) {"
                                "vec3 dirToLight = normalize(vertexToLight.xyz);"
                                "float nDotl = dot (dirToLight, bump.xyz);"
                                "attenuation *= nDotl;"
                            "}"
                            "gl_FragColor = vec4(lightColor.xyz, 1) * max(0.0f, attenuation) + specularColor;"
                        "} else {"
                            "gl_FragColor = lightMapColor;"
                        "}"
                    "}";
                    const std::string buildShadowMapVertexShader =
                    "#version 130 \n"
                    "uniform mat4 shadowProjMat;"
                    "out mat4 projMat;"
                    "void main () {"
                        "gl_Position = gl_ModelViewProjectionMatrix * shadowProjMat * gl_Vertex;"
                        "gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;"
                        "gl_FrontColor = gl_Color;"
                        "projMat = gl_ProjectionMatrix;"
                    "}";
                    const std::string buildShadowMapFragmentShader =
                    "#version 130 \n"
                    "uniform sampler2D texture;"
                    "uniform float haveTexture;"
                    "in mat4 projMat;"
                    "void main() {"
                    "   vec4 color = (haveTexture == 1) ? texture2D(texture, gl_TexCoord[0].xy) * gl_Color : gl_Color;"
                    "   float z = (gl_FragCoord.w != 1.f) ? (inverse(projMat) * vec4(0, 0, 0, gl_FragCoord.w)).w : gl_FragCoord.z;"
                    "   gl_FragColor = vec4(0, 0, z, color.a);"
                    "}";
                    const std::string perPixShadowVertexShader =
                    "#version 130 \n"
                    "uniform mat4 depthBiasMatrix;"
                    "uniform mat4 shadowProjMat;"
                    "out vec4 shadowCoords;"
                    "out mat4 projMat;"
                    "void main () {"
                        "gl_Position = gl_ModelViewProjectionMatrix * shadowProjMat * gl_Vertex;"
                        "gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;"
                        "gl_FrontColor = gl_Color;"
                        "projMat = gl_ProjectionMatrix;"
                        "shadowCoords = depthBiasMatrix * vec4(gl_Position.xyz, 1);"
                    "}";
                    const std::string perPixShadowFragmentShader =
                    "#version 130 \n"
                    "uniform sampler2D texture;"
                    "uniform sampler2D stencilBuffer;"
                    "uniform float firstPass;"
                    "uniform float set;"
                    "uniform float haveTexture;"
                    "in vec4 shadowCoords;"
                    "in mat4 projMat;"
                    "void main() {"
                    "   vec4 color = (haveTexture == 1) ? texture2D(texture, gl_TexCoord[0].xy) * gl_Color : gl_Color;"
                    "   vec4 stencil = texture2D (stencilBuffer, shadowCoords.xy);"
                    "   float z = (gl_FragCoord.w != 1.f) ? (inverse(projMat) * vec4(0, 0, 0, gl_FragCoord.w)).w : gl_FragCoord.z;"
                    "   if (firstPass == 1) {"
                    "       if (stencil.z < z && set == 1) {"
                    "           gl_FragColor = vec4(0, 0, 0, color.a);"
                    "       } else {"
                    "           gl_FragColor = vec4(1, 1, 1, 1);"
                    "       }"
                    "    } else {"
                    "       if (set == 1) {"
                    "           gl_FragColor = vec4(1, 1, 1, color.a);"
                    "       } else if (stencil.z == 1.f) {"
                    "           gl_FragColor = vec4(0, 0, 0, color.a);"
                    "       } else {"
                    "           gl_FragColor = vec4(1, 1, 1, color.a);"
                    "       }"
                    "    }"
                    "}";
                    if (!buildNormalMapShader->loadFromMemory(buildNormalMapVertexShader, buildNormalMapFragmentShader)) {
                        throw core::Erreur(52, "Error, failed to load build normal map shader", 3);
                    }
                    if (!buildShadowMapShader->loadFromMemory(buildShadowMapVertexShader, buildShadowMapFragmentShader)) {
                        throw core::Erreur(53, "Error, failed to load build shadow map shader", 3);
                    }
                    if (!perPixShadowShader->loadFromMemory(perPixShadowVertexShader, perPixShadowFragmentShader)) {
                        throw core::Erreur(54, "Error, failed to load per pix shadow map shader", 3);
                    }
                    if (!perPixLightingShader->loadFromMemory(perPixLightingVertexShader, perPixLightingFragmentShader)) {
                        throw core::Erreur(55, "Error, failed to load per pixel lighting shader", 3);
                    }
                }
                buildNormalMapShader->setParameter("texture", Shader::CurrentTexture);
                perPixLightingShader->setParameter("resolution", resolution.x, resolution.y, resolution.z);
                perPixLightingShader->setParameter("lightMap",lightMap->getTexture());
                perPixLightingShader->setParameter("maxM",Material::getMaxSpecularIntensity());
                perPixLightingShader->setParameter("maxP",Material::getMaxSpecularPower());
                buildShadowMapShader->setParameter("texture", Shader::CurrentTexture);
                perPixShadowShader->setParameter("stencilBuffer", stencilBuffer->getTexture());
                perPixShadowShader->setParameter("texture", Shader::CurrentTexture);
            }
        }
        void Map::generate_map(std::vector<Tile*> tGround, std::vector<Tile*> walls, math::Vec2f tileSize, physic::BoundingBox &rect, bool terrain3D) {

            int startX = rect.getPosition().x / tileSize.x * tileSize.x;
            int startY = rect.getPosition().y / tileSize.y * tileSize.y;
            int endX = (rect.getPosition().x + rect.getWidth()) / tileSize.x * tileSize.x;
            int endY = (rect.getPosition().y + rect.getHeight()) / tileSize.y * tileSize.y;
            BigTile *bt;
            if (!terrain3D)
                bt = new BigTile(math::Vec3f(startX, startY, startY + endY * 0.5f));
            else
                bt = new BigTile(math::Vec3f(startX, startY, rect.getPosition().z),tileSize,rect.getWidth() / tileSize.x);
            //Positions de d\E9part et d'arriv\E9es en fonction de la taille, de la position et de la taille des cellules de la map.
            for (int y = startY; y < endY;  y+=tileSize.y) {
                for (int x = startX; x < endX; x+=tileSize.x) {

                    //On projete les positions en fonction de la projection du jeux.
                    math::Vec3f projPos = gridMap->getBaseChangementMatrix().changeOfBase(math::Vec3f (x - startX, y - startY, 0));
                    math::Vec2f pos (projPos.x + startX, projPos.y + startY);
                    //Mur du coin en haut \E0 gauche.
                    if (x == startX && y == startY && walls.size() >= 4) {
                        g2d::Wall *w = new g2d::Wall(3, 80, new Tile(*walls[3]),&g2d::AmbientLight::getAmbientLight(), Shadow::SHADOW_TYPE::SHADOW_WALL);
                        w->setPosition(math::Vec3f(pos.x, pos.y, pos.y + walls[3]->getSize().y * 0.5f));
                        addEntity(w);
                        gridMap->getGridCellAt(math::Vec3f(w->getPosition().x, w->getPosition().y, 0))->setPassable(false);

                        //Mur du coin en haut \E0 droite.
                    } else if (x == endX - tileSize.x && y == startY && walls.size() >= 5) {
                        g2d::Wall *w = new g2d::Wall(4, 80, new Tile(*walls[4]),&g2d::AmbientLight::getAmbientLight(), Shadow::SHADOW_TYPE::SHADOW_WALL);
                        w->setPosition(math::Vec3f(pos.x, pos.y, pos.y + walls[4]->getSize().y * 0.5f));
                        addEntity(w);
                        gridMap->getGridCellAt(math::Vec3f(w->getPosition().x, w->getPosition().y, w->getPosition().z))->setPassable(false);
                        //Mur du coin en bas \E0 droite.
                    } else if (x == endX - tileSize.x && y == endY - tileSize.y && walls.size() >= 3) {
                        g2d::Wall *w = new g2d::Wall(2, 80, new Tile(*walls[2]),&g2d::AmbientLight::getAmbientLight(), Shadow::SHADOW_TYPE::SHADOW_WALL);
                        w->setPosition(math::Vec3f(pos.x, pos.y, pos.y + walls[2]->getSize().y * 0.5f));
                        addEntity(w);
                        gridMap->getGridCellAt(math::Vec3f(w->getPosition().x, w->getPosition().y, 0))->setPassable(false);
                    } else if (x == startX && y == endY - tileSize.y && walls.size() >= 6) {
                        g2d::Wall *w = new g2d::Wall(5, 80, new Tile(*walls[5]),&g2d::AmbientLight::getAmbientLight(), Shadow::SHADOW_TYPE::SHADOW_WALL);
                        w->setPosition(math::Vec3f(pos.x, pos.y, pos.y + walls[5]->getSize().y * 0.5f));
                        addEntity(w);
                        gridMap->getGridCellAt(math::Vec3f(w->getPosition().x, w->getPosition().y, 0))->setPassable(false);
                    } else if ((y == startY || y == endY - tileSize.y) && walls.size() >= 1) {
                        g2d::Wall *w = new g2d::Wall(0, 80, new Tile(*walls[0]),&g2d::AmbientLight::getAmbientLight(), Shadow::SHADOW_TYPE::SHADOW_WALL);
                        w->setPosition(math::Vec3f(pos.x, pos.y, pos.y + walls[0]->getSize().y * 0.5f));
                        addEntity(w);
                        if (y == endY - tileSize.y) {
                            int i = math::Math::random(tGround.size());
                            Tile *tile = new Tile(*tGround[i]);
                            tile->setPosition(math::Vec3f(pos.x, pos.y, pos.y + tile->getSize().y * 0.5f));
                            bt->addTile(tile);
                        }
                        gridMap->getGridCellAt(math::Vec3f(w->getPosition().x, w->getPosition().y, 0))->setPassable(false);
                    } else if ((x == startX || x == endX - tileSize.x) && walls.size() >= 2) {
                        g2d::Wall *w = new g2d::Wall(1, 80, new Tile(*walls[1]),&g2d::AmbientLight::getAmbientLight(), Shadow::SHADOW_TYPE::SHADOW_WALL);
                        w->setPosition(math::Vec3f(pos.x, pos.y, pos.y + walls[1]->getSize().y * 0.5f));
                        addEntity(w);
                        if (x == endX - tileSize.x) {
                            int i = math::Math::random(tGround.size());
                            Tile *tile = new Tile(*tGround[i]);
                            tile->setPosition(math::Vec3f(pos.x, pos.y, pos.y + tile->getSize().y * 0.5f));
                            bt->addTile(tile);
                        }
                        gridMap->getGridCellAt(math::Vec3f(w->getPosition().x, w->getPosition().y, 0))->setPassable(false);
                    } else {
                        int i = math::Math::random(tGround.size());
                        Tile *tile = new Tile(*tGround[i]);
                        tile->setPosition(math::Vec3f(pos.x, pos.y, pos.y + tile->getSize().y * 0.5f));
                        if (terrain3D) {
                            float heights[4];
                            for (unsigned int j = 0; j < sizeof(heights) / sizeof(float); j++) {
                                heights[j] = math::Math::random(rect.getPosition().z, rect.getPosition().z + rect.getDepth());
                            }
                            bt->addTile(tile, math::Vec2f(pos.x, pos.y), heights);
                        } else {
                            bt->addTile(tile);
                        }
                    }
                }
            }
            addEntity(bt);
        }
        vector<math::Vec2f> Map::getPath(Entity* entity, math::Vec2f finalPos) {
            return gridMap->getPath(entity, finalPos);
        }
        void Map::setName (string name) {
            this->name = name;
        }
        string Map::getName() {
            return name;
        }
        void Map::setId (int id) {
            this->id = id;
        }
        int Map::getId () {
            return id;
        }
        void Map::setVersion (int version) {
            this->version = version;
        }
        int Map::getVersion () {
            return version;
        }
        void Map::removeComptImg (const void* resource) {
            map<const void*, int>::iterator it;
            it = compImages.find(resource);
            if (it != compImages.end()) {
                compImages.erase(it);
            }
        }
        void Map::increaseComptImg(const void* resource) {
            map<const void*, int>::iterator it;
            it = compImages.find(resource);
            if (it != compImages.end()) {
                it->second = it->second + 1;
            } else {
                compImages.insert(pair<const void*, int> (resource, 1));
            }
        }
        void Map::decreaseComptImg (const void* resource) {
            map<const void*, int>::iterator it;
            it = compImages.find(resource);
            if (it != compImages.end() && it->second != 0) {
                it->second = it->second - 1;
            }
        }
        int Map::getCompImage(const void* resource) {
            map<const void*, int>::iterator it;
            it = compImages.find(resource);
            if (it != compImages.end())
                return it->second;
            return -1;

        }
        bool Map::addEntity(Entity *entity) {
            std::vector<Entity*> tiles;

            getChildren(entity, tiles, "*");
            if (tiles.size() != 0) {
                for (unsigned int i = 0; i < tiles.size(); i++) {
                   for (unsigned int j = 0; j < tiles[i]->getFaces().size(); j++) {
                        increaseComptImg(tiles[i]->getFaces()[j]->getMaterial().getTexture());
                   }
                }
            } else {
               for (unsigned int j = 0; j < entity->getFaces().size(); j++) {
                    increaseComptImg(entity->getFaces()[j]->getMaterial().getTexture());
               }
            }

            return gridMap->addEntity(entity);
        }

        bool Map::removeEntity (Entity *entity) {
            std::vector<Entity*> tiles;
            getChildren(entity, tiles, "*");
            for (unsigned int i = 0; i < tiles.size(); i++) {
               for (unsigned int j = 0; j < tiles[i]->getFaces().size(); j++) {
                   increaseComptImg(tiles[i]->getFaces()[j]->getMaterial().getTexture());
               }
            }
            return gridMap->deleteEntity(entity);
        }
        math::Vec3f Map::getPosition() {
            return gridMap->getMins();
        }
        int Map::getWidth() {
            return gridMap->getSize().x;
        }
        int Map::getHeight() {
            return gridMap->getSize().y;
        }
        int Map::getNbCasesPerRow () {
            return gridMap->getNbCasesPerRow();
        }
        bool Map::removeEntity(int id) {
            return gridMap->deleteEntity(id);
        }
        void Map::rotateEntity(Entity *entity, int angle) {
            gridMap->removeEntity(entity);
            entity->setRotation(angle);
            gridMap->replaceEntity(entity);
        }
        void Map::scaleEntity(Entity *entity, float sx, float sy) {
            gridMap->removeEntity(entity);
            entity->setScale(math::Vec3f(sx, sy, 0));
            gridMap->replaceEntity(entity);
        }
        void Map::moveEntity(Entity *entity, float dx, float dy, float dz) {
            entity->move(math::Vec3f(dx, dy, dz));
            gridMap->replaceEntity(entity);
        }
        void Map::checkVisibleEntities() {
            for (unsigned int c = 0; c < frcm->getNbComponents() + 1; c++) {
                physic::BoundingBox view ;
                if (c == frcm->getNbComponents())
                    view = frcm->getWindow().getView().getViewVolume();
                else
                    view = frcm->getRenderComponent(c)->getView().getViewVolume();
                visibleParentEntities.clear();
                vEntitiesByType.clear();
                int x = view.getPosition().x;
                int y = view.getPosition().y;
                int z = view.getPosition().z;
                int endX = view.getPosition().x + view.getWidth();
                int endY = view.getPosition().y + view.getHeight();
                int endZ = view.getDepth();
                physic::BoundingBox bx (x, y, z, endX-view.getPosition().x, endY-view.getPosition().y, endZ);

                for (int i = x; i <= endX; i+=gridMap->getOffsetX()) {
                    for (int j = y; j <= endY; j+=gridMap->getOffsetY()) {
                        //for (int k = 0; k <= endZ; k+=gridMap->getOffsetZ()) {
                            math::Vec3f point(i, j, 0);
                            CellMap* cell = getGridCellAt(point);
                            if (cell != nullptr) {
                                for (unsigned int n = 0; n < cell->getEntitiesInside().size(); n++) {
                                   Entity* entity = cell->getEntityInside(n);
                                   physic::BoundingBox bx2 = entity->getGlobalBounds();
                                   if (!containsVisibleParentEntity(entity)) {
                                        visibleParentEntities.push_back(entity);
                                        /*if (entity->isAnimated()) {
                                            if (!containsAnimatedVisibleEntity(static_cast<AnimatedEntity*>(entity))) {
                                                animatedVisibleEntities.push_back(static_cast<AnimatedEntity*>(entity));
                                            }
                                        } else {*/
                                            insertVisibleEntity(entity);
                                        //}
                                    }
                                }
                            //}
                        }
                    }
                }
                if (c < frcm->getNbComponents()) {
                    std::vector<Entity*> entities = getVisibleEntities(frcm->getRenderComponent(c)->getExpression());
                    frcm->getRenderComponent(c)->loadEntitiesOnComponent(entities);
                }
            }
            /*std::string s;
            std::cin>>s;*/

        }
        bool Map::containsVisibleParentEntity(Entity* entity) {
            for (unsigned int i = 0; i < visibleParentEntities.size(); i++) {
                if (visibleParentEntities[i] == entity)
                    return true;
            }
            return false;
        }
        void Map::insertVisibleEntity(Entity *entity) {
            if (entity->isAnimated()) {
                insertVisibleEntity(static_cast<AnimatedEntity*>(entity)->getCurrentEntity());
            }
            if (!entity->isAnimated()) {
                vector<Entity*> children;
                getChildren(entity, children, "*");
                VEntitiesByType it = vEntitiesByType.find(entity->getType());

                if (it == vEntitiesByType.end()) {
                    std::pair<std::string, std::vector<Entity*>> newEntitiesType(entity->getType(), std::vector<Entity*>());
                    vEntitiesByType.insert(newEntitiesType);
                    it = vEntitiesByType.find(entity->getType());
                }
                physic::BoundingBox view = frcm->getWindow().getView().getViewVolume();
                int x = view.getPosition().x;
                int y = view.getPosition().y;
                int z = view.getPosition().z;
                int endX = view.getWidth();
                int endY = view.getHeight();
                int endZ = view.getDepth();
                physic::BoundingBox bx (x, y, z, endX, endY, endZ);
                //std::cout<<"view volume : "<<view.getPosition()<<" "<<view.getWidth()<<" "<<view.getHeight()<<" "<<view.getDepth()<<std::endl;
                if (children.size() != 0) {

                    for (unsigned int i = 0; i < children.size(); i++) {
                        if (!containsVisibleEntity(children[i])) {
                            physic::BoundingBox bx2  = children[i]->getGlobalBounds();
                            //std::cout<<"view volume : "<<bx2.getPosition()<<" "<<bx2.getWidth()<<" "<<bx2.getHeight()<<" "<<bx2.getDepth()<<std::endl;
                            if (bx.intersects(bx2)) {
                                it->second.push_back(children[i]);
                            }
                        }
                    }

                } else {
                    if (!containsVisibleEntity(entity)) {

                        //physic::BoundingBox bx2 = entity->getGlobalBounds();
                        //if (bx.intersects(bx2)) {
                            //std::cout<<"add "<<std::endl;
                            it->second.push_back(entity);
                        //}
                    }
                }
            }
        }
        void Map::setBaseChangementMatrix (BaseChangementMatrix bm) {
            gridMap->setBaseChangementMatrix(bm);
        }
        void Map::insertAnimatedVisibleEntity (Entity *ae, std::vector<Entity*>& entities, View& view) {
            if (ae->isAnimated()) {
                insertAnimatedVisibleEntity(static_cast<AnimatedEntity*> (ae)->getCurrentEntity(), entities, view);
            }
            if (!ae->isAnimated()) {
                vector<Entity*> children;
                getChildren(ae, children, "*");

                if (children.size() == 0 && !containsVisibleEntity(ae)) {
                    entities.push_back(ae);
                } else {
                    for (unsigned int i = 0; i < children.size(); i++) {
                        if (!containsVisibleEntity(children[i])) {
                            entities.push_back(children[i]);
                        }
                    }
                }
            }
        }
        void Map::changeVisibleEntity(Entity* toRemove, Entity* toAdd) {
            for (unsigned int i = 0; i < frcm->getNbComponents(); i++) {
                frcm->getRenderComponent(i)->changeVisibleEntities(toRemove, toAdd, this);
            }
            View view = frcm->getWindow().getView();
            physic::BoundingBox bx (view.getViewVolume().getPosition().x,view.getViewVolume().getPosition().y,view.getViewVolume().getPosition().z,view.getViewVolume().getWidth(), view.getViewVolume().getHeight(), view.getViewVolume().getDepth());
            VEntitiesByType it = vEntitiesByType.find(toRemove->getType());
            std::vector<Entity*>::iterator it2;
            vector<Entity*> children;
            getChildren(toRemove, children, "*");
            if (children.size() != 0) {
                for (unsigned int i = 0; i < children.size(); i++) {
                    physic::BoundingBox bx2 = children[i]->getGlobalBounds();
                    if (bx.intersects(bx2)) {
                        for (it2 = it->second.begin(); it2 != it->second.end();) {
                            if (*it2 == children[i]) {
                                it2 = it->second.erase(it2);
                            } else {
                                it2++;
                            }
                        }
                    }
                }
            } else {
                physic::BoundingBox bx2 = toRemove->getGlobalBounds();
                if (bx.intersects(bx2)) {
                    for (it2 = it->second.begin(); it2 != it->second.end();) {
                        if (*it2 == toRemove) {
                            it2 = it->second.erase(it2);
                        } else {
                            it2++;
                        }
                    }
                }
            }
            getChildren(toAdd, children, "*");
            it = vEntitiesByType.find(toRemove->getType());
            if (children.size() == 0 && !containsVisibleEntity(toAdd)) {
                it->second.push_back(toAdd);
            } else {
                for (unsigned int i = 0; i < children.size(); i++) {
                    if (!containsVisibleEntity(children[i])) {
                        it->second.push_back(children[i]);
                    }
                }
            }
        }
        void Map::removeAnimatedVisibleEntity(Entity *toRemove, std::vector<Entity*>& entities, View& view, bool& removed) {
            if (toRemove->isAnimated()) {
                removeAnimatedVisibleEntity(static_cast<AnimatedEntity*>(toRemove)->getCurrentEntity(), entities, view, removed);
            }

            if (!toRemove->isAnimated()) {
                vector<Entity*>::iterator it2;
                vector<Entity*> children;
                getChildren(toRemove, children, "*");
                physic::BoundingBox bx (view.getViewVolume().getPosition().x,view.getViewVolume().getPosition().y,view.getViewVolume().getPosition().z,view.getViewVolume().getWidth(), view.getViewVolume().getHeight(), view.getViewVolume().getDepth());
                if (children.size() != 0) {
                    for (unsigned int i = 0; i < children.size(); i++) {
                        physic::BoundingBox bx2 = children[i]->getGlobalBounds();
                        if (bx.intersects(bx2)) {
                            for (it2 = entities.begin(); it2 != entities.end();) {
                                if (*it2 == children[i]) {
                                    removed = true;
                                    it2 = entities.erase(it2);
                                } else {
                                    it2++;
                                }
                            }
                        }
                    }
                } else {
                    physic::BoundingBox bx2 = toRemove->getGlobalBounds();
                    if (bx.intersects(bx2)) {
                        for (it2 = entities.begin(); it2 != entities.end();) {
                            if (*it2 == toRemove) {
                                removed = true;
                                it2 = entities.erase(it2);
                            } else {
                                it2++;
                            }
                        }
                    }
                }
            }
        }

        Entity* Map::getEntity(int id) {
            return gridMap->getEntity(id);
        }
        vector<CellMap*> Map::getCasesMap() {
            return gridMap->getCasesMap();
        }

        void Map::getChildren (Entity *entity, std::vector<Entity*>& entities, std::string type) {
            vector<Entity*> children = entity->getChildren();
            for (unsigned int i = 0; i < children.size(); i++) {
                if (children[i]->getChildren().size() != 0)
                    getChildren(children[i], children, type);
                if (type.at(0) == '*') {
                    std::string types;
                    if (type.find("-") != string::npos)
                        types = type.substr(2, type.size() - 3);
                    vector<string> excl = core::split(types, "-");
                    bool exclude = false;
                    for (unsigned int j = 0; j < excl.size(); j++) {
                        if (children[i]->getType() == excl[j])
                            exclude = true;
                    }
                    if (!exclude) {
                        entities.push_back(children[i]);
                    }
                } else {
                   vector<string> types = core::split(type, "+");
                   for (unsigned int j = 0; j < types.size(); j++) {
                        if (children[i]->getType() == types[j]) {
                            entities.push_back(children[i]);
                        }
                   }
                }
            }
        }

        vector<Entity*> Map::getEntities(string type) {
            vector<Entity*> entities;
            vector<Entity*> allEntities = gridMap->getEntities();
            if (type.at(0) == '*') {
                if (type.find("-") != string::npos)
                    type = type.substr(2, type.size() - 3);
                vector<string> excl = core::split(type, "-");
                for (unsigned int i = 0; i < allEntities.size(); i++) {
                    Entity* entity = allEntities[i];
                    bool exclude = false;
                    for (unsigned int j = 0; j < excl.size(); j++) {
                        if (entity->getType() == excl[i])
                            exclude = true;
                    }
                    if (!exclude) {
                        entity->updateTransform();
                        entities.push_back(entity);
                    }
                }
                return entities;
            }
            vector<string> types = core::split(type, "+");
            for (unsigned int i = 0; i < types.size(); i++) {
                for (unsigned int j = 0; j < allEntities.size(); j++) {
                    Entity* entity = allEntities[j];
                    if (entity->getType() == types[i]) {
                        entity->updateTransform();
                        entities.push_back(entity);
                    }

                }
            }
            return entities;
        }

        vector<Entity*> Map::getVisibleEntities (std::string type) {
            std::vector<Entity*> entities;
            if (type.at(0) == '*') {
                VEntitiesByType it;
                if (type.find("-") != string::npos)
                    type = type.substr(2, type.size() - 2);
                vector<string> excl = core::split(type, "-");
                for (it = vEntitiesByType.begin(); it != vEntitiesByType.end(); it++) {
                    bool exclude = false;
                    for (unsigned int i = 0; i < excl.size(); i++) {
                        if (it->first == excl[i])
                            exclude = true;
                    }
                    if (!exclude) {
                        for (unsigned int i = 0; i < it->second.size(); i++) {
                            entities.push_back(it->second[i]);
                        }
                    }
                }
                return entities;
            }
            vector<string> types = core::split(type, "+");
            for (unsigned int i = 0; i < types.size(); i++) {
                VEntitiesByType it = vEntitiesByType.find(types[i]);
                if (it != vEntitiesByType.end()) {
                    for (unsigned int n = 0; n < it->second.size(); n++) {
                        entities.push_back(it->second[n]);
                    }
                }
            }
            return entities;
        }

        vector<Entity*> Map::getEntitiesInBox (physic::BoundingBox bx, std::string type) {
             vector<Entity*> entities;
             vector<Entity*> allEntitiesInRect = gridMap->getEntitiesInBox(bx);

             if (type.at(0) == '*') {
                if (type.find("-") != string::npos)
                    type = type.substr(2, type.size() - 3);
                vector<string> excl = core::split(type, "-");
                for (unsigned int i = 0; i < allEntitiesInRect.size(); i++) {
                    Entity* entity = allEntitiesInRect[i];
                    bool exclude = false;
                    for (unsigned int i = 0; i < excl.size(); i++) {
                        if (entity->getType() == excl[i])
                            exclude = true;
                    }
                    if (!exclude) {
                        entities.push_back(entity);
                    }
                }
                return entities;
            }
            vector<string> types = core::split(type, "+");
            for (unsigned int i = 0; i < types.size(); i++) {
                 for (unsigned int j = 0; j < allEntitiesInRect.size(); j++) {
                    Entity* entity = allEntitiesInRect[j];
                    if (entity->getType() == types[i])
                        entities.push_back(entity);
                }
            }
            return entities;
        }

        math::Vec2f Map::getCoordinatesAt(math::Vec2f p) {
            return gridMap->getCoordinatesAt(p);
        }
        CellMap* Map::getGridCellAt(math::Vec2f p) {
            return gridMap->getGridCellAt(p);
        }
        vector<CellMap*> Map::getCasesInBox (physic::BoundingBox bx) {
            return gridMap->getCasesInBox(bx);
        }
        bool Map::containsVisibleEntity(Entity* entity) {
            VEntitiesByType it;
            it = vEntitiesByType.find(entity->getRootType());
            if (it == vEntitiesByType.end())
                return false;
            for (unsigned int i = 0; i < it->second.size(); i++) {
                if (it->second[i] == entity) {
                    return true;
                }
            }
            return false;
        }
        bool Map::collide (Entity *entity, math::Vec3f position) {
             return gridMap->collideWithEntity(entity, position);
        }
        bool Map::collide (Entity *entity) {
             return gridMap->collideWithEntity(entity);
        }
        void Map::computeIntersectionsWithWalls () {
             vector<Entity*> visibleLights = getEntities ("E_PONCTUAL_LIGHT");
             vector<Entity*> visibleDecors = getEntities ("E_DECOR+E_WALL");
             for (unsigned int i = 0; i < visibleLights.size(); i++) {
                static_cast<g2d::PonctualLight*>(visibleLights[i])->initTriangles();
             }
             for (unsigned int i = 0; i < visibleDecors.size(); i++) {
                 g2d::Decor *decor = static_cast<g2d::Decor*> (visibleDecors[i]);
                 for (unsigned int j = 0; j < visibleLights.size(); j++) {
                    g2d::PonctualLight *light = static_cast<g2d::PonctualLight*> (visibleLights[j]);

                    physic::BoundingSphere bc (light->getCenter(), light->getSize().x * 0.5f);

                    for (unsigned int s = 0; s < decor->getSegments().size(); s++) {
                        math::Ray ray = *decor->getSegments()[s];
                        if (bc.intersects(ray)) {
                            int size = light->getTris().size();
                            for (int n = 0; n < size; n++) {
                                light->checkInterTri(n, ray);

                            }
                        }
                        light->update();
                    }
                }
            }
            visibleLights.clear();
            visibleDecors.clear();
        }
        void Map::drawOnComponents(std::string expression, int layer, sf::BlendMode blendMode) {
            FastRenderComponent* frc = frcm->getRenderComponent(layer);
            frc->getFrameBufferTile().setCenter(frcm->getWindow().getView().getPosition());
            frc->setExpression(expression);
            frc->drawNextFrame();
        }
        void Map::drawOnComponents(Drawable& drawable, int layer, RenderStates states) {
            FastRenderComponent *frc = frcm->getRenderComponent(layer);
            frc->getFrameBufferTile().setCenter(frcm->getWindow().getView().getPosition());
            frc->draw(drawable, states);
        }

        void Map::generateStencilBuffer(std::string expression, int n, va_list args) {
            if (Shader::isAvailable()) {
                std::vector<Entity*> entities;
                View view = frcm->getWindow().getView();
                physic::BoundingBox viewArea = view.getViewVolume();
                math::Vec3f position (viewArea.getPosition().x,viewArea.getPosition().y, view.getPosition().z);
                math::Vec3f size (viewArea.getWidth(), viewArea.getHeight(), 0);
                stencilBuffer->setView(frcm->getWindow().getView());
                stencilBuffer->clear(sf::Color::Transparent);
                RenderStates states;
                states.shader = buildShadowMapShader;
                states.blendMode = sf::BlendAdd;
                if (n != -1) {
                    std::vector<unsigned int> idsCompInt;
                    for (int c = 0; c < n; c++) {
                        int j = va_arg(args, int);
                        if (j < n)
                            idsCompInt.push_back(j);
                    }
                    for (unsigned int i = 0; i < frcm->getNbComponents(); i++) {
                        entities = frcm->getRenderComponent(i)->getEntities();
                        view = frcm->getRenderComponent(i)->getView();
                        math::Vec3f centerLight = g2d::AmbientLight::getAmbientLight().getLightCenter();
                        view = View(view.getSize().x, view.getSize().y, -g2d::AmbientLight::getAmbientLight().getHeight(), g2d::AmbientLight::getAmbientLight().getHeight());
                        view.setCenter(centerLight);
                        view.rotate(0, 0, frcm->getRenderComponent(i)->getView().getTeta() + frcm->getRenderComponent(i)->getView().getAngles().z);
                        math::Vec3f forward = frcm->getRenderComponent(i)->getView().getPosition() - view.getPosition();
                        math::Vec3f target = view.getPosition() + forward;
                        view.lookAt(target.x, target.y, target.z);
                        stencilBuffer->setView(view);
                        bool find = false;
                        math::Vec3f v = view.getPosition() - frcm->getRenderComponent(i)->getView().getPosition();
                        math::Matrix4f biasMatrix(0.5f, 0.0f, 0.0f, 0.0f,
                                                  0.0f, 0.5f, 0.0f, 0.0f,
                                                  0.0f, 0.0f, 0.5f, 0.0f,
                                                  0.5f, 0.5f, 0.5f, 1.f);
                        math::Matrix4f depthBiasMatrix = biasMatrix * view.getViewMatrix().getMatrix() * view.getProjMatrix().getMatrix();
                        perPixShadowShader->setParameter("depthBiasMatrix", depthBiasMatrix.transpose());
                        for (unsigned int k = 0; k < entities.size(); k++) {
                            forward = entities[k]->getCenter() - view.getPosition();
                            target = view.getPosition() + forward;
                            view.lookAt(target.x, target.y, target.z);
                            if (entities[k]->getFaces().size() > 0) {
                                if (entities[k]->getFaces()[0]->getMaterial().getTexture() != nullptr) {
                                    buildShadowMapShader->setParameter("haveTexture", 1);
                                } else {
                                    buildShadowMapShader->setParameter("haveTexture", 0);
                                }
                                math::Vec3f shadowCenter, shadowScale(1.f, 1.f, 1.f), shadowRotationAxis;
                                float shadowRotationAngle = 0;
                                if (entities[k]->getParent() != nullptr && entities[k]->getParent()->isModel()) {
                                    shadowCenter = static_cast<Model*>(entities[k]->getParent())->getShadowCenter();
                                    shadowScale = static_cast<Model*>(entities[k]->getParent())->getShadowScale();
                                    shadowRotationAxis = static_cast<Model*>(entities[k]->getParent())->getShadowRotationAxis();
                                    shadowRotationAngle = static_cast<Model*>(entities[k]->getParent())->getShadowRotationAngle();
                                }
                                TransformMatrix tm;
                                tm.setOrigin(entities[k]->getPosition());
                                tm.setScale(shadowScale);
                                tm.setRotation(shadowRotationAxis, shadowRotationAngle);
                                tm.setTranslation(entities[k]->getPosition() + shadowCenter);
                                tm.update();
                                buildShadowMapShader->setParameter("shadowProjMat", tm.getMatrix().transpose());
                                stencilBuffer->draw(*entities[k], states);
                            }
                        }
                    }
                }
                va_end(args);
                stencilBuffer->display();
            }
        }

        Tile& Map::getShadowTile(std::string expression, int n, va_list args) {
            std::vector<Entity*> entities;
            View view = frcm->getWindow().getView();
            physic::BoundingBox viewArea = view.getViewVolume();
            math::Vec3f position (viewArea.getPosition().x,viewArea.getPosition().y, view.getPosition().z);
            math::Vec3f size (viewArea.getWidth(), viewArea.getHeight(), 0);
            shadowMap->clear(sf::Color::White);
            shadowMap->setView(frcm->getWindow().getView());
            if (Shader::isAvailable()) {
                RenderStates states;
                states.shader = perPixShadowShader;
                if (n != -1) {
                    std::vector<unsigned int> idsCompInt;
                    for (int c = 0; c < n; c++) {
                        int j = va_arg(args, int);
                        if (j < n)
                            idsCompInt.push_back(j);
                    }
                    for (unsigned int i = 0; i < frcm->getNbComponents(); i++) {
                        entities = frcm->getRenderComponent(i)->getEntities();
                        view = frcm->getRenderComponent(i)->getView();
                        shadowMap->setView(view);
                        bool find = false;
                        for (unsigned int k = 0; k < idsCompInt.size(); k++)
                            if (idsCompInt[k] == i)
                                find = true;
                        if (find) {
                            //std::cout<<"don't set"<<std::endl;
                            perPixShadowShader->setParameter("set", 1);
                        } else {
                            //std::cout<<"set"<<std::endl;
                            perPixShadowShader->setParameter("set", 0);
                        }
                        for (unsigned int p = 0; p < 2; p++) {
                            for (unsigned int k = 0; k < entities.size(); k++) {
                                if (entities[k]->getFaces().size() > 0) {
                                    if (entities[k]->getFaces()[0]->getMaterial().getTexture() != nullptr) {
                                        perPixShadowShader->setParameter("haveTexture", 1);
                                    } else {
                                        perPixShadowShader->setParameter("haveTexture", 0);
                                    }
                                    math::Vec3f shadowCenter, shadowScale(1.f, 1.f, 1.f), shadowRotationAxis;
                                    float shadowRotationAngle = 0;
                                    if (entities[k]->getParent() != nullptr && entities[k]->getParent()->isModel()) {
                                        shadowCenter = static_cast<Model*>(entities[k]->getParent())->getShadowCenter();
                                        shadowScale = static_cast<Model*>(entities[k]->getParent())->getShadowScale();
                                        shadowRotationAxis = static_cast<Model*>(entities[k]->getParent())->getShadowRotationAxis();
                                        shadowRotationAngle = static_cast<Model*>(entities[k]->getParent())->getShadowRotationAngle();
                                    }
                                    if (p == 0) {
                                        TransformMatrix tm;
                                        tm.setOrigin(entities[k]->getPosition());
                                        tm.setScale(shadowScale);
                                        tm.setRotation(shadowRotationAxis, shadowRotationAngle);
                                        tm.setTranslation(entities[k]->getPosition() + shadowCenter);
                                        tm.update();
                                        perPixShadowShader->setParameter("shadowProjMat", tm.getMatrix().transpose());
                                        perPixShadowShader->setParameter("firstPass", 1);
                                        shadowMap->draw(*entities[k], states);
                                    } else {
                                        TransformMatrix tm;
                                        //tm.update();
                                        perPixShadowShader->setParameter("shadowProjMat", tm.getMatrix().transpose());
                                        perPixShadowShader->setParameter("firstPass", 0);
                                        shadowMap->draw(*entities[k], states);
                                    }
                                }
                            }
                        }
                    }
                }
                RectangleShape rect(size * 2.f);
                rect.setPosition(position - size * 0.5f);
                rect.setFillColor(sf::Color(100, 100, 100, 128));
                shadowMap->draw(rect);
                shadowMap->display();
            }
            va_end(args);
            shadowTile = new Tile (&shadowMap->getTexture(),math::Vec3f(position.x, position.y, position.z), math::Vec3f(size.x, size.y, 0),sf::IntRect(0, 0, size.x, size.y));
            return *shadowTile;
        }
        Tile& Map::getLightTile (std::string expression, int n, va_list args) {

            std::vector<Entity*> lights = getVisibleEntities(expression);
            View view = frcm->getWindow().getView();
            physic::BoundingBox viewArea = view.getViewVolume();
            math::Vec3f position (viewArea.getPosition().x,viewArea.getPosition().y, view.getPosition().z);
            math::Vec3f size (viewArea.getWidth(), viewArea.getHeight(), 0);
            sf::Color ambientColor = g2d::AmbientLight::getAmbientLight().getColor();
            lightMap->clear(sf::Color::Black);
            lightMap->setView(frcm->getWindow().getView());
            RectangleShape rect(size * 2.f);
            rect.setPosition(position - size * 0.5f);
            rect.setFillColor(ambientColor);
            lightMap->draw(rect);
            lightMap->display();
            if (Shader::isAvailable()) {
                RenderStates states;
                if (n != -1) {
                       std::vector<unsigned int> idsCompInt;
                       for (int c = 0; c < n; c++) {
                           int j = va_arg(args, int);
                           if (j < n)
                               idsCompInt.push_back(j);
                       }
                       for (unsigned int i = 0; i < frcm->getNbComponents(); i++) {
                            bool find = false;
                            for (unsigned int k = 0; k < idsCompInt.size(); k++)
                                if (idsCompInt[k] == i)
                                    find = true;
                            if (find) {
                                lightMap->setView(frcm->getRenderComponent(i)->getView());
                                normalMap->setView(frcm->getRenderComponent(i)->getView());
                                normalMap->clear(sf::Color::Transparent);
                                Tile& heightMapTile = frcm->getRenderComponent(i)->getDepthBufferTile();
                                heightMapTile.setCenter(frcm->getRenderComponent(i)->getView().getPosition());
                                states.shader = buildNormalMapShader;
                                states.blendMode = sf::BlendNone;
                                normalMap->draw(heightMapTile, states);
                                normalMap->display();
                                perPixLightingShader->setParameter("heightMap", frcm->getRenderComponent(i)->getDepthBufferTexture());
                                perPixLightingShader->setParameter("normalMap", normalMap->getTexture());
                                states.shader = perPixLightingShader;
                                states.blendMode = sf::BlendAdd;
                                for (unsigned int j = 0; j < lights.size(); j++) {
                                    EntityLight* el = static_cast<EntityLight*> (lights[j]);
                                    math::Vec3f center = frcm->getWindow().mapCoordsToPixel(el->getLightCenter(), frcm->getRenderComponent(i)->getView());
                                    center.w = el->getSize().x * 0.5f;
                                    perPixLightingShader->setParameter("lightPos", center.x, center.y, center.z, center.w);
                                    perPixLightingShader->setParameter("lightColor", el->getColor().r, el->getColor().g,el->getColor().b,el->getColor().a);
                                    lightMap->draw(*el, states);
                                }
                            }
                       }
                       lightMap->display();
                }
            }
            va_end(args);
            lightTile = new Tile (&lightMap->getTexture(), math::Vec3f(position.x, position.y, position.z), math::Vec3f(size.x, size.y, 0),sf::IntRect(0, 0, size.x, size.y));
            return *lightTile;
        }

        BaseChangementMatrix Map::getBaseChangementMatrix() {
            return gridMap->getBaseChangementMatrix();
        }
        Map::~Map () {
            delete gridMap;
            delete lightTile;
            delete shadowTile;
            delete shadowMap;
            delete lightMap;
            delete buildShadowMapShader;
            delete stencilBuffer;
            delete stencilBufferTile;
            delete perPixLightingShader;
            delete perPixShadowShader;
        }
    }
}


