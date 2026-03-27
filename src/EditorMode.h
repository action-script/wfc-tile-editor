#pragma once
#include "ofMain.h"
#include "ofxGui.h"
#include "TileManager.h"

struct FaceHit {
    int tileIndex = -1;
    CubeFace face;
    glm::vec3 worldPoint;
};

class EditorMode {
public:
    void setup(TileManager* tm);
    void update(ofEasyCam& cam);
    void draw(ofEasyCam& cam, ofLight& keyLight, ofLight& fillLight, ofLight& backLight, ofMaterial& material);
    void mouseReleased(int x, int y, int button, ofEasyCam& cam);
    void keyPressed(int key);

private:
    TileManager* tileManager = nullptr;
    float cubeSize = 1.0;
    float spacing = 4.0;

    bool hasFirstSelection = false;
    int selectedTile = -1;
    CubeFace selectedFace;

    std::vector<std::string> symmetryTypes = {"X", "I", "\\", "L", "T", "+"};

    FaceHit hovered;  // face under mouse cursor (tileIndex=-1 if none)

    ofxPanel gui;
    ofxLabel infoLabel;

    void drawModel(int tileIndex, ofMaterial& material);
    void drawCubeOverlay(int tileIndex);
    void drawFaceEdges(const glm::vec3& center, CubeFace face, ofColor color, float lineWidth);
    void drawFaceQuad(const glm::vec3& center, CubeFace face, ofColor color, float alpha);
    void drawConnections();
    glm::vec3 faceCenterWorld(int tileIndex, CubeFace face);
    bool isFaceActive(CubeFace face) const;
    glm::vec3 faceNormal(CubeFace face);
    FaceHit pickFace(int mouseX, int mouseY, ofEasyCam& cam);
    bool rayPlaneIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                           const glm::vec3& planePoint, const glm::vec3& planeNormal,
                           float& t);
    void updateInfo();
};
