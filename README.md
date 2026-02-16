# Gravity Boost

A Simple SDL3 Box2D physics puzzle game

## Outline of Game

The premise of Gravity Boost is you can only set initial velocity of your spaceship to reach a destination. In between you and the destination are planets that pull your craft in. This can be used to your advantage or disadvantage.

## Notes

QJS - using claude code to play around and learn some of the SDL3 implementation details.
Making this public, it is impressive how quickly things came together with some aspects that would have taking me a considerable amount of time to align correctly.

My goal is making this for me to have fun, and I am working on an independent repo where I take the ideas from this playground to my own hand written approach.

## Gameplay loop

Start menu

Big title + short “how to play”

ImGui dropdown: Select Level

Buttons: Play, Practice (optional), Quit

Shows best score(s) for the selected level:

Best Ship Time (τ)

(optional) Best Mission Time (t)

Level loads

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

AIM mode (set initial velocity)

Player sets the only control: initial velocity.

Click + drag from the ship to create a velocity vector

UI shows:

speed magnitude (clamped to vel_max)

predicted trajectory preview (dotted line)

optional vector-field “goggles” overlay toggle

Release mouse → commit velocity → state enters RUN.

## RUN mode (no thrust, purely ballistic)

Ship is now a projectile under gravity sources.

Planets attract/repel based on level + player placements.

Optional mechanic: during RUN, player may add a limited number of sinks/repels by clicking:

Left click = sink (attract)

Right click = repel

Shift+click removes nearest placed source

Each placement consumes inventory (e.g., max 2)

## States

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

## Loading levels with cJSON

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
