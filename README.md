# Gravity Boost

A Simple SDL3 Box2D physics puzzle game

## Outline of Game

The premise of Gravity Boost is you can only set initial velocity of your spaceship to reach a destination. In between you and the destination are planets that pull your craft in. This can be used to your advantage or disadvantage.

In addition to the gravity pulling the craft, it will also slow down time relative to not near the gravity source. The score is given by the highest ship time (tau):

τ counting metric (implementation-ready)

You update two clocks every fixed physics step:

𝑡
←
𝑡
+
Δ
𝑡
t←t+Δt

𝜏
←
𝜏
+
Δ
𝑡
⋅
𝑑
𝜏
𝑑
𝑡
τ←τ+Δt⋅
dt
dτ
 ​

Time dilation factor

Use a weak-field + low-speed approximation (game-friendly):

𝑑
𝜏
𝑑
𝑡
≈
1
−
𝑣
2
𝑐
2
−
2
Φ
(
𝑥
)
𝑐
2
dt
dτ
 ​

≈
1−
c
2
v
2
 ​

−
c
2
2Φ(x)
 ​

 ​

Where:

𝑣
2
=

𝑣
𝑥
2
+
𝑣
𝑦
2
v
2
=v
x
2
 ​

+v
y
2
 ​

𝑐
c is a tunable “speed of light” in your game units (picked so effects are visible)

Φ
(
𝑥
)
Φ(x) is gravitational potential at ship position (negative near attractors)

Potential model consistent with your softened gravity

For each gravity source
𝑘
k:

position
𝑝
𝑘
p
k
 ​

strength
𝜇
𝑘
μ
k
 ​

 (positive attract, negative repel)

softening
𝜖
𝑘
ϵ
k
 ​

Φ
(
𝑥
)
=

∑
𝑘
−
𝜇
𝑘
∥
𝑝
𝑘
−
𝑥
∥
2
+
𝜖
𝑘
2
Φ(x)=
k
∑
 ​

−
∥p
k
 ​

−x∥
2
+ϵ
k
2
 ​

 ​

μ
k
 ​

 ​

This pairs naturally with your acceleration model:

𝑎
(
𝑥
)
=

∑
𝑘
𝜇
𝑘
(
𝑝
𝑘
−
𝑥
)
(
∥
𝑝
𝑘
−
𝑥
∥
2
+
𝜖
𝑘
2
)
3
/
2
a(x)=
k
∑
 ​

μ
k
 ​

(∥p
k
 ​

−x∥
2
+ϵ
k
2
 ​

)
3/2
(p
k
 ​

−x)
 ​

Practical clamps (so it behaves well)

Because this is a game, clamp the radicand:

𝑅
=

1
−
𝑣
2
𝑐
2
−
2
Φ
𝑐
2
R=1−
c
2
v
2
 ​

−
c
2
2Φ
 ​

Gameplay loop

1) Start menu

Big title + short “how to play”

ImGui dropdown: Select Level

Buttons: Play, Practice (optional), Quit

Shows best score(s) for the selected level:

Best Ship Time (τ)

(optional) Best Mission Time (t)

1) Level loads

Level defines:

start position

goal position + goal radius

planets (collision radii + gravity strength)

whether player can place sinks/repels + max count

You spawn:

ship at start (not moving)

static planet bodies (for collision)

goal region (sensor/check only)

State enters AIM.

1) AIM mode (set initial velocity)

Player sets the only control: initial velocity.

Click + drag from the ship to create a velocity vector

UI shows:

speed magnitude (clamped to vel_max)

predicted trajectory preview (dotted line)

optional vector-field “goggles” overlay toggle

Release mouse → commit velocity → state enters RUN.

1) RUN mode (no thrust, purely ballistic)

Ship is now a projectile under gravity sources.

Planets attract/repel based on level + player placements.

Optional mechanic: during RUN, player may add a limited number of sinks/repels by clicking:

Left click = sink (attract)

Right click = repel

Shift+click removes nearest placed source

Each placement consumes inventory (e.g., max 2)

If you want “purer puzzle,” you can restrict placements to AIM only.

1) End conditions

SUCCESS: ship enters goal radius.

FAIL: ship collides with any planet radius (or leaves bounds).

Buttons: Retry, Back to Menu, Next Level (if unlocked)

A) Procedural craft render

Goal: draw a ship that looks good without textures.

Approach

Represent ship as 2D triangles in local space (a tiny mesh).

Transform by ship pose (position + angle) from Box2D.

Draw filled triangles + outline.

Add “thruster” flicker when launching (or always-on subtle noise).

Implementation pieces

render_prims.c: triangle list draw helper

render_ship_mesh() returns a static array of vertices + indices

render_ship() applies transform and emits triangles/lines

B) Procedural background (moving starfield)

Goal: looks alive and gives motion cues.

Approach

Multi-layer starfield with parallax.

Optional “nebula haze” as a low-frequency noise field sampled on a coarse grid (cheap rectangles with alpha).

Files

render_background.c: bg_init(), bg_update(dt, cam_delta_px), bg_render()

C) Procedural planets

Goal: recognizable planets + gravity wells.

Approach

Draw base filled circle

Add:

limb darkening (fake shading): draw a few offset translucent circles or radial rings

atmosphere glow: a ring around planet

rings: ellipse-ish polyline or thick ring at an angle (2D cheat)

Files

render_planets.c: render_planet(PlanetDef, camera)

D) Start menu + game states

States

START_MENU

LEVEL_SELECT (can be part of start menu UI)

AIM (player sets initial velocity)

RUN

SUCCESS

FAIL

PAUSED

Where

game.c owns the state machine

render_ui.c draws simple HUD text, but the menu can be ImGui for now.

E) Score

Two timers

mission time t

optional proper time tau (if you keep that scoring mechanic)

Record

per-level bests stored in memory for now; later game_save.c writes JSON.

Where

game_rules.c: updates/compares best score, exposes GameScore

F) Level selection via ImGui dropdown

For now:

ImGui combo lists all .json in assets/levels/ (or a compiled-in list).

Selecting an entry triggers game_load_level(name) and transitions to AIM.

Where

app_imgui.c draws global windows

It calls game_ui(game) which draws “Level” combo and debug toggles.

G) Loading levels with cJSON

Level JSON schema (simple + extensible)

{
  "name": "Slingshot 01",
  "ppm": 50.0,
  "bounds": { "min": [-20, -12], "max": [20, 12] },

  "start": { "pos": [-15, 0], "vel_max": 18.0 },
  "goal":  { "pos": [ 15, 0], "radius": 1.2 },

  "ship":  { "radius": 0.25, "density": 1.0, "restitution": 0.1 },

  "planets": [
    { "pos": [0, 0], "radius": 2.2, "mu": 60.0, "eps": 0.6 }
  ],

  "allow_place": { "sink": true, "repel": true, "max": 3 },

  "ui": { "show_field_default": false }
}

Parsing rules

Required fields: start.pos, goal.pos, goal.radius

Optional: planets array, allow_place, ppm, bounds…

Validate and clamp values (production sanity)

Where

data/fs.c: fs_read_entire_file(path, &buf, &len)

data/json.c: wrapper helpers: json_req_f32(), json_opt_v2(), etc.

game_levels.c: level_load_from_json(path, Level* out)

1) Suggested core structs
game/game.h

Game owns everything: current level, ship state, placement, timers, toggles.

Key sub-structs:

Level (loaded from JSON)

PhysicsWorld (Box2D world + bodies)

Camera

Background

game/game_levels.h

LevelDef: pure data (no Box2D objects)

LevelDB: list of known levels + display names

physics/phys.h

Create/destroy world

Create ship body

Create planet colliders (for crash zones) and store gravity sources separately

physics/phys_gravity.h

Apply gravity forces each fixed step:

for each gravity source: compute accel and apply b2Body_ApplyForceToCenter

1) Execution plan (milestones)
Milestone 1: “Game boots, starfield renders”

SDL3 window + renderer

ImGui overlay (simple debug window)

Background starfield update/render

Fixed timestep loop stub

Milestone 2: “Ship exists and can be launched”

Box2D world

Ship body created at start

Aim mode: mouse drag sets initial velocity

Run mode: integrate + boundaries

Procedural ship render aligned to body angle

Milestone 3: “Planets + gravity”

Parse a hardcoded level struct (no JSON yet)

Render planets

Apply gravity forces from planets

Crash if ship enters planet radius

Milestone 4: “Levels via cJSON + dropdown”

Implement file load + cJSON parse

Level list (hardcoded filenames at first)

ImGui combo selection triggers load/reset

Display level name + best score

Milestone 5: “Score + win/lose screens”

Goal zone + success state

Mission time and/or proper time

Best time table per level

Milestone 6: “Debug overlays become mechanics”

Vector field overlay toggle

“limited goggles” timer or cost

Click to add sink/repel (respects max)

1) ImGui layout (for now)

One window: Game

Level combo

Buttons: Reset, Start, Pause

Toggles: show vector field, show predicted trajectory, show planet radii

Sliders: ppm (debug), gravity scale, max launch speed

Another window (optional): Metrics

FPS, fixed-step substeps, Box2D body count

1) cJSON integration details (clean)

Treat cJSON as an implementation detail:

data/json.h exposes tiny helper functions so game code never calls cJSON_GetObjectItem directly.

Keep all JSON schema assumptions in game_levels.c.

That makes schema changes painless.

If you want, I can also provide:

a starter Level struct + cJSON parsing helpers (json_req_v2, json_opt_f32, json_req_array_len)

a minimal CMakeLists.txt that builds SDL3 + ImGui + cJSON + Box2D cleanly

and a concrete Game state machine outline (START_MENU → AIM → RUN → SUCCESS/FAIL) that matches this layout.
