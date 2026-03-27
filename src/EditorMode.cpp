#include "EditorMode.h"

void EditorMode::setup(TileManager* tm) {
    tileManager = tm;

    gui.setup("Editor");
    gui.add(infoLabel.setup("Info", "Click a face to select"));
    gui.setPosition(10, 10);
}

void EditorMode::update(ofEasyCam& cam) {
    hovered = pickFace(ofGetMouseX(), ofGetMouseY(), cam);
}

void EditorMode::draw(ofEasyCam& cam, ofLight& keyLight, ofLight& fillLight, ofLight& backLight, ofMaterial& material) {
    auto& tiles = tileManager->getTiles();

    cam.begin();
    ofEnableDepthTest();

    keyLight.enable();
    fillLight.enable();
    backLight.enable();
    material.begin();

    for (size_t i = 0; i < tiles.size(); i++) {
        drawModel(i, material);
    }

    material.end();
    keyLight.disable();
    fillLight.disable();
    backLight.disable();

    ofDisableLighting();
    for (size_t i = 0; i < tiles.size(); i++) {
        drawCubeOverlay(i);
    }

    drawConnections();

    ofDisableDepthTest();
    cam.end();

    // 2D overlay
    gui.draw();

    ofSetColor(255);
    for (size_t i = 0; i < tiles.size(); i++) {
        auto screenPos = cam.worldToScreen(tiles[i].position - glm::vec3(0, cubeSize * 1.5, 0));
        ofDrawBitmapString(tiles[i].name, screenPos.x - tiles[i].name.size() * 4, screenPos.y);

        auto symPos = cam.worldToScreen(tiles[i].position - glm::vec3(0, cubeSize * 1.8, 0));
        ofDrawBitmapString("[" + tiles[i].symmetry + "]", symPos.x - 12, symPos.y);
    }

    // Rotation labels on faces
    for (size_t i = 0; i < tiles.size(); i++) {
        for (int f = 0; f < 6; f++) {
            CubeFace face = (CubeFace)f;
            bool isSelected = hasFirstSelection && selectedTile == (int)i && selectedFace == face;
            if (!isSelected && !isFaceActive(face)) continue;

            glm::vec3 fc = faceCenterWorld(i, face);
            auto sp = cam.worldToScreen(fc);

            auto faceConns = tileManager->getConnectionsForFace(i, face);
            bool isConnected = !faceConns.empty();

            if (isSelected) ofSetColor(255, 220, 50);
            else if (isConnected) ofSetColor(100, 255, 150);
            else ofSetColor(180, 180, 200);

            std::string label;
            if (face == FACE_TOP) label = "T";
            else if (face == FACE_BOTTOM) label = "B";
            else label = ofToString(TileManager::rotationLabelForFace(face));

            ofDrawBitmapString(label, sp.x - 4, sp.y + 4);
        }
    }

    ofSetColor(180);
    int y = ofGetHeight() - 100;
    ofDrawBitmapString("Click face to select, click another to connect/disconnect", 10, y);
    ofDrawBitmapString("Right-click tile to cycle symmetry type", 10, y + 16);
    ofDrawBitmapString("ESC/D: deselect  |  S: save XML  |  C: clear all connections", 10, y + 32);
}

void EditorMode::drawModel(int tileIndex, ofMaterial& material) {
    auto& tile = tileManager->getTiles()[tileIndex];

    ofPushMatrix();
    ofTranslate(tile.position);

    auto& verts = tile.mesh.getVertices();
    if (!verts.empty()) {
        float scale = tileManager->getUniformScale(cubeSize);

        glm::vec3 minB(FLT_MAX), maxB(-FLT_MAX);
        for (auto& v : verts) {
            minB = glm::min(minB, v);
            maxB = glm::max(maxB, v);
        }
        glm::vec3 center = (minB + maxB) * 0.5f;

        ofPushMatrix();
        ofScale(scale);
        ofTranslate(-center);
        ofSetColor(220);
        tile.mesh.draw();
        ofPopMatrix();
    }

    ofPopMatrix();
}

void EditorMode::drawCubeOverlay(int tileIndex) {
    auto& tile = tileManager->getTiles()[tileIndex];
    glm::vec3 pos = tile.position;

    ofPushMatrix();
    ofTranslate(pos);
    ofNoFill();
    ofSetColor(80, 80, 120);
    ofSetLineWidth(1);
    ofDrawBox(0, 0, 0, cubeSize * 2, cubeSize * 2, cubeSize * 2);
    ofFill();
    ofPopMatrix();

    for (int f = 0; f < 6; f++) {
        CubeFace face = (CubeFace)f;
        bool isSelected = hasFirstSelection && selectedTile == tileIndex && selectedFace == face;
        bool active = isFaceActive(face);
        auto faceConns = tileManager->getConnectionsForFace(tileIndex, face);
        bool isConnected = !faceConns.empty();

        if (isSelected) {
            drawFaceEdges(pos, face, ofColor(255, 220, 50), 4.0f);
        } else if (!active && hasFirstSelection) {
            // Disabled face — invisible
        } else if (active && hasFirstSelection) {
            drawFaceQuad(pos, face, ofColor(10, 10, 20), 140);
            if (isConnected) {
                drawFaceEdges(pos, face, ofColor(100, 255, 150), 2.5f);
            } else {
                drawFaceEdges(pos, face, ofColor(80, 80, 140), 1.5f);
            }
        } else if (isConnected) {
            drawFaceEdges(pos, face, ofColor(100, 255, 150), 2.5f);
        }
    }
}

void EditorMode::drawFaceEdges(const glm::vec3& center, CubeFace face, ofColor color, float lineWidth) {
    float s = cubeSize * 0.98f;
    glm::vec3 offset(0);
    glm::vec3 u(0), v(0);

    switch (face) {
        case FACE_TOP:    offset.y =  cubeSize; u = {s,0,0}; v = {0,0,s}; break;
        case FACE_BOTTOM: offset.y = -cubeSize; u = {s,0,0}; v = {0,0,-s}; break;
        case FACE_LEFT:   offset.x = -cubeSize; u = {0,0,s}; v = {0,s,0}; break;
        case FACE_RIGHT:  offset.x =  cubeSize; u = {0,0,-s}; v = {0,s,0}; break;
        case FACE_FRONT:  offset.z =  cubeSize; u = {s,0,0}; v = {0,s,0}; break;
        case FACE_BACK:   offset.z = -cubeSize; u = {-s,0,0}; v = {0,s,0}; break;
    }

    glm::vec3 c = center + offset;
    glm::vec3 p0 = c - u - v;
    glm::vec3 p1 = c + u - v;
    glm::vec3 p2 = c + u + v;
    glm::vec3 p3 = c - u + v;

    ofSetColor(color);
    ofSetLineWidth(lineWidth);
    ofDrawLine(p0, p1);
    ofDrawLine(p1, p2);
    ofDrawLine(p2, p3);
    ofDrawLine(p3, p0);
    ofSetLineWidth(1);
}

void EditorMode::drawFaceQuad(const glm::vec3& center, CubeFace face, ofColor color, float alpha) {
    float s = cubeSize * 0.99f;
    glm::vec3 offset(0);
    glm::vec3 u(0), v(0);

    switch (face) {
        case FACE_TOP:    offset.y =  cubeSize; u = {s,0,0}; v = {0,0,s}; break;
        case FACE_BOTTOM: offset.y = -cubeSize; u = {s,0,0}; v = {0,0,-s}; break;
        case FACE_LEFT:   offset.x = -cubeSize; u = {0,0,s}; v = {0,s,0}; break;
        case FACE_RIGHT:  offset.x =  cubeSize; u = {0,0,-s}; v = {0,s,0}; break;
        case FACE_FRONT:  offset.z =  cubeSize; u = {s,0,0}; v = {0,s,0}; break;
        case FACE_BACK:   offset.z = -cubeSize; u = {-s,0,0}; v = {0,s,0}; break;
    }

    glm::vec3 c = center + offset;
    ofMesh quad;
    quad.setMode(OF_PRIMITIVE_TRIANGLE_FAN);
    quad.addVertex(c - u - v);
    quad.addVertex(c + u - v);
    quad.addVertex(c + u + v);
    quad.addVertex(c - u + v);

    ofEnableAlphaBlending();
    ofSetColor(color, (int)alpha);
    quad.draw();
    ofDisableAlphaBlending();
}

// Orange-to-red gradient based on face rotation index (0=orange, 3=red)
static ofColor faceRotColor(CubeFace face) {
    int rot = TileManager::rotationLabelForFace(face);
    float t = rot / 3.0f;
    int g = (int)(170 * (1.0f - t) + 40 * t);
    return ofColor(255, g, 30);
}

void EditorMode::drawConnections() {
    auto& connections = tileManager->getConnections();
    bool hovering = (hovered.tileIndex >= 0);

    ofEnableAlphaBlending();

    for (auto& c : connections) {
        glm::vec3 a = faceCenterWorld(c.tileA, c.faceA);
        glm::vec3 b = faceCenterWorld(c.tileB, c.faceB);

        bool isVertical = TileManager::isVerticalFace(c.faceA) && TileManager::isVerticalFace(c.faceB);

        if (!hovering) {
            // No hover — all connections at reduced opacity
            if (isVertical) ofSetColor(100, 200, 255, 180);
            else ofSetColor(255, 150, 50, 180);
            ofSetLineWidth(2);
            ofDrawLine(a, b);
            ofDrawSphere(a, 0.05);
            ofDrawSphere(b, 0.05);
            continue;
        }

        // Face-level filtering: only show connections from the hovered face
        bool matchA = (c.tileA == hovered.tileIndex && c.faceA == hovered.face);
        bool matchB = (c.tileB == hovered.tileIndex && c.faceB == hovered.face);

        if (!matchA && !matchB) {
            // Not connected to hovered face — very dim
            if (isVertical) ofSetColor(100, 200, 255, 25);
            else ofSetColor(255, 150, 50, 25);
            ofSetLineWidth(1);
            ofDrawLine(a, b);
            continue;
        }

        // This connection involves the hovered face — draw highlighted
        if (isVertical) {
            ofSetColor(100, 200, 255);
        } else {
            ofSetColor(faceRotColor(hovered.face));
        }
        ofSetLineWidth(3);
        ofDrawLine(a, b);
        ofDrawSphere(a, 0.06);
        ofDrawSphere(b, 0.06);

        // Highlight the destination face on the remote tile with a colored quad
        int remoteTile = matchA ? c.tileB : c.tileA;
        CubeFace remoteFace = matchA ? c.faceB : c.faceA;
        glm::vec3 remotePos = tileManager->getTiles()[remoteTile].position;

        ofColor destColor = isVertical ? ofColor(100, 200, 255) : faceRotColor(hovered.face);
        drawFaceQuad(remotePos, remoteFace, destColor, 100);
        drawFaceEdges(remotePos, remoteFace, destColor, 2.5f);
    }

    ofSetLineWidth(1);
    ofDisableAlphaBlending();
}

glm::vec3 EditorMode::faceCenterWorld(int tileIndex, CubeFace face) {
    glm::vec3 pos = tileManager->getTiles()[tileIndex].position;
    glm::vec3 offset(0);
    switch (face) {
        case FACE_TOP:    offset.y =  cubeSize; break;
        case FACE_BOTTOM: offset.y = -cubeSize; break;
        case FACE_LEFT:   offset.x = -cubeSize; break;
        case FACE_RIGHT:  offset.x =  cubeSize; break;
        case FACE_FRONT:  offset.z =  cubeSize; break;
        case FACE_BACK:   offset.z = -cubeSize; break;
    }
    return pos + offset;
}

glm::vec3 EditorMode::faceNormal(CubeFace face) {
    switch (face) {
        case FACE_TOP:    return {0, 1, 0};
        case FACE_BOTTOM: return {0,-1, 0};
        case FACE_LEFT:   return {-1,0, 0};
        case FACE_RIGHT:  return {1, 0, 0};
        case FACE_FRONT:  return {0, 0, 1};
        case FACE_BACK:   return {0, 0,-1};
    }
    return {0,0,0};
}

bool EditorMode::isFaceActive(CubeFace face) const {
    if (!hasFirstSelection) return true;
    bool selectedIsVertical = TileManager::isVerticalFace(selectedFace);
    if (selectedIsVertical) {
        if (selectedFace == FACE_TOP) return face == FACE_BOTTOM;
        else return face == FACE_TOP;
    } else {
        return TileManager::isSideFace(face);
    }
}

bool EditorMode::rayPlaneIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                                    const glm::vec3& planePoint, const glm::vec3& planeNormal,
                                    float& t) {
    float denom = glm::dot(planeNormal, rayDir);
    if (std::abs(denom) < 1e-6f) return false;
    t = glm::dot(planePoint - rayOrigin, planeNormal) / denom;
    return t > 0;
}

FaceHit EditorMode::pickFace(int mouseX, int mouseY, ofEasyCam& cam) {
    FaceHit result;
    float bestDist = FLT_MAX;

    glm::vec3 worldNear = cam.screenToWorld(glm::vec3(mouseX, mouseY, 0));
    glm::vec3 worldFar  = cam.screenToWorld(glm::vec3(mouseX, mouseY, 1));
    glm::vec3 rayDir = glm::normalize(worldFar - worldNear);
    glm::vec3 rayOrigin = worldNear;

    auto& tiles = tileManager->getTiles();
    for (size_t i = 0; i < tiles.size(); i++) {
        for (int f = 0; f < 6; f++) {
            CubeFace face = (CubeFace)f;
            if (!isFaceActive(face)) continue;
            glm::vec3 faceCenter = faceCenterWorld(i, face);
            glm::vec3 normal = faceNormal(face);

            float t;
            if (!rayPlaneIntersect(rayOrigin, rayDir, faceCenter, normal, t)) continue;

            glm::vec3 hitPoint = rayOrigin + rayDir * t;
            glm::vec3 local = hitPoint - faceCenter;

            float s = cubeSize;
            bool inBounds = false;
            switch (face) {
                case FACE_TOP:
                case FACE_BOTTOM:
                    inBounds = std::abs(local.x) <= s && std::abs(local.z) <= s;
                    break;
                case FACE_LEFT:
                case FACE_RIGHT:
                    inBounds = std::abs(local.z) <= s && std::abs(local.y) <= s;
                    break;
                case FACE_FRONT:
                case FACE_BACK:
                    inBounds = std::abs(local.x) <= s && std::abs(local.y) <= s;
                    break;
            }

            if (inBounds && t < bestDist) {
                bestDist = t;
                result.tileIndex = i;
                result.face = face;
                result.worldPoint = hitPoint;
            }
        }
    }

    return result;
}

void EditorMode::updateInfo() {
    if (hasFirstSelection) {
        auto& tile = tileManager->getTiles()[selectedTile];
        infoLabel = "Selected: " + tile.name + " [" + TileManager::faceName(selectedFace) + "]";
    } else {
        int nc = tileManager->getConnections().size();
        infoLabel = ofToString(tileManager->getTiles().size()) + " tiles, " + ofToString(nc) + " connections";
    }
}

void EditorMode::mouseReleased(int x, int y, int button, ofEasyCam& cam) {
    if (button == OF_MOUSE_BUTTON_RIGHT) {
        auto hit = pickFace(x, y, cam);
        if (hit.tileIndex >= 0) {
            auto& tile = tileManager->getTiles()[hit.tileIndex];
            if (tile.name != "empty") {
                auto it = std::find(symmetryTypes.begin(), symmetryTypes.end(), tile.symmetry);
                int idx = (it != symmetryTypes.end()) ? (it - symmetryTypes.begin() + 1) % symmetryTypes.size() : 0;
                tileManager->setSymmetry(hit.tileIndex, symmetryTypes[idx]);
                tileManager->saveXml("data.xml");
                updateInfo();
            }
        }
        return;
    }

    if (button != OF_MOUSE_BUTTON_LEFT) return;

    auto hit = pickFace(x, y, cam);
    if (hit.tileIndex < 0) {
        hasFirstSelection = false;
        updateInfo();
        return;
    }

    if (!hasFirstSelection) {
        hasFirstSelection = true;
        selectedTile = hit.tileIndex;
        selectedFace = hit.face;
        updateInfo();
    } else {
        tileManager->toggleConnection(selectedTile, selectedFace, hit.tileIndex, hit.face);
        tileManager->saveXml("data.xml");
        hasFirstSelection = false;
        updateInfo();
    }
}

void EditorMode::keyPressed(int key) {
    if (key == OF_KEY_ESC || key == 'd' || key == 'D') {
        hasFirstSelection = false;
        updateInfo();
    } else if (key == 's' || key == 'S') {
        tileManager->saveXml("data.xml");
        infoLabel = "Saved data.xml";
    } else if (key == 'c' || key == 'C') {
        tileManager->getConnections().clear();
        tileManager->saveXml("data.xml");
        infoLabel = "Cleared all connections";
    }
}
