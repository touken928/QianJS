/**
 * 多窗口 — 仅 `canvas` + `timers`（不用 `game`）。
 *
 * 运行：`qianjs run examples/multi_windows.js`
 * 无头：`QIANJS_NULL_UI=1 qianjs run examples/multi_windows.js 300`
 */
import * as canvas from 'canvas';
import { setInterval, clearInterval } from 'timers';
import { argv as argvFn } from 'process';

const argv = argvFn();
const maxFrames = argv.length > 1 ? parseInt(argv[1], 10) : -1;
const frameCap = Number.isFinite(maxFrames) && maxFrames > 0 ? maxFrames : -1;

const W = 360;
const H = 280;

const left = canvas.createCanvas(W, H, { title: 'Window A — sine' });
const right = canvas.createCanvas(W, H, { title: 'Window B — cosine' });
const ctxA = left.getContext('2d');
const ctxB = right.getContext('2d');

if (!ctxA || !ctxB) {
    throw new Error('expected 2d contexts');
}

let frame = 0;
let closed = false;

function rgba(r, g, b, a) {
    return `rgba(${Math.floor(r * 255)}, ${Math.floor(g * 255)}, ${Math.floor(b * 255)}, ${a})`;
}

function drawWindow(ctx, cvs, phase, hue) {
    const t = frame * 0.04 + phase;
    const cx = cvs.width * 0.5;
    const cy = cvs.height * 0.5;
    const r = 40 + 30 * Math.sin(t);

    ctx.setFillStyle(rgba(0.06, 0.07, 0.1, 1));
    ctx.fillRect(0, 0, cvs.width, cvs.height);

    ctx.setFillStyle(rgba(hue[0], hue[1], hue[2], 0.25));
    ctx.fillRect(0, 0, cvs.width, cvs.height);

    const ox = cx + 80 * Math.cos(t) - r;
    const oy = cy + 50 * Math.sin(t * 1.3) - r;
    ctx.setFillStyle(rgba(hue[0], hue[1], hue[2], 0.9));
    ctx.fillRect(ox, oy, r * 2, r * 2);

    ctx.setFillStyle('rgba(240, 242, 255, 0.92)');
    ctx.setFont('14px sans-serif');
    ctx.fillText(cvs === left ? 'A' : 'B', 12, 22);
    ctx.fillText(`frame ${frame}`, 12, 42);
}

function shouldClosePoll(poll) {
    if (poll.shouldClose) {
        return true;
    }
    for (const e of poll.events) {
        if (e.type === 'keydown' && e.key === 'Escape') {
            return true;
        }
    }
    return false;
}

function shutdown() {
    if (closed) {
        return;
    }
    closed = true;
    clearInterval(timerId);
    left.close();
    right.close();
}

let timerId = 0;

function tick() {
    if (closed) {
        return;
    }

    const pump = canvas.pumpEvents();
    if (pump.quit) {
        shutdown();
        return;
    }

    const pollA = left.pollEvents();
    const pollB = right.pollEvents();
    if (shouldClosePoll(pollA) || shouldClosePoll(pollB)) {
        shutdown();
        return;
    }

    drawWindow(ctxA, left, 0, [0.35, 0.55, 0.95]);
    drawWindow(ctxB, right, 1.2, [0.9, 0.45, 0.35]);

    left.present();
    right.present();

    frame += 1;
    if (frameCap > 0 && frame >= frameCap) {
        shutdown();
    }
}

timerId = setInterval(tick, 16);
