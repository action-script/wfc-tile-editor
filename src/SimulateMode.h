#pragma once
#include "ofMain.h"
#include "ofxGui.h"
#include "ofxWFC3D.h"
#include "TileManager.h"

class SimulateMode {
public:
    void setup(TileManager* tm);
    void update();
    void draw(ofEasyCam& cam, ofLight& keyLight, ofLight& fillLight, ofLight& backLight, ofMaterial& material);
    void keyPressed(int key);

private:
    TileManager* tileManager = nullptr;

    ofxWFC3D wfc;
    ofNode worldNode;
    std::vector<std::pair<std::string, ofNode>> simNodes;
    std::unordered_map<std::string, ofVboMesh*> simMeshMap;
    bool simDebugView = true;
    bool simGenerated = false;

    ofxPanel gui;
    ofParameterGroup params;
    ofParameter<int> simWidth, simHeight, simLength;
    ofParameter<float> simTileSize;

    ofBoxPrimitive container;

    void generate();
    ofColor tileColor(const std::string& name) const;
};
