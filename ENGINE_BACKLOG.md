# Engine Backlog

## Rules
- Keep TODOs out of source files.
- Every item must have: scope, expected result, and done criteria.
- Close or rewrite vague tasks ("maybe", "should", "conviene").

## P0 - Stabilize Architecture

### ENG-001 - Split State_Game responsibilities
- Priority: P0
- Scope: Separate gameplay update, debug overlay, camera/view setup, and HUD drawing.
- Why: `State_Game` is still a hotspot with mixed responsibilities.
- Done when:
  - `State_Game` delegates to small private methods for each concern.
  - No behavior regressions in pause/menu/debug/hud flow.

### ENG-002 - Externalize hardcoded gameplay parameters
- Priority: P0
- Scope: Move jump/attack timings, projectile speed/lifetime/damage, and camera constants to config files.
- Why: Hardcoded values block balancing and reuse.
- Done when:
  - Values are loaded from config.
  - Defaults exist if config is missing.
  - No magic numbers remain in core gameplay logic.

### ENG-003 - Clean TODO debt in source
- Priority: P0
- Scope: Remove remaining TODO comments from `.cpp/.h`, keep tasks here.
- Why: Source comments are currently noisy and non-actionable.
- Done when:
  - `TODO` search in `src/` and `include/` returns only intentional third-party references.

## P1 - Data & Content Pipeline

### ENG-004 - Character data unification
- Priority: P1
- Scope: Define one consistent schema for character config (stats, collider, animation sheet refs).
- Why: Current loading path is fragmented.
- Done when:
  - Single documented format is used for all character archetypes.
  - Loader validates required fields.

### ENG-005 - SpriteSheet validation rules
- Priority: P1
- Scope: Validate duplicate animations, malformed lines, invalid frame ranges.
- Why: Runtime issues should fail early with clear logs.
- Done when:
  - Loader emits explicit warnings/errors for invalid entries.
  - Duplicate behavior is deterministic and documented.

### ENG-006 - Map authoring specification
- Priority: P1
- Scope: Write mapping spec (required layers, object classes, properties like collision/friction/traps/doors).
- Why: Content breaks easily without strict conventions.
- Done when:
  - A mapper can create a valid map without reading code.
  - Common mistakes are listed with examples.

## P2 - Engine Quality

### ENG-007 - Include dependency hygiene
- Priority: P2
- Scope: Remove accidental transitive includes and enforce direct includes.
- Why: Reduces fragile compile dependencies.
- Done when:
  - Each translation unit includes what it uses.
  - No compile break from include order changes.

### ENG-008 - Random engine policy
- Priority: P2
- Scope: Choose one RNG strategy and expose controlled seeding.
- Why: Needed for deterministic tests/replays later.
- Done when:
  - RNG source is centralized.
  - Seed can be fixed from config/debug mode.

### ENG-009 - Input conflict policy
- Priority: P2
- Scope: Prevent contradictory movement inputs (left+right) and define resolution rule.
- Why: Avoid inconsistent controller state.
- Done when:
  - Conflict rule is explicit (cancel/last input/priority).
  - Behavior is tested.

## P3 - Future Features (Not Now)

### ENG-010 - Scripting integration
- Priority: P3
- Scope: Evaluate scripting layer for gameplay iteration.
- Why: Useful only after core systems are stable.
- Done when:
  - Clear choice made (or rejected) with tradeoffs documented.

### ENG-011 - Multiplayer feasibility study
- Priority: P3
- Scope: Define game model requirements before any implementation.
- Why: Premature implementation would create rework.
- Done when:
  - Written architecture note exists (authoritative sim, rollback, etc.).

---

## Next 3 Recommended Tasks
1. ENG-003 (remove in-source TODO debt now)
2. ENG-002 (config-driven gameplay constants)
3. ENG-004 (character schema unification)
