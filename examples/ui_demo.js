/**
 * Demo: SDL2 window + 2D drawing API (requires qianjs built with -DQIANJS_MODULE_UI=ON).
 * Headless/CI: SDL_VIDEODRIVER=dummy qianjs run examples/ui_demo.js 120
 * Local (until close / Esc): qianjs run examples/ui_demo.js
 * `runLoop` 每帧回调参数：`{ frame, events }`（见 `src/native/ui/README.md`）。
 */
import * as ui from 'ui';
import { argv as argvFn } from 'process';

const argv = argvFn();
const maxFrames = argv.length > 1 ? parseInt(argv[1], 10) : -1;
const cap = Number.isFinite(maxFrames) && maxFrames > 0 ? maxFrames : -1;

ui.init(480, 320, 'QianJS + SDL2');
ui.setLineWidth(2);
ui.runLoop((inp) => {
    ui.clear(0.12, 0.14, 0.18, 1);
    ui.setSourceRGBA(0.2, 0.75, 0.55, 1);
    ui.fillRect(40, 40, 160, 120);
    ui.setSourceRGBA(0.95, 0.55, 0.2, 1);
    ui.strokeRect(220, 60, 200, 100);
    ui.setSourceRGBA(0.9, 0.9, 0.95, 1);
    ui.moveTo(40, 260);
    ui.lineTo(440, 280);
    ui.stroke();
    const m = ui.getMouseState();
    ui.setSourceRGBA(1, 0.35, 0.4, 1);
    ui.fillRect(m.x - 5, m.y - 5, 10, 10);
}, cap);
ui.close();
