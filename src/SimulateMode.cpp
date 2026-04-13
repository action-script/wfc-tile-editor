#include "SimulateMode.h"

void SimulateMode::setup(TileManager* tm) {
    tileManager = tm;

    // Build mesh map
    simMeshMap.clear();
    for (auto& tile : tileManager->getTiles()) {
        if (tile.mesh.getNumVertices() > 0) {
            simMeshMap[tile.name] = &tile.mesh;
        }
    }

    // Auto-detect tile size from model dimensions
    float rawScale = tileManager->getRawUniformScale();

    if (params.size() == 0) {
        params.setName("Simulate");
        params.add(simWidth.set("Width", 8, 1, 20));
        params.add(simHeight.set("Height", 4, 1, 20));
        params.add(simLength.set("Length", 8, 1, 20));
        params.add(simTileSize.set("Tile Size", rawScale, 0.1f, rawScale * 4.0f));
        gui.setup(params);
        gui.setPosition(10, 10);
    } else {
        simTileSize.set(rawScale);
        simTileSize.setMax(rawScale * 4.0f);
    }
}

void SimulateMode::update() {
    int x = simWidth, y = simHeight, z = simLength;
    container.set(x, y, z);
    container.setPosition(0, y / 2.0f, 0);
}

void SimulateMode::draw(ofEasyCam& cam, ofLight& keyLight, ofLight& fillLight, ofLight& backLight, ofMaterial& material) {
    auto renderer = ofGetCurrentRenderer();

    cam.begin();
    ofEnableDepthTest();

    ofDrawGrid(1.0, 10, false, false, true, false);

    ofSetColor(50, 50, 200);
    container.drawWireframe();

    keyLight.enable();
    fillLight.enable();
    backLight.enable();

    for (auto& node : simNodes) {
        auto& tileName = node.first;
        node.second.transformGL();

        if (simDebugView) {
            ofSetColor(255);
            int ri = (int)(node.second.getOrientationEulerRad().y + 2.0f);
            if (ri == 0) ri = 3;
            else if (ri == 1) ri = 2;
            else if (ri == 2) ri = 0;
            else if (ri == 3) ri = 1;
            renderer->drawString(ofToString(ri), 0, 0.5, 0);

            material.setDiffuseColor(tileColor(tileName));
        } else {
            material.setDiffuseColor(ofColor(220));
        }

        material.begin();
        auto it = simMeshMap.find(tileName);
        if (it != simMeshMap.end()) {
            it->second->draw();
        }
        material.end();

        node.second.restoreTransformGL();
    }

    keyLight.disable();
    fillLight.disable();
    backLight.disable();

    ofDisableDepthTest();
    cam.end();

    // 2D overlay
    ofDisableLighting();
    gui.draw();

    ofDrawBitmapStringHighlight("FPS " + ofToString(ofGetFrameRate(), 0), ofGetWidth() - 80, 20);

    ofSetColor(180);
    int iy = ofGetHeight() - 80;
    ofDrawBitmapString("SPACE: generate  |  V: toggle debug view", 10, iy);
    ofDrawBitmapString("Debug view: " + std::string(simDebugView ? "ON" : "OFF"), 10, iy + 16);
    if (!simGenerated) {
        ofSetColor(255, 200, 50);
        ofDrawBitmapString("Press SPACE to generate a WFC structure", 10, iy + 40);
    }
}

void SimulateMode::generate() {
    tileManager->saveXml("data.xml");

    int x = simWidth, y = simHeight, z = simLength;
    float ts = simTileSize;
    wfc.SetUp("data.xml", "connected", x, y, z);

    int limit = 20;
    int seed = (int)ofRandom(1000);
    for (int k = 0; k < limit; k++) {
        bool result = wfc.Run(seed++);
        if (result) {
            simNodes = wfc.NodeTileOutput(worldNode, glm::vec3(ts, ts, ts), {"empty"});

            worldNode.setPosition(-x / 2.0f, 0, -z / 2.0f);
            worldNode.move(0.5f, 0.5f, 0.5f);
            worldNode.setScale(1.0f / ts);

            simGenerated = true;
            ofLogNotice("Simulate") << "WFC generated (" << simNodes.size() << " tiles, tileSize=" << ts << ")";
            return;
        }
    }

    ofLogWarning("Simulate") << "WFC failed after " << limit << " attempts";
    simGenerated = false;
}

void SimulateMode::keyPressed(int key) {
    if (key == ' ') {
        generate();
    } else if (key == 'v' || key == 'V') {
        simDebugView = !simDebugView;
    }
}

ofColor SimulateMode::tileColor(const std::string& name) const {
    std::hash<std::string> hasher;
    size_t h = hasher(name);
    float hue = (h % 360);
    float sat = 150 + (h / 360 % 106);
    float bri = 150 + (h / 38000 % 106);
    ofColor c;
    c.setHsb(hue * 255.0f / 360.0f, sat, bri);
    return c;
}
