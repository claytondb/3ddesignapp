# UI Design Document

**Project:** dc-3ddesignapp (Scan-to-CAD Application)  
**Version:** 1.0  
**Author:** UX Design Team  
**Date:** 2026-02-12

---

## 1. Design Philosophy

### Goals
- **Familiar to CAD users** — Follow established patterns from SolidWorks, Fusion 360, QuickSurface
- **Workflow-oriented** — UI adapts based on current task (mesh editing, sketching, surfacing)
- **Performance-first** — Minimize UI overhead when handling large meshes
- **Cross-platform consistency** — Identical experience on Windows and macOS

### Principles
1. **Progressive disclosure** — Show advanced options only when needed
2. **Direct manipulation** — Prefer on-canvas interaction over dialog boxes
3. **Visual feedback** — Real-time deviation display, selection highlighting
4. **Undo confidence** — Every action is reversible

---

## 2. Main Window Layout

```
┌──────────────────────────────────────────────────────────────────────────┐
│ Menu Bar                                                                  │
├──────────────────────────────────────────────────────────────────────────┤
│ Main Toolbar                                                              │
├────────────┬─────────────────────────────────────────────────┬───────────┤
│            │                                                  │           │
│  Object    │                                                  │ Properties│
│  Browser   │              3D Viewport                         │   Panel   │
│  Panel     │                                                  │           │
│            │                                                  │           │
│ (200px)    │           (flex - fills remaining)               │  (280px)  │
│            │                                                  │           │
│            │                                                  │           │
│            │                   [ViewCube]                     │           │
│            │                                                  │           │
├────────────┴─────────────────────────────────────────────────┴───────────┤
│ Context Toolbar (changes based on active tool/mode)                       │
├──────────────────────────────────────────────────────────────────────────┤
│ Status Bar                                                                │
└──────────────────────────────────────────────────────────────────────────┘
```

### 2.1 Viewport (Center)

The 3D viewport is the primary workspace occupying maximum screen real estate.

**Features:**
- OpenGL/Vulkan rendered 3D scene
- Ground plane grid (toggleable)
- Coordinate axis indicator (bottom-left corner)
- ViewCube (top-right corner) for quick view orientation
- Selection highlight with glow effect
- Real-time deviation color overlay

**View Modes:**
| Mode | Description | Shortcut |
|------|-------------|----------|
| Shaded | Smooth shaded with lighting | `1` |
| Wireframe | Mesh edges only | `2` |
| Shaded + Wireframe | Combined view | `3` |
| X-Ray | Semi-transparent for hidden geometry | `4` |
| Deviation Map | Color-coded distance to reference | `5` |

### 2.2 Object Browser Panel (Left)

Collapsible panel showing scene hierarchy and history.

**Sections:**
```
┌─ Object Browser ─────────────────┐
│ ▼ Meshes                         │
│   ├─ 📦 imported_scan.stl        │
│   │   ├─ Original (hidden)       │
│   │   └─ Reduced (active)        │
│   └─ 📦 reference_part.obj       │
│                                  │
│ ▼ Primitives                     │
│   ├─ ◼ Plane_1                   │
│   ├─ ⬤ Cylinder_1                │
│   └─ ● Sphere_1                  │
│                                  │
│ ▼ Sketches                       │
│   ├─ ✎ Section_1                 │
│   └─ ✎ Section_2                 │
│                                  │
│ ▼ Surfaces                       │
│   ├─ ◇ Extruded_1                │
│   └─ ◇ FreeForm_1                │
│                                  │
│ ▼ Bodies                         │
│   └─ ⬡ Solid_1                   │
└──────────────────────────────────┘
```

**Interactions:**
- Click to select
- Double-click to rename
- Right-click for context menu (Hide, Delete, Isolate, Export)
- Drag to reorder
- Eye icon toggle for visibility
- Lock icon to prevent accidental edits

### 2.3 Properties Panel (Right)

Context-sensitive panel showing properties of selected object(s).

**When nothing selected:**
```
┌─ Properties ─────────────────────┐
│                                  │
│  Scene Statistics                │
│  ────────────────                │
│  Meshes: 2                       │
│  Triangles: 1,245,678            │
│  Surfaces: 5                     │
│  Bodies: 1                       │
│                                  │
│  Coordinate System               │
│  ────────────────                │
│  Units: mm ▼                     │
│  Origin: [0, 0, 0]               │
│                                  │
└──────────────────────────────────┘
```

**When mesh selected:**
```
┌─ Properties ─────────────────────┐
│                                  │
│  imported_scan.stl               │
│  ════════════════════            │
│                                  │
│  ▼ Geometry                      │
│    Triangles: 1,245,678          │
│    Vertices:  623,456            │
│    Bounds:    120 × 80 × 45 mm   │
│    Has holes: Yes (3)            │
│                                  │
│  ▼ Display                       │
│    Color: [■ #808080] ⚙         │
│    Opacity: ━━━━━━━━━● 100%      │
│    Show edges: ☑                 │
│                                  │
│  ▼ Transform                     │
│    Position:                     │
│      X: [    0.000  ] mm         │
│      Y: [    0.000  ] mm         │
│      Z: [    0.000  ] mm         │
│    Rotation:                     │
│      X: [    0.00   ] °          │
│      Y: [    0.00   ] °          │
│      Z: [    0.00   ] °          │
│                                  │
│  ▼ Deviation (if enabled)        │
│    Min: -0.05 mm                 │
│    Max: +0.12 mm                 │
│    Avg: 0.03 mm                  │
│    Std Dev: 0.02 mm              │
│                                  │
│    [Color Legend Bar]            │
│    -0.1  ────●────  +0.1 mm      │
│                                  │
└──────────────────────────────────┘
```

### 2.4 Main Toolbar

Horizontal toolbar below menu bar with primary tool groups.

```
┌──────────────────────────────────────────────────────────────────────────┐
│ [📁▾][💾][↶][↷] │ [🖱][🎯][🔲][🖌] │ [📐][⬤][◇][✎] │ [📏][🔍][💡] │ [▶]  │
│  File  Undo/Redo │   Selection     │    Create      │   Analyze   │ Run   │
└──────────────────────────────────────────────────────────────────────────┘
```

**Tool Groups:**

| Group | Tools | Description |
|-------|-------|-------------|
| File | New, Open, Save, Import, Export | File operations dropdown |
| History | Undo, Redo | With history dropdown |
| Selection | Select, Box Select, Lasso, Brush | Selection modes |
| Create | Primitives, Section, Sketch, Surface | Creation tools |
| Analyze | Measure, Deviation, Draft Analysis | Analysis tools |
| Run | Execute/Confirm current operation | Contextual action |

### 2.5 Context Toolbar

Appears below viewport when a tool is active. Shows tool-specific options.

**Example: Polygon Reduction active**
```
┌──────────────────────────────────────────────────────────────────────────┐
│ Polygon Reduction │ Target: ○ Triangles ● Percentage │ [━━━━●━━━] 50%   │
│                   │ Preserve: ☑ Edges ☑ Sharp       │ [Preview] [Apply] │
└──────────────────────────────────────────────────────────────────────────┘
```

**Example: 2D Sketch mode**
```
┌──────────────────────────────────────────────────────────────────────────┐
│ 2D Sketch │ [/][○][⌒][~] │ Constraints: [═][⊥][⊙][📐] │ [Exit Sketch]   │
│           │ Line Arc Circle Spline │ H V Coincident Dimension │          │
└──────────────────────────────────────────────────────────────────────────┘
```

### 2.6 Status Bar

Bottom bar showing contextual information.

```
┌──────────────────────────────────────────────────────────────────────────┐
│ Ready │ Selection: 1,234 triangles │ Cursor: (45.2, 12.8, 0.0) mm │ 60 FPS│
└──────────────────────────────────────────────────────────────────────────┘
```

**Status Bar Sections:**
- **Mode indicator** — Ready, Processing, Sketch Mode, etc.
- **Selection info** — Count and type of selected elements
- **Cursor position** — 3D coordinates under mouse
- **Performance** — FPS counter (toggleable)

---

## 3. Navigation Patterns

### 3.1 Viewport Navigation

| Action | Mouse | Trackpad (macOS) | Description |
|--------|-------|------------------|-------------|
| **Orbit** | Middle-drag | Two-finger drag | Rotate view around focal point |
| **Pan** | Shift + Middle-drag | Shift + Two-finger | Move view laterally |
| **Zoom** | Scroll wheel | Pinch | Zoom in/out |
| **Zoom to fit** | `F` key | `F` key | Fit all visible to viewport |
| **Zoom to selection** | `Z` key | `Z` key | Fit selection to viewport |
| **Quick views** | ViewCube click | ViewCube click | Front, Back, Left, Right, Top, Bottom, Iso |

### 3.2 ViewCube Interaction

```
        ┌─────────┐
       /   TOP   /│
      /_________/ │
      │         │ │ FRONT
      │  FRONT  │/
      └─────────┘
```

- Click face → Snap to orthographic view
- Click edge → Snap to edge-on view
- Click corner → Snap to isometric view
- Drag on cube → Free orbit
- Home icon → Reset to default isometric

### 3.3 Selection Modes

| Mode | Icon | Behavior |
|------|------|----------|
| **Click Select** | 🖱 | Single element under cursor |
| **Box Select** | 🔲 | All elements within rectangle |
| **Lasso Select** | ⭕ | Freeform selection region |
| **Brush Select** | 🖌 | Paint to add to selection |
| **Magic Wand** | 🪄 | Select connected region by curvature |

**Modifiers:**
- `Shift` + Click — Add to selection
- `Ctrl/Cmd` + Click — Remove from selection
- `Ctrl/Cmd` + `A` — Select all
- `Escape` — Clear selection

---

## 4. Menu Structure

### File
```
File
├─ New Project                    Ctrl+N
├─ Open...                        Ctrl+O
├─ Open Recent                  ▶
├─ ─────────────────────────────
├─ Save                          Ctrl+S
├─ Save As...                    Ctrl+Shift+S
├─ ─────────────────────────────
├─ Import                       ▶
│   ├─ Mesh (STL, OBJ, PLY)...
│   └─ CAD (STEP, IGES)...
├─ Export                       ▶
│   ├─ Mesh (STL)...
│   ├─ CAD (STEP)...
│   └─ CAD (IGES)...
├─ ─────────────────────────────
├─ Project Settings...
└─ Exit                          Alt+F4
```

### Edit
```
Edit
├─ Undo                          Ctrl+Z
├─ Redo                          Ctrl+Y
├─ ─────────────────────────────
├─ Cut                           Ctrl+X
├─ Copy                          Ctrl+C
├─ Paste                         Ctrl+V
├─ Delete                        Del
├─ ─────────────────────────────
├─ Select All                    Ctrl+A
├─ Deselect All                  Esc
├─ Invert Selection              Ctrl+I
└─ ─────────────────────────────
└─ Preferences...                Ctrl+,
```

### Mesh
```
Mesh
├─ Polygon Reduction...
├─ Smoothing...
├─ ─────────────────────────────
├─ Fill Holes...
├─ Remove Outliers...
├─ De-feature...
├─ ─────────────────────────────
├─ Clipping Box
├─ Split Mesh
└─ Merge Meshes
```

### Align
```
Align
├─ Interactive Align             A
├─ Align to WCS...
├─ ─────────────────────────────
├─ N-Point Alignment...
├─ Global Fine Align...
├─ ─────────────────────────────
└─ Find Symmetry Plane...
```

### Create
```
Create
├─ Primitives                   ▶
│   ├─ Plane                     P
│   ├─ Cylinder                  C
│   ├─ Cone
│   └─ Sphere
├─ ─────────────────────────────
├─ Section Plane...              S
├─ Multiple Sections...
├─ ─────────────────────────────
├─ 2D Sketch                     K
├─ 3D Sketch
├─ ─────────────────────────────
├─ Fit Surface...
├─ Auto Surface...
├─ ─────────────────────────────
├─ Extrude...                    E
├─ Revolve...                    R
├─ Loft...
├─ Sweep...
└─ ─────────────────────────────
└─ Free-form Surface...
```

### Modify
```
Modify
├─ Trim...                       T
├─ Extend...
├─ ─────────────────────────────
├─ Boolean                      ▶
│   ├─ Union
│   ├─ Subtract
│   └─ Intersect
├─ ─────────────────────────────
├─ Fillet...
├─ Chamfer...
├─ ─────────────────────────────
├─ Shell...
├─ Thicken...
├─ Offset...
├─ ─────────────────────────────
├─ Mirror...
├─ Pattern                      ▶
│   ├─ Linear Pattern...
│   └─ Circular Pattern...
└─ ─────────────────────────────
└─ Move Face...
```

### Analyze
```
Analyze
├─ Measure                      ▶
│   ├─ Distance
│   ├─ Angle
│   └─ Radius
├─ ─────────────────────────────
├─ Deviation Map...              D
├─ Scan-to-CAD Compare...
├─ ─────────────────────────────
├─ Draft Analysis...
├─ Straightness Analysis...
└─ ─────────────────────────────
└─ Mesh Statistics
```

### View
```
View
├─ Standard Views               ▶
│   ├─ Front                     Numpad 1
│   ├─ Back                      Ctrl+Numpad 1
│   ├─ Left                      Numpad 3
│   ├─ Right                     Ctrl+Numpad 3
│   ├─ Top                       Numpad 7
│   ├─ Bottom                    Ctrl+Numpad 7
│   └─ Isometric                 Numpad 0
├─ ─────────────────────────────
├─ Zoom to Fit                   F
├─ Zoom to Selection             Z
├─ ─────────────────────────────
├─ Display Mode                 ▶
│   ├─ Shaded                    1
│   ├─ Wireframe                 2
│   ├─ Shaded + Wireframe        3
│   ├─ X-Ray                     4
│   └─ Deviation Map             5
├─ ─────────────────────────────
├─ Show Grid                     G
├─ Show Axes                     
├─ Show ViewCube
├─ ─────────────────────────────
├─ Object Browser Panel          F2
├─ Properties Panel              F3
└─ ─────────────────────────────
└─ Full Screen                   F11
```

### Help
```
Help
├─ Getting Started
├─ Tutorials
├─ Keyboard Shortcuts...
├─ ─────────────────────────────
├─ Documentation
├─ Release Notes
├─ ─────────────────────────────
├─ Check for Updates...
└─ About
```

---

## 5. Panel Designs

### 5.1 Polygon Reduction Dialog

```
┌─ Polygon Reduction ──────────────────────────────────────────┐
│                                                               │
│  Original: 1,245,678 triangles                               │
│                                                               │
│  Target                                                       │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │ ○ Triangle count:  [ 100,000          ]                 │ │
│  │ ● Percentage:      [━━━━━━━●━━━━━━━━━] 50%              │ │
│  │ ○ Max deviation:   [ 0.1              ] mm              │ │
│  └─────────────────────────────────────────────────────────┘ │
│                                                               │
│  Result: ~622,839 triangles                                  │
│                                                               │
│  Options                                                      │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │ ☑ Preserve boundary edges                               │ │
│  │ ☑ Preserve sharp features (angle > [30] °)              │ │
│  │ ☐ Lock vertex colors                                    │ │
│  └─────────────────────────────────────────────────────────┘ │
│                                                               │
│  Preview                                                      │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │                                                          │ │
│  │              [Mini 3D Preview Window]                   │ │
│  │                                                          │ │
│  └─────────────────────────────────────────────────────────┘ │
│  ☑ Auto-preview                                              │
│                                                               │
│                            [ Cancel ]  [ Apply ]  [ OK ]     │
└───────────────────────────────────────────────────────────────┘
```

### 5.2 Deviation Analysis Panel

```
┌─ Deviation Analysis ─────────────────────────────────────────┐
│                                                               │
│  Reference: imported_scan.stl                                │
│  Compare to: Solid_1                                         │
│                                                               │
│  Results                                                      │
│  ────────────────────────────────────────                    │
│  Minimum:     -0.052 mm                                      │
│  Maximum:     +0.127 mm                                      │
│  Average:     +0.031 mm                                      │
│  Std Dev:      0.024 mm                                      │
│  RMS:          0.039 mm                                      │
│                                                               │
│  Color Scale                                                  │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │  -0.1    ─────────────●─────────────    +0.1   mm       │ │
│  │   🔵      🟢      ⬜      🟡      🔴                      │ │
│  └─────────────────────────────────────────────────────────┘ │
│  Min: [ -0.1  ] mm    Max: [ +0.1  ] mm    [Auto]           │
│                                                               │
│  Display Options                                              │
│  ────────────────────────────────────────                    │
│  ○ Show on mesh                                              │
│  ● Show on CAD                                               │
│  ☑ Show out-of-tolerance in red                             │
│  ☑ Show labels at extremes                                   │
│                                                               │
│  Tolerance: [ 0.05 ] mm                                      │
│  In tolerance: 94.2%   Out: 5.8%                            │
│                                                               │
│                            [ Export Report... ]  [ Close ]   │
└───────────────────────────────────────────────────────────────┘
```

### 5.3 2D Sketch Toolbox (Floating)

```
┌─ Sketch Tools ────────────────┐
│                               │
│  Draw                         │
│  [/][○][⌒][~][▭][⬡]          │
│  Line Circle Arc Spline Rect  │
│                               │
│  ─────────────────────────    │
│                               │
│  Fit to Section               │
│  [/⚡][○⚡][⌒⚡]               │
│  Auto-Line Auto-Arc Auto-Fit  │
│                               │
│  ─────────────────────────    │
│                               │
│  Constraints                  │
│  [═][‖][⊥][⊙][📐][🔒]         │
│  Horiz Parallel Perp Coinc    │
│                               │
│  ─────────────────────────    │
│                               │
│  Modify                       │
│  [✂][📋][↔][🔄]               │
│  Trim Offset Mirror Rotate    │
│                               │
└───────────────────────────────┘
```

### 5.4 Primitive Fitting Options

```
┌─ Fit Cylinder ───────────────────────────────────────────────┐
│                                                               │
│  Selection: 12,456 triangles                                 │
│                                                               │
│  Fitted Parameters                                            │
│  ────────────────────────────────────────                    │
│  Radius:     [ 25.032   ] mm    🔒                           │
│  Height:     [ 48.156   ] mm    🔒                           │
│                                                               │
│  Axis Direction                                               │
│  X: [ 0.000 ]  Y: [ 0.000 ]  Z: [ 1.000 ]                   │
│                                                               │
│  Center Point                                                 │
│  X: [ 12.5  ]  Y: [ 8.3   ]  Z: [ 0.0   ]                   │
│                                                               │
│  Fit Quality                                                  │
│  ────────────────────────────────────────                    │
│  Max deviation:  0.045 mm   ✓ Good                          │
│  Avg deviation:  0.012 mm                                    │
│  [████████████████████░░░] 92% within 0.05mm                │
│                                                               │
│  Options                                                      │
│  ☑ Constrain to selection bounds                            │
│  ☑ Extend 10% beyond selection                              │
│  ☐ Snap radius to standard sizes                            │
│                                                               │
│                            [ Cancel ]  [ Apply ]  [ OK ]     │
└───────────────────────────────────────────────────────────────┘
```

---

## 6. Keyboard Shortcuts

### Global

| Action | Windows | macOS |
|--------|---------|-------|
| New project | `Ctrl+N` | `⌘N` |
| Open | `Ctrl+O` | `⌘O` |
| Save | `Ctrl+S` | `⌘S` |
| Save As | `Ctrl+Shift+S` | `⌘⇧S` |
| Undo | `Ctrl+Z` | `⌘Z` |
| Redo | `Ctrl+Y` | `⌘⇧Z` |
| Preferences | `Ctrl+,` | `⌘,` |
| Quit | `Alt+F4` | `⌘Q` |

### Selection

| Action | Windows/macOS |
|--------|---------------|
| Select All | `Ctrl/⌘+A` |
| Deselect | `Escape` |
| Invert Selection | `Ctrl/⌘+I` |
| Add to Selection | `Shift+Click` |
| Remove from Selection | `Ctrl/⌘+Click` |
| Delete Selected | `Delete` / `Backspace` |

### View

| Action | Shortcut |
|--------|----------|
| Zoom to Fit | `F` |
| Zoom to Selection | `Z` |
| Front View | `Numpad 1` |
| Back View | `Ctrl+Numpad 1` |
| Right View | `Numpad 3` |
| Left View | `Ctrl+Numpad 3` |
| Top View | `Numpad 7` |
| Bottom View | `Ctrl+Numpad 7` |
| Isometric | `Numpad 0` |
| Toggle Grid | `G` |
| Shaded Mode | `1` |
| Wireframe Mode | `2` |
| Shaded+Wire Mode | `3` |
| X-Ray Mode | `4` |
| Deviation Mode | `5` |
| Full Screen | `F11` |

### Tools

| Action | Shortcut |
|--------|----------|
| Select Mode | `Q` |
| Interactive Align | `A` |
| Section Plane | `S` |
| 2D Sketch | `K` |
| Plane Primitive | `P` |
| Cylinder Primitive | `C` |
| Extrude | `E` |
| Revolve | `R` |
| Trim | `T` |
| Deviation Map | `D` |
| Measure Distance | `M` |

### During Sketch

| Action | Shortcut |
|--------|----------|
| Line | `L` |
| Circle | `O` |
| Arc | `A` |
| Spline | `S` |
| Rectangle | `R` |
| Trim | `T` |
| Horizontal Constraint | `H` |
| Vertical Constraint | `V` |
| Exit Sketch | `Escape` |

---

## 7. Workflow Modes

The UI adapts based on current workflow mode:

### 7.1 Mesh Mode (Default)

- Object browser shows mesh hierarchy
- Properties show mesh statistics
- Toolbar emphasizes mesh tools
- Selection works on mesh regions

### 7.2 Sketch Mode

- Viewport shows 2D plane with section
- Sketch toolbox appears (floating)
- Context toolbar shows sketch tools
- Grid snapping enabled
- Constraint visualization active

### 7.3 Surface Mode

- Shows CAD surfaces and bodies
- Properties show surface parameters
- Trim/boolean tools available
- Deviation overlay on mesh

### 7.4 Analysis Mode

- Deviation color map active
- Analysis panel visible
- Measurement tools available
- Statistics displayed

---

## 8. Dialog Patterns

### Standard Button Order

- Windows: `[ Cancel ]  [ Apply ]  [ OK ]`
- macOS: `[ Cancel ]  [ Apply ]  [ OK ]` (same, for consistency)

### Input Validation

- Real-time validation with red border
- Tooltip shows error message
- OK button disabled until valid

### Progress Indicators

- Modal progress for blocking operations
- Status bar progress for background tasks
- Estimated time remaining shown

### Confirmation Dialogs

Used for destructive actions:
```
┌─ Confirm Delete ─────────────────────────────┐
│                                               │
│  ⚠️  Delete "imported_scan.stl"?              │
│                                               │
│  This action cannot be undone.               │
│                                               │
│              [ Cancel ]  [ Delete ]          │
└───────────────────────────────────────────────┘
```

---

## 9. Responsive Behavior

### Minimum Window Size
- Width: 1024px
- Height: 768px

### Panel Collapsing
- Panels collapse to icons at narrow widths
- Double-click panel header to collapse
- Drag dividers to resize

### Floating Panels
- All panels can be undocked
- Remember positions across sessions
- Support multi-monitor setups

---

## 10. Accessibility

### Keyboard Navigation
- All functions accessible via keyboard
- Tab through toolbar buttons
- Arrow keys in object browser
- Focus indicators visible

### Color Vision
- Don't rely solely on color
- Deviation map has texture option
- High contrast mode available

### Screen Readers
- Toolbar buttons have labels
- Status changes announced
- Object browser is navigable

---

*Document Version: 1.0*  
*Next Review: After user testing*
