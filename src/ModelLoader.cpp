#include "ModelLoader.h"

// --- OBJ Loader ---

namespace {

void fillFlatNormalsIfMissing(ofVboMesh& mesh) {
    if (mesh.getNumNormals() != 0 || mesh.getNumVertices() == 0) return;
    auto& verts = mesh.getVertices();
    for (size_t i = 0; i + 2 < verts.size(); i += 3) {
        glm::vec3 n = glm::normalize(glm::cross(verts[i+1] - verts[i], verts[i+2] - verts[i]));
        mesh.addNormal(n);
        mesh.addNormal(n);
        mesh.addNormal(n);
    }
}

// Parse a single `f` line into the target mesh, resolving against the shared
// positions/normals/texcoords tables. OBJ indices are 1-based and may be
// negative (relative to current end).
void parseFaceLine(std::istringstream& iss, ofVboMesh& mesh,
                   const std::vector<glm::vec3>& positions,
                   const std::vector<glm::vec3>& normals,
                   const std::vector<glm::vec2>& texcoords) {
    struct FaceVert { int v = 0, vt = 0, vn = 0; };
    std::vector<FaceVert> faceVerts;

    std::string token;
    while (iss >> token) {
        FaceVert fv;
        std::replace(token.begin(), token.end(), '/', ' ');
        std::istringstream tiss(token);
        tiss >> fv.v;
        if (tiss.peek() != EOF) {
            tiss >> fv.vt;
            if (tiss.peek() != EOF) tiss >> fv.vn;
        }
        auto resolve = [](int idx, int count) {
            if (idx > 0) return idx - 1;
            if (idx < 0) return count + idx;
            return -1;
        };
        fv.v  = resolve(fv.v,  (int)positions.size());
        fv.vt = resolve(fv.vt, (int)texcoords.size());
        fv.vn = resolve(fv.vn, (int)normals.size());
        faceVerts.push_back(fv);
    }

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

} // namespace

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
            parseFaceLine(iss, mesh, positions, normals, texcoords);
        }
    }

    fillFlatNormalsIfMissing(mesh);
    return mesh.getNumVertices() > 0;
}

bool ObjModelLoader::loadGroups(const std::string& path,
                                std::vector<std::pair<std::string, ofVboMesh>>& outGroups) {
    outGroups.clear();
    ofFile file(path);
    if (!file.exists()) return false;

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoords;

    // Start with a single anonymous group; if an `o`/`g` appears before any
    // face we'll rename it; if faces land in it first, it stays anonymous.
    outGroups.emplace_back("", ofVboMesh{});
    outGroups.back().second.setMode(OF_PRIMITIVE_TRIANGLES);
    bool currentHasFaces = false;

    auto startGroup = [&](const std::string& name) {
        // Reuse the current group if it hasn't accumulated any faces yet.
        if (!currentHasFaces) {
            outGroups.back().first = name;
            return;
        }
        outGroups.emplace_back(name, ofVboMesh{});
        outGroups.back().second.setMode(OF_PRIMITIVE_TRIANGLES);
        currentHasFaces = false;
    };

    ofBuffer buffer = ofBufferFromFile(path);
    for (auto& rawLine : buffer.getLines()) {
        std::string line = ofTrimFront(rawLine);
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "v") {
            glm::vec3 v; iss >> v.x >> v.y >> v.z;
            positions.push_back(v);
        } else if (prefix == "vn") {
            glm::vec3 n; iss >> n.x >> n.y >> n.z;
            normals.push_back(n);
        } else if (prefix == "vt") {
            glm::vec2 t; iss >> t.x >> t.y;
            texcoords.push_back(t);
        } else if (prefix == "o" || prefix == "g") {
            std::string name;
            std::getline(iss, name);
            name = ofTrim(name);
            // Ignore the default "(null)" group some exporters emit.
            if (name == "(null)" || name == "default") name = "";
            if (!name.empty()) startGroup(name);
        } else if (prefix == "f") {
            parseFaceLine(iss, outGroups.back().second, positions, normals, texcoords);
            currentHasFaces = true;
        }
    }

    // Drop any trailing empty groups and flat-shade missing normals.
    outGroups.erase(std::remove_if(outGroups.begin(), outGroups.end(),
        [](const std::pair<std::string, ofVboMesh>& g) {
            return g.second.getNumVertices() == 0;
        }), outGroups.end());
    for (auto& g : outGroups) fillFlatNormalsIfMissing(g.second);

    return !outGroups.empty();
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
