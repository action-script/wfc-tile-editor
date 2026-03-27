#pragma once
#include "ofMain.h"
#include "TileManager.h"
#include "EditorMode.h"
#include "SimulateMode.h"

enum AppMode {
    MODE_EDITOR,
    MODE_SIMULATE
};

class ofApp : public ofBaseApp {
public:
    void setup() override;
    void update() override;
    void draw() override;
    void keyPressed(int key) override;
    void mousePressed(int x, int y, int button) override;
    void mouseReleased(int x, int y, int button) override;
    void mouseDragged(int x, int y, int button) override;

private:
    AppMode mode = MODE_EDITOR;
    bool mouseDragging = false;

    ofEasyCam cam;
    ofLight keyLight, fillLight, backLight;
    ofMaterial material;
    TileManager tileManager;

    EditorMode editor;
    SimulateMode simulator;

    void setupLights();
};
