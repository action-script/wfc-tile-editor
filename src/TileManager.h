#pragma once
#include "ofMain.h"
#include "ModelLoader.h"

enum CubeFace {
    FACE_LEFT   = 0,  // -X
    FACE_RIGHT  = 1,  // +X
    FACE_BOTTOM = 2,  // -Y
    FACE_TOP    = 3,  // +Y
    FACE_BACK   = 4,  // -Z
    FACE_FRONT  = 5   // +Z
};

struct Tile {
    std::string name;
    std::string symmetry = "L";  // X, I, \, L, T, +
    ofVboMesh mesh;
    glm::vec3 position;          // world position in gallery grid
};

struct Connection {
    int tileA;
    CubeFace faceA;
    int tileB;
    CubeFace faceB;

    bool operator==(const Connection& o) const {
        return (tileA == o.tileA && faceA == o.faceA && tileB == o.tileB && faceB == o.faceB)
            || (tileA == o.tileB && faceA == o.faceB && tileB == o.tileA && faceB == o.faceA);
    }
};

class TileManager {
public:
    void loadModelsFromDirectory(const std::string& directory, bool splitObjGroups = false);
    void layoutTiles(float spacing);

    bool addConnection(int tileA, CubeFace faceA, int tileB, CubeFace faceB);
    bool removeConnection(int tileA, CubeFace faceA, int tileB, CubeFace faceB);
    bool toggleConnection(int tileA, CubeFace faceA, int tileB, CubeFace faceB);

    void setSymmetry(int tileIndex, const std::string& sym);

    void loadXml(const std::string& path);
    void saveXml(const std::string& path);

    std::vector<Tile>& getTiles() { return tiles; }
    const std::vector<Tile>& getTiles() const { return tiles; }
    std::vector<Connection>& getConnections() { return connections; }
    const std::vector<Connection>& getConnections() const { return connections; }

    // Get connections involving a specific tile+face
    std::vector<const Connection*> getConnectionsForFace(int tileIndex, CubeFace face) const;

    // Face-to-rotation mapping for XML output
    // For horizontal neighbors:
    //   "left" tile:  LEFT=2, RIGHT=0, FRONT=1, BACK=3
    //   "right" tile: LEFT=0, BACK=1, RIGHT=2, FRONT=3
    static int faceToRotationLeft(CubeFace face);
    static int faceToRotationRight(CubeFace face);

    static bool isSideFace(CubeFace f) { return f != FACE_TOP && f != FACE_BOTTOM; }
    static bool isVerticalFace(CubeFace f) { return f == FACE_TOP || f == FACE_BOTTOM; }

    static std::string faceName(CubeFace f);
    static int rotationLabelForFace(CubeFace f);

    static CubeFace rotationToFaceLeft(int rot);
    static CubeFace rotationToFaceRight(int rot);

    int findTileByName(const std::string& name) const;

    // Returns names of tiles that appear in at least one connection
    std::vector<std::string> getTilesWithConnections() const;

    // Uniform scale — same across all tiles so they appear at consistent size
    float getUniformScale(float cubeSize) const {
        return uniformScale > 0 ? (cubeSize * 1.6f) / uniformScale : 1.0f;
    }

    // Raw max model dimension across all loaded tiles
    float getRawUniformScale() const { return uniformScale; }

private:
    float uniformScale = 1.0f;  // global max model dimension across all tiles
    std::vector<Tile> tiles;
    std::vector<Connection> connections;
    ModelLoaderFactory loaderFactory;
};
