#include "ModelLoader.h"

// --- OBJ Loader ---

bool ObjModelLoader::load(const std::string& path, ofVboMesh& mesh) {
    ofFile file(path);
    if (!file.exists()) return false;

    mesh.clear();
    mesh.setMode(OF_PRIMITIVE_TRIANGLES);

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoords;

    ofBuffer buffer = ofBufferFromFile(path);
    for (auto& rawLine : buffer.getLines()) {
        std::string line = ofTrimFront(rawLine);
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "v") {
            glm::vec3 v;
            iss >> v.x >> v.y >> v.z;
            positions.push_back(v);
        } else if (prefix == "vn") {
            glm::vec3 n;
            iss >> n.x >> n.y >> n.z;
            normals.push_back(n);
        } else if (prefix == "vt") {
            glm::vec2 t;
            iss >> t.x >> t.y;
            texcoords.push_back(t);
        } else if (prefix == "f") {
            // Parse face - supports v, v/vt, v/vt/vn, v//vn
            struct FaceVert { int v = -1, vt = -1, vn = -1; };
            std::vector<FaceVert> faceVerts;

            std::string token;
            while (iss >> token) {
                FaceVert fv;
                std::replace(token.begin(), token.end(), '/', ' ');
                std::istringstream tiss(token);
                tiss >> fv.v;
                if (tiss.peek() != EOF) {
                    // Check if next is empty (v//vn case handled by space replacement)
                    tiss >> fv.vt;
                    if (tiss.peek() != EOF) tiss >> fv.vn;
                }
                // OBJ indices are 1-based
                fv.v--;
                if (fv.vt > 0) fv.vt--;
                if (fv.vn > 0) fv.vn--;
                faceVerts.push_back(fv);
            }

            // Triangulate fan
            for (size_t i = 1; i + 1 < faceVerts.size(); i++) {
                for (int idx : {0, (int)i, (int)(i + 1)}) {
                    auto& fv = faceVerts[idx];
                    if (fv.v >= 0 && fv.v < (int)positions.size())
                        mesh.addVertex(positions[fv.v]);
                    if (fv.vn >= 0 && fv.vn < (int)normals.size())
                        mesh.addNormal(normals[fv.vn]);
                    if (fv.vt >= 0 && fv.vt < (int)texcoords.size())
                        mesh.addTexCoord(texcoords[fv.vt]);
                }
            }
        }
    }

    // Generate normals if none were loaded
    if (mesh.getNumNormals() == 0 && mesh.getNumVertices() > 0) {
        auto& verts = mesh.getVertices();
        for (size_t i = 0; i + 2 < verts.size(); i += 3) {
            glm::vec3 n = glm::normalize(glm::cross(verts[i+1] - verts[i], verts[i+2] - verts[i]));
            mesh.addNormal(n);
            mesh.addNormal(n);
            mesh.addNormal(n);
        }
    }

    return mesh.getNumVertices() > 0;
}

std::vector<std::string> ObjModelLoader::getSupportedExtensions() const {
    return {"obj"};
}

// --- PLY Loader ---

bool PlyModelLoader::load(const std::string& path, ofVboMesh& mesh) {
    mesh.clear();
    mesh.load(path);
    return mesh.getNumVertices() > 0;
}

std::vector<std::string> PlyModelLoader::getSupportedExtensions() const {
    return {"ply"};
}

// --- Factory ---

ModelLoaderFactory::ModelLoaderFactory() {
    loaders.push_back(std::make_unique<ObjModelLoader>());
    loaders.push_back(std::make_unique<PlyModelLoader>());
}

bool ModelLoaderFactory::load(const std::string& path, ofVboMesh& mesh) {
    std::string ext = ofFilePath::getFileExt(path);
    ofStringReplace(ext, ".", "");
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    auto* loader = getLoaderForExtension(ext);
    if (!loader) {
        ofLogError("ModelLoaderFactory") << "No loader for extension: " << ext;
        return false;
    }
    return loader->load(path, mesh);
}

std::vector<std::string> ModelLoaderFactory::getAllSupportedExtensions() const {
    std::vector<std::string> all;
    for (auto& loader : loaders) {
        auto exts = loader->getSupportedExtensions();
        all.insert(all.end(), exts.begin(), exts.end());
    }
    return all;
}

IModelLoader* ModelLoaderFactory::getLoaderForExtension(const std::string& ext) {
    for (auto& loader : loaders) {
        for (auto& e : loader->getSupportedExtensions()) {
            if (e == ext) return loader.get();
        }
    }
    return nullptr;
}
