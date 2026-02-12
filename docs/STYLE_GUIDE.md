# Visual Style Guide

**Project:** dc-3ddesignapp (Scan-to-CAD Application)  
**Version:** 1.0  
**Author:** UX Design Team  
**Date:** 2026-02-12

---

## 1. Design Principles

### Brand Identity
- **Professional** — Trusted by engineers and designers
- **Precise** — Every pixel matters, like every micron
- **Efficient** — Clean, no visual clutter
- **Modern** — Contemporary without being trendy

### Visual Language
- Flat design with subtle depth cues
- Rounded corners for approachability
- Consistent spacing using 4px grid
- Generous whitespace for clarity

---

## 2. Color Palette

### 2.1 Dark Theme (Primary)

The application defaults to a dark theme optimized for:
- Extended viewing comfort
- Better visualization of 3D models
- Reduced eye strain in dim environments
- Professional CAD aesthetic

#### Background Colors

| Name | Hex | RGB | Usage |
|------|-----|-----|-------|
| **Background Deep** | `#1a1a1a` | `26, 26, 26` | Main viewport background |
| **Background Base** | `#242424` | `36, 36, 36` | Panel backgrounds |
| **Background Elevated** | `#2d2d2d` | `45, 45, 45` | Cards, dialogs, popovers |
| **Background Hover** | `#383838` | `56, 56, 56` | Hover states |
| **Background Selected** | `#404040` | `64, 64, 64` | Selected items |

#### Surface Colors

| Name | Hex | RGB | Usage |
|------|-----|-----|-------|
| **Surface 1** | `#2a2a2a` | `42, 42, 42` | Toolbar backgrounds |
| **Surface 2** | `#333333` | `51, 51, 51` | Input fields |
| **Surface 3** | `#3d3d3d` | `61, 61, 61` | Buttons (default) |
| **Surface Border** | `#4a4a4a` | `74, 74, 74` | Borders, dividers |

#### Text Colors

| Name | Hex | RGB | Usage |
|------|-----|-----|-------|
| **Text Primary** | `#ffffff` | `255, 255, 255` | Headings, important text |
| **Text Secondary** | `#b3b3b3` | `179, 179, 179` | Body text, labels |
| **Text Tertiary** | `#808080` | `128, 128, 128` | Hints, disabled text |
| **Text Disabled** | `#5c5c5c` | `92, 92, 92` | Disabled controls |

#### Accent Colors

| Name | Hex | RGB | Usage |
|------|-----|-----|-------|
| **Primary** | `#0078d4` | `0, 120, 212` | Primary actions, links, focus |
| **Primary Hover** | `#1a88e0` | `26, 136, 224` | Primary hover state |
| **Primary Active** | `#0066b8` | `0, 102, 184` | Primary pressed state |
| **Primary Light** | `#0078d422` | `0, 120, 212, 13%` | Selection backgrounds |

#### Semantic Colors

| Name | Hex | RGB | Usage |
|------|-----|-----|-------|
| **Success** | `#4caf50` | `76, 175, 80` | Success messages, valid |
| **Warning** | `#ff9800` | `255, 152, 0` | Warnings, caution |
| **Error** | `#f44336` | `244, 67, 54` | Errors, destructive |
| **Info** | `#2196f3` | `33, 150, 243` | Information, tips |

### 2.2 Light Theme (Secondary)

Available for users who prefer light interfaces or work in bright environments.

#### Background Colors (Light)

| Name | Hex | RGB | Usage |
|------|-----|-----|-------|
| **Background Deep** | `#e5e5e5` | `229, 229, 229` | Main viewport background |
| **Background Base** | `#f5f5f5` | `245, 245, 245` | Panel backgrounds |
| **Background Elevated** | `#ffffff` | `255, 255, 255` | Cards, dialogs |
| **Background Hover** | `#ebebeb` | `235, 235, 235` | Hover states |
| **Background Selected** | `#e0e0e0` | `224, 224, 224` | Selected items |

#### Text Colors (Light)

| Name | Hex | RGB | Usage |
|------|-----|-----|-------|
| **Text Primary** | `#1a1a1a` | `26, 26, 26` | Headings |
| **Text Secondary** | `#5c5c5c` | `92, 92, 92` | Body text |
| **Text Tertiary** | `#8c8c8c` | `140, 140, 140` | Hints |
| **Text Disabled** | `#b3b3b3` | `179, 179, 179` | Disabled |

### 2.3 Viewport Colors

Colors used within the 3D viewport:

| Element | Hex | Description |
|---------|-----|-------------|
| **Grid Major** | `#404040` | Major grid lines |
| **Grid Minor** | `#2a2a2a` | Minor grid lines |
| **Axis X** | `#f44336` | X-axis (red) |
| **Axis Y** | `#4caf50` | Y-axis (green) |
| **Axis Z** | `#2196f3` | Z-axis (blue) |
| **Selection Glow** | `#ff9800` | Selected object outline |
| **Hover Glow** | `#0078d4` | Hovered object outline |

### 2.4 Deviation Color Map

Default deviation visualization gradient:

```
Negative ◄────────────────────────────────────────► Positive
  🔵         🟢         ⬜         🟡         🔴
#2196f3   #4caf50   #ffffff   #ffeb3b   #f44336
 -Max       -50%       0        +50%      +Max
```

| Deviation | Color | Hex |
|-----------|-------|-----|
| Maximum negative | Blue | `#2196f3` |
| Negative | Cyan | `#00bcd4` |
| Slightly negative | Green | `#4caf50` |
| Zero | White | `#ffffff` |
| Slightly positive | Yellow | `#ffeb3b` |
| Positive | Orange | `#ff9800` |
| Maximum positive | Red | `#f44336` |

---

## 3. Typography

### 3.1 Font Family

**Primary Font:** Inter  
- Clean, modern sans-serif
- Excellent screen legibility
- Wide character set
- Open source (SIL license)

**Fallback Stack:**
```css
font-family: 'Inter', -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, sans-serif;
```

**Monospace Font:** JetBrains Mono  
- For numerical inputs and measurements
- Tabular figures for alignment
```css
font-family: 'JetBrains Mono', 'SF Mono', 'Consolas', 'Monaco', monospace;
```

### 3.2 Type Scale

Based on 4px baseline grid with 1.25 ratio:

| Name | Size | Weight | Line Height | Usage |
|------|------|--------|-------------|-------|
| **Display** | 24px | 600 | 32px | Splash screen, welcome |
| **Heading 1** | 20px | 600 | 28px | Dialog titles |
| **Heading 2** | 16px | 600 | 24px | Panel headers |
| **Heading 3** | 14px | 600 | 20px | Section headers |
| **Body** | 13px | 400 | 20px | Default text |
| **Body Small** | 12px | 400 | 16px | Secondary info |
| **Caption** | 11px | 400 | 16px | Labels, hints |
| **Overline** | 10px | 500 | 16px | Category labels |

### 3.3 Font Weights

| Weight | Value | Usage |
|--------|-------|-------|
| Regular | 400 | Body text, descriptions |
| Medium | 500 | Buttons, labels |
| Semibold | 600 | Headings, emphasis |
| Bold | 700 | Rarely used (extreme emphasis) |

### 3.4 Numerical Display

Measurements and coordinates use:
- Monospace font
- Right-aligned in columns
- Fixed decimal places per context
- Unit labels in regular weight

**Example:**
```
Position X:  125.432 mm
Position Y:   12.500 mm  
Position Z:    0.000 mm
```

---

## 4. Iconography

### 4.1 Icon Style

**Style:** Outlined with 1.5px stroke  
**Grid:** 24×24px canvas with 20×20px live area  
**Corner radius:** 2px for internal corners  
**Stroke caps:** Round  
**Stroke joins:** Round

### 4.2 Icon Sizes

| Size | Pixels | Usage |
|------|--------|-------|
| **XS** | 12×12 | Inline indicators |
| **Small** | 16×16 | Menu items, compact toolbars |
| **Medium** | 20×20 | Standard toolbar buttons |
| **Large** | 24×24 | Main toolbar, feature icons |
| **XL** | 32×32 | Empty states, welcome |

### 4.3 Icon Colors

| State | Dark Theme | Light Theme |
|-------|------------|-------------|
| Default | `#b3b3b3` | `#5c5c5c` |
| Hover | `#ffffff` | `#1a1a1a` |
| Active | `#0078d4` | `#0078d4` |
| Disabled | `#5c5c5c` | `#b3b3b3` |

### 4.4 Core Icon Set

**File Operations:**
| Icon | Name | Description |
|------|------|-------------|
| 📄 | `file-new` | New project |
| 📂 | `folder-open` | Open file |
| 💾 | `save` | Save |
| 📥 | `import` | Import mesh/CAD |
| 📤 | `export` | Export |

**Edit Operations:**
| Icon | Name | Description |
|------|------|-------------|
| ↶ | `undo` | Undo |
| ↷ | `redo` | Redo |
| ✂ | `cut` | Cut |
| 📋 | `copy` | Copy |
| 📄 | `paste` | Paste |
| 🗑 | `delete` | Delete |

**Selection:**
| Icon | Name | Description |
|------|------|-------------|
| 🖱 | `select-pointer` | Click select |
| ⬜ | `select-box` | Box select |
| ⭕ | `select-lasso` | Lasso select |
| 🖌 | `select-brush` | Brush select |
| 🪄 | `select-magic` | Magic wand |

**View:**
| Icon | Name | Description |
|------|------|-------------|
| 🔍+ | `zoom-in` | Zoom in |
| 🔍- | `zoom-out` | Zoom out |
| ⊞ | `fit` | Fit to view |
| 👁 | `visibility-on` | Visible |
| 👁‍🗨 | `visibility-off` | Hidden |

**Primitives:**
| Icon | Name | Description |
|------|------|-------------|
| ◼ | `primitive-plane` | Plane |
| ⬤ | `primitive-cylinder` | Cylinder |
| 🔺 | `primitive-cone` | Cone |
| ● | `primitive-sphere` | Sphere |

**Tools:**
| Icon | Name | Description |
|------|------|-------------|
| ✏️ | `sketch` | 2D sketch mode |
| 📐 | `section` | Section plane |
| 📏 | `measure` | Measure |
| 🌈 | `deviation` | Deviation map |
| ⚙ | `settings` | Settings |

### 4.5 Icon Guidelines

**Do:**
- Maintain consistent stroke width (1.5px)
- Center icons in touch targets
- Use semantic colors sparingly
- Test at actual display size

**Don't:**
- Mix filled and outlined styles
- Use overly detailed icons
- Rely on color alone for meaning
- Scale icons non-uniformly

---

## 5. Components

### 5.1 Buttons

#### Primary Button
```
┌────────────────────┐
│       Apply        │  ← 32px height
└────────────────────┘
```

| State | Background | Text | Border |
|-------|------------|------|--------|
| Default | `#0078d4` | `#ffffff` | none |
| Hover | `#1a88e0` | `#ffffff` | none |
| Active | `#0066b8` | `#ffffff` | none |
| Disabled | `#3d3d3d` | `#5c5c5c` | none |

**Specs:**
- Height: 32px
- Padding: 16px horizontal
- Border radius: 4px
- Font: 13px medium

#### Secondary Button
| State | Background | Text | Border |
|-------|------------|------|--------|
| Default | `transparent` | `#b3b3b3` | `#4a4a4a` |
| Hover | `#383838` | `#ffffff` | `#4a4a4a` |
| Active | `#404040` | `#ffffff` | `#4a4a4a` |
| Disabled | `transparent` | `#5c5c5c` | `#333333` |

#### Icon Button (Toolbar)
```
┌──────┐
│  🔍  │  ← 32×32px
└──────┘
```

| State | Background | Icon |
|-------|------------|------|
| Default | `transparent` | `#b3b3b3` |
| Hover | `#383838` | `#ffffff` |
| Active/Selected | `#0078d4` | `#ffffff` |
| Disabled | `transparent` | `#5c5c5c` |

**Specs:**
- Size: 32×32px (clickable area)
- Icon size: 20×20px
- Border radius: 4px
- Grouped buttons: 2px gap

### 5.2 Input Fields

#### Text Input
```
┌──────────────────────────────────┐
│ Placeholder text                 │
└──────────────────────────────────┘
```

| State | Background | Text | Border |
|-------|------------|------|--------|
| Default | `#333333` | `#b3b3b3` | `#4a4a4a` |
| Hover | `#333333` | `#b3b3b3` | `#5c5c5c` |
| Focus | `#333333` | `#ffffff` | `#0078d4` |
| Error | `#333333` | `#ffffff` | `#f44336` |
| Disabled | `#2a2a2a` | `#5c5c5c` | `#333333` |

**Specs:**
- Height: 28px
- Padding: 8px horizontal
- Border radius: 4px
- Border width: 1px
- Font: 13px regular (monospace for numbers)

#### Number Input
```
┌────────────────────┬──┬──┐
│        125.00      │▲ │▼ │
└────────────────────┴──┴──┘
```

- Includes increment/decrement buttons
- Supports scroll wheel adjustment
- Optional unit suffix display

#### Slider
```
┌─────────●──────────┐
     50%
```

**Specs:**
- Track height: 4px
- Track color: `#4a4a4a`
- Fill color: `#0078d4`
- Thumb size: 14×14px
- Thumb color: `#ffffff`

### 5.3 Dropdowns

```
┌────────────────────────────┬───┐
│ Selected Option            │ ▼ │
└────────────────────────────┴───┘
         │
         ▼
┌────────────────────────────────┐
│ Option 1                       │
├────────────────────────────────┤
│ Option 2            ✓          │ ← Selected
├────────────────────────────────┤
│ Option 3                       │
└────────────────────────────────┘
```

**Specs:**
- Same height as text input (28px)
- Menu item height: 32px
- Menu max height: 300px (scrollable)
- Shadow: 0 4px 12px rgba(0,0,0,0.3)

### 5.4 Checkboxes & Radio Buttons

#### Checkbox
```
☐ Unchecked   ☑ Checked   ☐ Indeterminate   ☐ Disabled
```

| State | Box | Check |
|-------|-----|-------|
| Unchecked | `#333333` border | none |
| Checked | `#0078d4` fill | `#ffffff` |
| Hover | `#383838` border | — |
| Disabled | `#2a2a2a` border | `#5c5c5c` |

**Specs:**
- Size: 16×16px
- Border radius: 3px
- Check stroke: 2px

#### Radio Button
```
○ Unselected   ● Selected   ○ Disabled
```

**Specs:**
- Size: 16×16px (circular)
- Inner dot: 8×8px when selected

### 5.5 Tabs

```
┌─────────┐ ┌─────────┐ ┌─────────┐
│ Active  │ │  Tab 2  │ │  Tab 3  │
└─────────┘─┴─────────┴─┴─────────┴───────
    ████
```

| State | Text | Indicator |
|-------|------|-----------|
| Active | `#ffffff` | `#0078d4` 2px bar |
| Inactive | `#808080` | none |
| Hover | `#b3b3b3` | none |

### 5.6 Tooltips

```
          ┌───────────────────────┐
          │ Polygon Reduction     │
          │ Shortcut: Ctrl+R      │
          └───────────────────────┘
                    ▼
               [ Button ]
```

**Specs:**
- Background: `#1a1a1a`
- Text: `#ffffff` (title), `#808080` (shortcut)
- Padding: 8px 12px
- Border radius: 4px
- Shadow: 0 2px 8px rgba(0,0,0,0.4)
- Delay: 500ms before showing

### 5.7 Panels

```
┌─ Panel Header ─────────────────[−][×]─┐
│                                        │
│  Content area                          │
│                                        │
└────────────────────────────────────────┘
```

**Specs:**
- Header height: 28px
- Header background: `#2a2a2a`
- Header text: 12px semibold
- Content padding: 12px
- Border: 1px `#4a4a4a`
- Resize handle: 4px grabbable area

---

## 6. Spacing

### 6.1 Base Unit

All spacing based on **4px unit**:

| Token | Pixels | Usage |
|-------|--------|-------|
| `space-1` | 4px | Tight spacing, icon gaps |
| `space-2` | 8px | Between related elements |
| `space-3` | 12px | Component padding |
| `space-4` | 16px | Section spacing |
| `space-5` | 20px | Panel margins |
| `space-6` | 24px | Large section gaps |
| `space-8` | 32px | Major separations |

### 6.2 Component Spacing

| Context | Spacing |
|---------|---------|
| Button group gap | 8px |
| Input stack gap | 8px |
| Form section gap | 16px |
| Panel content padding | 12px |
| Dialog padding | 20px |
| Toolbar button gap | 4px |
| Menu item padding | 8px 12px |

---

## 7. Shadows & Elevation

### 7.1 Elevation Levels

| Level | Shadow | Usage |
|-------|--------|-------|
| 0 | none | Flat elements |
| 1 | `0 1px 2px rgba(0,0,0,0.2)` | Cards, panels |
| 2 | `0 2px 8px rgba(0,0,0,0.3)` | Dropdowns, tooltips |
| 3 | `0 4px 16px rgba(0,0,0,0.4)` | Dialogs, modals |
| 4 | `0 8px 24px rgba(0,0,0,0.5)` | Floating panels |

### 7.2 Focus States

Focus ring for keyboard navigation:
```css
box-shadow: 0 0 0 2px #0078d4;
```

---

## 8. Animation

### 8.1 Timing

| Duration | Usage |
|----------|-------|
| 100ms | Micro-interactions (hover, press) |
| 150ms | Small transitions (color change) |
| 200ms | Standard transitions |
| 300ms | Panel open/close |
| 400ms | Page transitions |

### 8.2 Easing

| Name | Curve | Usage |
|------|-------|-------|
| Ease Out | `cubic-bezier(0, 0, 0.2, 1)` | Enter animations |
| Ease In | `cubic-bezier(0.4, 0, 1, 1)` | Exit animations |
| Ease In-Out | `cubic-bezier(0.4, 0, 0.2, 1)` | Standard |

### 8.3 Motion Guidelines

**Do:**
- Use animation to provide feedback
- Keep animations subtle and quick
- Respect reduced-motion preferences

**Don't:**
- Animate unnecessarily
- Block user input during animation
- Use bouncy or playful effects

---

## 9. Viewport Visual Language

### 9.1 3D Object Appearance

#### Mesh Rendering

| Display Mode | Description |
|--------------|-------------|
| **Shaded** | Smooth Phong shading, default gray `#808080` |
| **Wireframe** | Edge lines only, `#606060` |
| **Points** | Vertex display, `#b3b3b3` |

#### Selection Visualization

| State | Effect |
|-------|--------|
| **Hover** | Subtle highlight glow, `#0078d4` at 30% opacity |
| **Selected** | Orange outline glow, `#ff9800` 2px |
| **Multi-select** | Same orange, all selected objects |

#### CAD Surface Rendering

| Surface Type | Color |
|--------------|-------|
| Default surface | `#6699cc` (blue-gray) |
| Solid body | `#7fb069` (green-gray) |
| Construction | `#cc8855` (orange), dashed edges |
| Sketch | `#ffcc00` (yellow) |

### 9.2 Guide Elements

| Element | Style |
|---------|-------|
| Section plane | Semi-transparent blue `#2196f3` at 20% |
| Clipping box | Dashed orange lines `#ff9800` |
| Dimension lines | White `#ffffff` with arrows |
| Constraint icons | Small blue badges |

---

## 10. Platform Considerations

### 10.1 Windows

- Use system title bar for native feel
- Standard window controls (min/max/close) on right
- Support high DPI displays
- Ctrl+key shortcuts

### 10.2 macOS

- Native title bar with traffic lights on left
- Menu bar in system menu bar (optional)
- Support Retina displays
- ⌘+key shortcuts
- Support trackpad gestures

### 10.3 Common

- Same color palette on both platforms
- Same icon set
- Same spacing and layout
- Identical functionality

---

## 11. Accessibility

### 11.1 Color Contrast

All text meets WCAG 2.1 AA standards:
- Normal text: 4.5:1 minimum
- Large text: 3:1 minimum
- UI components: 3:1 minimum

### 11.2 Focus Indicators

- 2px primary color outline
- Never rely on color alone
- Visible in both themes

### 11.3 Motion

- Respect `prefers-reduced-motion`
- Provide static alternatives
- No auto-playing animations

---

## 12. Asset Specifications

### 12.1 Icons

| Format | Usage |
|--------|-------|
| SVG | Primary format, scalable |
| PNG @1x | Fallback, standard displays |
| PNG @2x | Retina/HiDPI displays |

### 12.2 File Naming

```
icon-{category}-{name}-{size}.{format}
```

Examples:
- `icon-file-open-24.svg`
- `icon-tool-measure-20@2x.png`

---

## Appendix A: Color Tokens (CSS Variables)

```css
:root {
  /* Background */
  --bg-deep: #1a1a1a;
  --bg-base: #242424;
  --bg-elevated: #2d2d2d;
  --bg-hover: #383838;
  --bg-selected: #404040;
  
  /* Surface */
  --surface-1: #2a2a2a;
  --surface-2: #333333;
  --surface-3: #3d3d3d;
  --surface-border: #4a4a4a;
  
  /* Text */
  --text-primary: #ffffff;
  --text-secondary: #b3b3b3;
  --text-tertiary: #808080;
  --text-disabled: #5c5c5c;
  
  /* Accent */
  --primary: #0078d4;
  --primary-hover: #1a88e0;
  --primary-active: #0066b8;
  --primary-light: rgba(0, 120, 212, 0.13);
  
  /* Semantic */
  --success: #4caf50;
  --warning: #ff9800;
  --error: #f44336;
  --info: #2196f3;
  
  /* Viewport */
  --axis-x: #f44336;
  --axis-y: #4caf50;
  --axis-z: #2196f3;
  --selection: #ff9800;
}
```

---

## Appendix B: Design Checklist

Before finalizing any design:

- [ ] Tested in dark and light themes
- [ ] Verified color contrast ratios
- [ ] Keyboard navigation works
- [ ] Touch targets are 32px minimum
- [ ] Spacing follows 4px grid
- [ ] Icons are consistent style
- [ ] Typography follows scale
- [ ] Animations respect reduced-motion
- [ ] Tested on Windows and macOS

---

*Document Version: 1.0*  
*Last Updated: 2026-02-12*
