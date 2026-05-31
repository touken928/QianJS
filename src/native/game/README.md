# game

Fixed-step frame loop and keyboard polling for canvas-backed apps.

| API | Description |
|-----|-------------|
| `game.run(canvas, app, options?)` | Blocking loop: poll SDL → `update(dt, input)` → `render()` → present. `app` must provide `update` and `render`; optional `init` / `shutdown`. |
| `game.isKeyDown(key)` | Keyboard state (string key name / code). |

**Options** (`game.run` third argument): `width`, `height`, `title`, `maxFrames`, `fps`, `fixedStep`.

Requires module **`canvas`** (desktop profile enables both). Headless CI: `QIANJS_NULL_UI=1`.
