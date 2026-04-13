#pragma once
#include "ofMain.h"

// Abstract interface for loading 3D models into ofVboMesh
class IModelLoader {
public:
    virtual ~IModelLoader() = default;
    virtual bool load(const std::string& path, ofVboMesh& mesh) = 0;
    virtual std::vector<std::string> getSupportedExtensions() const = 0;
};

// OBJ file loader
class ObjModelLoader : public IModelLoader {
public:
    bool load(const std::string& path, ofVboMesh& mesh) override;
    std::vector<std::string> getSupportedExtensions() const override;

    // Load OBJ split by group/object (`g`/`o`) into separate named meshes.
    // If the file has no group markers, returns a single unnamed group ("").
    static bool loadGroups(const std::string& path,
                           std::vector<std::pair<std::string, ofVboMesh>>& outGroups);
};

// PLY file loader (ofVboMesh natively supports PLY)
class PlyModelLoader : public IModelLoader {
public:
    bool load(const std::string& path, ofVboMesh& mesh) override;
    std::vector<std::string> getSupportedExtensions() const override;
};

// Factory that picks the right loader based on file extension
class ModelLoaderFactory {
public:
    ModelLoaderFactory();
    bool load(const std::string& path, ofVboMesh& mesh);
    std::vector<std::string> getAllSupportedExtensions() const;

private:
    std::vector<std::unique_ptr<IModelLoader>> loaders;
    IModelLoader* getLoaderForExtension(const std::string& ext);
};
