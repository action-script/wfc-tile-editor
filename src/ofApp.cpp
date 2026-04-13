#include "ofApp.h"

void ofApp::setup() {
    ofSetWindowTitle("WFC Tile Editor");
    ofBackground(40);
    ofEnableSmoothing();
    ofSetEscapeQuitsApp(false);

    cam.setDistance(15);
    cam.setNearClip(0.1);

    setupLights();

    material.setDiffuseColor(ofColor(210));
    material.setSpecularColor(ofColor(50));
    material.setAmbientColor(ofColor(40));

    tileManager.loadModelsFromDirectory("", splitObjGroups);
    tileManager.loadXml("data.xml");
    tileManager.layoutTiles(4.0);

    editor.setup(&tileManager);
    simulator.setup(&tileManager);
}

void ofApp::setupLights() {
    keyLight.setDirectional();
    keyLight.setDiffuseColor(ofColor(220, 215, 210));
    keyLight.setSpecularColor(ofColor(80));
    keyLight.setPosition(8, 12, 10);
    keyLight.lookAt(glm::vec3(0));

    fillLight.setDirectional();
    fillLight.setDiffuseColor(ofColor(100, 110, 130));
    fillLight.setSpecularColor(ofColor(20));
    fillLight.setPosition(-10, 4, 5);
    fillLight.lookAt(glm::vec3(0));

    backLight.setDirectional();
    backLight.setDiffuseColor(ofColor(60, 60, 80));
    backLight.setSpecularColor(ofColor(10));
    backLight.setPosition(0, -3, -10);
    backLight.lookAt(glm::vec3(0));
}

void ofApp::update() {
    if (mode == MODE_SIMULATE) {
        simulator.update();
    } else {
        editor.update(cam);
    }
}

void ofApp::draw() {
    if (mode == MODE_EDITOR) {
        editor.draw(cam, keyLight, fillLight, backLight, material);
    } else {
        simulator.draw(cam, keyLight, fillLight, backLight, material);
    }

    // Mode tab indicator
    ofSetColor(255);
    std::string modeStr = (mode == MODE_EDITOR) ? "[EDITOR]  Simulate" : "Editor  [SIMULATE]";
    ofDrawBitmapStringHighlight(modeStr, ofGetWidth() / 2 - 80, 20,
        ofColor(0, 0, 0, 180), ofColor(255));
    ofDrawBitmapString("TAB: switch mode", ofGetWidth() / 2 - 60, 38);

}

void ofApp::mousePressed(int x, int y, int button) {
    mouseDragging = false;
}

void ofApp::mouseDragged(int x, int y, int button) {
    mouseDragging = true;
}

void ofApp::mouseReleased(int x, int y, int button) {
    if (mouseDragging) {
        mouseDragging = false;
        return;
    }

    if (mode == MODE_EDITOR) {
        editor.mouseReleased(x, y, button, cam);
    }
}

void ofApp::keyPressed(int key) {
    if (key == OF_KEY_TAB) {
        mode = (mode == MODE_EDITOR) ? MODE_SIMULATE : MODE_EDITOR;
        return;
    }

    if (key == 'g' || key == 'G') {
        splitObjGroups = !splitObjGroups;
        ofLogNotice("ofApp") << "Split OBJ groups: " << (splitObjGroups ? "ON" : "OFF");
        tileManager.loadModelsFromDirectory("", splitObjGroups);
        tileManager.loadXml("data.xml");
        tileManager.layoutTiles(4.0);
        editor.setup(&tileManager);
        simulator.setup(&tileManager);
        return;
    }

    if (mode == MODE_EDITOR) {
        editor.keyPressed(key);
    } else {
        simulator.keyPressed(key);
    }
}
