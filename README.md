# WFC Tile Editor

A visual OpenFrameworks application for creating and editing Wave Function Collapse tile configurations. Design tile neighbor relationships visually and generate the XML configuration needed by the ofxWFC3D addon.

## Overview

- **Load 3D tile models** (OBJ or PLY format) from your `bin/data/` directory
- **Define tile symmetries** (X, I, \, L, T, +) with visual feedback
- **Connect tile faces** to define valid neighbor relationships (horizontal and vertical)
- **Save configurations** automatically to `data.xml` in WFC format
- **Simulate WFC** directly to verify your tile rules produce valid output

## Project Structure

```
wfc-tile-editor/
├── src/
│   ├── main.cpp                  # Entry point
│   ├── ofApp.h / ofApp.cpp       # App shell: shared resources, mode switching
│   ├── EditorMode.h / .cpp       # Editor mode: face picking, connections, overlays
│   ├── SimulateMode.h / .cpp     # Simulate mode: WFC execution, debug/clean view
│   ├── TileManager.h / .cpp      # Tile and connection management, XML I/O
│   └── ModelLoader.h / .cpp      # Pluggable 3D model loading (OBJ, PLY)
├── bin/
│   └── data/                     # Place your .obj or .ply model files here
├── addons.make                   # Dependencies: ofxWFC3D, ofxGui
└── Makefile                      # Build configuration
```

## Getting Started

### 1. Prepare Your Models

Place 3D model files in `bin/data/`:
- **Format**: OBJ (`.obj`) or PLY (`.ply`) files
- **Coordinate system**: Y-up, Z-forward (Blender convention)
- **Scale**: Models should be ~1 unit in size, centered at origin
- **Example**: `down.obj`, `line.ply`, `turn.obj`, etc.

### 2. Build and Run

```bash
make
make RunRelease
```

### 3. Load Existing Configuration (Optional)

If `bin/data/data.xml` exists, it will be loaded automatically on startup, preserving:
- Tile symmetries
- All previously defined neighbor connections

## Editor Mode

### Main Viewport

- **3D Gallery**: Tiles displayed in a grid layout
- **Cube visualization**: Each tile rendered inside a cube with 6 clickable faces
- **Face labels**:
  - `T` = Top face
  - `B` = Bottom face
  - `0-3` = Rotation indices for side faces (counterclockwise around Y-axis)

### Visual Feedback

**When no selection active:**
- Cube wireframes visible in gray
- Connection lines shown at reduced opacity (orange for horizontal, blue for vertical)

**When hovering a face:**
- Only connections for the hovered face are highlighted
- Other connections fade to near-invisible
- Connected destination faces on remote tiles are highlighted with colored quads
- Horizontal connections use an orange-to-red gradient based on face rotation (0=orange, 3=red)

**When you select a face:**
- Selected face shows **bright yellow border** (4px)
- **Compatible faces** (enabled for connection) show dark semi-transparent overlay
- **Incompatible faces** (disabled) are fully invisible and cannot be clicked
- Connected faces show **green edge outline**

**Face compatibility rules:**
- Select a **top/bottom face** → only the opposite vertical face (bottom/top) is enabled
- Select a **side face** (left/right/front/back) → only other side faces are enabled

### Controls

| Action | Effect |
|--------|--------|
| **Left-click face** | Select first face (highlight yellow). Click another compatible face to connect/disconnect. |
| **Left-click empty space** | Deselect current selection. |
| **Left-drag** | Orbit, pan, zoom camera (won't trigger selection). |
| **Right-click tile** | Cycle symmetry type (X → I → \ → L → T → + → X). |
| **ESC** or **D** | Deselect current selection. |
| **S** | Save current configuration to `bin/data/data.xml`. |
| **C** | Clear all neighbor connections (tiles remain). |
| **TAB** | Switch to Simulate mode. |

### Camera (ofEasyCam)

- **Left-drag**: Rotate view (orbit around center)
- **Right-drag**: Pan camera
- **Scroll wheel**: Zoom in/out

## Simulate Mode

Press **TAB** to switch between Editor and Simulate modes. Simulate mode runs the WFC algorithm using the current tile configuration and renders the result in 3D.

Only tiles with at least one connection are included (auto-generated "connected" subset).

### Controls

| Key | Action |
|-----|--------|
| **SPACE** | Generate a new WFC structure (re-runs with a random seed) |
| **V** | Toggle debug view on/off |
| **TAB** | Switch back to Editor mode |

### Views

- **Debug view (default)**: Each tile type rendered in a unique color with rotation index labels.
- **Clean view**: All tiles rendered in uniform gray material.

### Grid Parameters

The simulate GUI panel (top-left) lets you adjust:
- **Width** (X): 1-20 (default 8)
- **Height** (Y): 1-20 (default 4)
- **Length** (Z): 1-20 (default 8)
- **Tile Size**: Auto-detected from model dimensions, adjustable via slider

## Coordinate System & Rotation Indices

### Global Coordinates
```
        Y (UP)
        |
        |
        +---- X (RIGHT)
       /
      /
     Z (FORWARD)
```

### Face Rotation Mapping

For **horizontal neighbor connections**, the rotation index depends on which tile is "left" vs "right":

**Left tile** (counterclockwise from above):
- Rotation 0: Right face
- Rotation 1: Front face
- Rotation 2: Left face
- Rotation 3: Back face

**Right tile** (counterclockwise from above):
- Rotation 0: Left face
- Rotation 1: Back face
- Rotation 2: Right face
- Rotation 3: Front face

**Vertical connections** (top/bottom):
- No rotation index (fully symmetric in Y direction)

## Configuration File Format

Generated `data.xml` structure:

```xml
<set>
  <tiles>
    <tile name="down" symmetry="T"/>
    <tile name="line" symmetry="I"/>
  </tiles>

  <neighbors>
    <!-- Horizontal rules grouped by left tile -->
    <horizontal left="down" right="line"/>
    <horizontal left="turn 1" right="down 3"/>
    <!-- Vertical rules grouped by bottom tile -->
    <vertical bottom="down" top="up"/>
  </neighbors>

  <subsets>
    <subset name="connected">
      <tile name="down"/>
      <tile name="line"/>
    </subset>
  </subsets>
</set>
```

### Tile Symmetries

| Type | Cardinality | Description |
|------|-------------|-------------|
| **X** | 1 | Fully symmetric (e.g., cube, sphere) |
| **I** | 2 | Line symmetry (e.g., straight pipe, 0 = 180) |
| **\\** | 2 | Diagonal symmetry |
| **L** | 4 | L-shape, mirror pairs: 0↔1, 2↔3 |
| **T** | 4 | T-junction, mirror pairs: 1↔3 (0 and 2 are independent) |
| **+** | 4 | Cross/plus, no mirror (all 4 rotations fully independent) |


### Symmetry-Aware Deduplication

The XML output automatically deduplicates rules that are equivalent under symmetry. The WFC engine auto-expands each horizontal rule into rotated and mirrored variants, so redundant connections created in the editor are collapsed to a single canonical XML rule.

## Model Loader Architecture

The model loading system is **pluggable** and extensible:

- **IModelLoader** interface: Abstract base for model format support
- **ObjModelLoader**: Loads `.obj` files with automatic normal generation
- **PlyModelLoader**: Loads `.ply` files (native OF support)
- **ModelLoaderFactory**: Auto-detects format by file extension

To add a new format (e.g., `.gltf`, `.fbx`):
1. Create `XyzModelLoader` inheriting from `IModelLoader`
2. Implement `load()` and `getSupportedExtensions()`
3. Register in `ModelLoaderFactory::ModelLoaderFactory()`

## Workflow Example

1. **Create tile models**: `down.obj`, `line.obj`, `turn.obj`, `up.obj`
2. **Place in** `bin/data/`
3. **Launch app**: `make RunRelease`
4. **Set symmetries**: Right-click each tile to assign symmetry (e.g., down=T, line=I, turn=L)
5. **Define neighbors**: Click a face, then click another compatible face to connect
6. **Verify**: Hover faces to inspect connections visually
7. **Simulate**: Press TAB, then SPACE to generate WFC output
8. **Save**: Press **S** (also auto-saves after each connection)

## Dependencies

- **OpenFrameworks** 0.12.1+
- **ofxWFC3D**: Wave Function Collapse addon
- **ofxGui**: UI panel library
- **C++20** compiler

---

_Disclosure:_
Built with the assistance of AI (Claude Code).

**Version**: 1.0
**Status**: Active Development
