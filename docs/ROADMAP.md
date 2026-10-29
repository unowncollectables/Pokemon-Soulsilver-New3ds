# Project roadmap

This document tracks the general direction of the project and the milestones that matter most for making the port usable and maintainable.

## Current direction

The project is focused on making a native New 3DS homebrew port of Pokémon SoulSilver using the upstream decompilation as a base. The near-term priority is to stabilize the build flow and port key systems while keeping the repo legal and clean.

## Where things stand (2026-10-29)

Boot, overworld, battles, menus, saving, 3D, the Johto touch-screen map with Pokégear tabs, and local wireless trading
between two consoles work on real hardware at about 30 fps. Still ahead: testing the other Union Room options, the
roamer/breeding tracker and radio tap, online trading with a real DS, and a late 60 fps attempt. The phases below are
the long-term shape and are no longer a strict order.

## Planned phases

### Phase 1: core boot and exploration

- boot sequence and title screen
- overworld rendering and basic navigation
- stereo 3D compatibility checks
- basic hardware validation on New 3DS

### Phase 2: gameplay systems

- battle flow
- menus and interfaces
- Pokégear flow and system integration
- save/load flow and stability checks

### Phase 3: world and event systems

- followers
- day/night cycle
- weekly events and recurring systems
- environmental and script consistency improvements

### Phase 4: broad feature parity

- Pokéathlon integration
- Battle Frontier systems
- side systems and gameplay loops
- deeper compatibility testing on real hardware

### Phase 5: polish and packaging

- cleanup and refactoring
- documentation finalization
- packaging and distribution guidelines
- public-facing release readiness

## Design goals

- keep the repo legal and clean
- maintain a decompilation-based foundation
- prioritize real hardware testing
- document unknowns honestly
- avoid shipping bundled Nintendo-owned content

## Notes

This roadmap is intentionally flexible. Project milestones may shift as work reveals new blockers or as the upstream decompilation changes.
