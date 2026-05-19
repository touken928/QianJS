/**
 * 弹球打砖块 — 需 `ui` 模块（`-DQIANJS_MODULE_UI=ON` + SDL2 子模块）。
 *
 * 运行：`qianjs run examples/breakout.js`
 * 无头/CI：`SDL_VIDEODRIVER=dummy qianjs run examples/breakout.js 2000`（数字为最大帧数，可选）
 *
 * **鼠标** 或 **A/D**、**左右方向键** 移动挡板；**R** 重开；清完砖块或球落底后按 **R**；**Esc** 或关窗退出。
 */
import * as ui from 'ui';
import { createApp, runApp } from 'app';
import { argv as argvFn } from 'process';

const W = 480;
const H = 420;
const PADDLE_W = 88;
const PADDLE_H = 10;
const PADDLE_Y = H - 36;
const BALL_R = 6;
const BRICK_ROWS = 5;
const BRICK_COLS = 10;
const BRICK_H = 20;
const BRICK_TOP = 56;
const BRICK_GAP = 4;
const BRICK_MARGIN = 16;

const argv = argvFn();
const maxFrames = argv.length > 1 ? parseInt(argv[1], 10) : -1;
const cap = Number.isFinite(maxFrames) && maxFrames > 0 ? maxFrames : -1;

const brickW = (W - 2 * BRICK_MARGIN - (BRICK_COLS - 1) * BRICK_GAP) / BRICK_COLS;

/** @type {{ alive: boolean, x: number, y: number, hue: number }[]} */
let bricks;
/** @type {{ x: number, y: number, vx: number, vy: number }} */
let ball;
/** @type {number} 挡板左缘 x */
let paddleX;
let score;
/** 'play' | 'lost' | 'won' */
let phase;
function reset() {
    bricks = [];
    for (let row = 0; row < BRICK_ROWS; row++) {
        for (let col = 0; col < BRICK_COLS; col++) {
            const x = BRICK_MARGIN + col * (brickW + BRICK_GAP);
            const y = BRICK_TOP + row * (BRICK_H + BRICK_GAP);
            const hue = 0.55 + (row * BRICK_COLS + col) * 0.02;
            bricks.push({ alive: true, x, y, hue: hue - Math.floor(hue) });
        }
    }
    paddleX = (W - PADDLE_W) / 2;
    ball = { x: W / 2, y: PADDLE_Y - BALL_R - 8, vx: 220, vy: -260 };
    score = 0;
    phase = 'play';
}

function aliveCount() {
    let n = 0;
    for (let i = 0; i < bricks.length; i++) if (bricks[i].alive) n++;
    return n;
}

function ballBrickHit(b) {
    const bx0 = ball.x - BALL_R;
    const bx1 = ball.x + BALL_R;
    const by0 = ball.y - BALL_R;
    const by1 = ball.y + BALL_R;
    if (!b.alive) return false;
    if (bx1 < b.x || bx0 > b.x + brickW || by1 < b.y || by0 > b.y + BRICK_H) return false;
    const overlapL = bx1 - b.x;
    const overlapR = b.x + brickW - bx0;
    const overlapT = by1 - b.y;
    const overlapB = b.y + BRICK_H - by0;
    const minO = Math.min(overlapL, overlapR, overlapT, overlapB);
    if (minO === overlapL || minO === overlapR) ball.vx *= -1;
    else ball.vy *= -1;
    b.alive = false;
    score += 10;
    return true;
}

function step(dt) {
    if (phase !== 'play') return;

    ball.x += ball.vx * dt;
    ball.y += ball.vy * dt;

    if (ball.x < BALL_R) {
        ball.x = BALL_R;
        ball.vx *= -1;
    } else if (ball.x > W - BALL_R) {
        ball.x = W - BALL_R;
        ball.vx *= -1;
    }
    if (ball.y < BALL_R) {
        ball.y = BALL_R;
        ball.vy *= -1;
    }

    const py = PADDLE_Y;
    if (ball.vy > 0 && ball.y + BALL_R >= py && ball.y + BALL_R <= py + PADDLE_H + 6) {
        const px0 = paddleX;
        const px1 = paddleX + PADDLE_W;
        if (ball.x >= px0 - BALL_R && ball.x <= px1 + BALL_R) {
            ball.y = py - BALL_R - 0.1;
            const cx = (px0 + px1) / 2;
            const off = (ball.x - cx) / (PADDLE_W / 2);
            const speed = Math.sqrt(ball.vx * ball.vx + ball.vy * ball.vy) || 320;
            const ang = (-1.1 + off * 0.9) * Math.PI;
            ball.vx = Math.cos(ang) * speed;
            ball.vy = Math.sin(ang) * speed;
            if (ball.vy > 0) ball.vy *= -1;
        }
    }

    for (let guard = 0; guard < 12; guard++) {
        let any = false;
        for (let i = 0; i < bricks.length; i++) {
            if (ballBrickHit(bricks[i])) {
                any = true;
                break;
            }
        }
        if (!any) break;
    }

    if (ball.y - BALL_R > H) phase = 'lost';
    if (aliveCount() === 0) phase = 'won';
}

function fillDisk(px, py, radius, r, g, b, a) {
    ui.setSourceRGBA(r, g, b, a);
    const r0 = radius | 0;
    for (let dy = -r0; dy <= r0; dy++) {
        const w = Math.sqrt(Math.max(0, radius * radius - dy * dy)) | 0;
        ui.fillRect((px - w) | 0, (py + dy) | 0, w * 2 + 1, 1);
    }
}

function hsvToRgb(h, s, v) {
    const i = (h * 6) | 0;
    const f = h * 6 - i;
    const p = v * (1 - s);
    const q = v * (1 - f * s);
    const t = v * (1 - (1 - f) * s);
    let r = v;
    let g = v;
    let b = v;
    switch (i % 6) {
        case 0:
            r = v;
            g = t;
            b = p;
            break;
        case 1:
            r = q;
            g = v;
            b = p;
            break;
        case 2:
            r = p;
            g = v;
            b = t;
            break;
        case 3:
            r = p;
            g = q;
            b = v;
            break;
        case 4:
            r = t;
            g = p;
            b = v;
            break;
        default:
            r = v;
            g = p;
            b = q;
            break;
    }
    return { r, g, b };
}

function handleEvents(events) {
    for (let i = 0; i < events.length; i++) {
        const ev = events[i];
        if (ev.type === 'keydown') {
            const k = ev.key || '';
            if (k === 'r' || k === 'R') {
                reset();
                continue;
            }
        }
    }
}

function syncPaddle() {
    const m = ui.getMouseState();
    paddleX = m.x - PADDLE_W / 2;
    const step = 10;
    if (ui.isKeyDown('Left') || ui.isKeyDown('a') || ui.isKeyDown('A')) paddleX -= step;
    if (ui.isKeyDown('Right') || ui.isKeyDown('d') || ui.isKeyDown('D')) paddleX += step;
    paddleX = Math.max(0, Math.min(W - PADDLE_W, paddleX));
}

function draw() {
    ui.clear(0.08, 0.09, 0.12, 1);

    ui.setSourceRGBA(0.15, 0.17, 0.22, 1);
    ui.fillRect(0, 0, W, 44);
    ui.setSourceRGBA(0.85, 0.88, 0.92, 1);
    ui.fillRect(12, 14, Math.min(W - 24, 100 + score * 2), 6);

    for (let i = 0; i < bricks.length; i++) {
        const b = bricks[i];
        if (!b.alive) continue;
        const rgb = hsvToRgb(b.hue, 0.55, 0.92);
        ui.setSourceRGBA(rgb.r, rgb.g, rgb.b, 1);
        ui.fillRect(b.x | 0, b.y | 0, brickW, BRICK_H);
        ui.setSourceRGBA(0.04, 0.04, 0.06, 1);
        ui.setLineWidth(1);
        ui.strokeRect(b.x | 0, b.y | 0, brickW, BRICK_H);
    }

    ui.setSourceRGBA(0.35, 0.75, 0.95, 1);
    ui.fillRect(paddleX | 0, PADDLE_Y | 0, PADDLE_W, PADDLE_H);

    fillDisk(ball.x, ball.y, BALL_R, 0.98, 0.92, 0.35, 1);

    if (phase === 'lost' || phase === 'won') {
        ui.setSourceRGBA(0.05, 0.05, 0.08, 0.55);
        ui.fillRect(0, BRICK_TOP - 8, W, H - BRICK_TOP + 8);
        ui.setSourceRGBA(0.9, 0.35, 0.2, 0.9);
        ui.fillRect(W * 0.2, H * 0.42, W * 0.6, 10);
        ui.setSourceRGBA(0.85, 0.86, 0.9, 0.85);
        ui.fillRect(W * 0.28, H * 0.52, W * 0.44, 6);
    }
}

const runOpts = {
    width: W,
    height: H,
    title: '打砖块 — 鼠标/A/D 挡板 | R 重开 | 清砖胜利',
};
if (cap > 0) {
    runOpts.maxFrames = cap;
}

runApp(
    createApp({
        init() {
            reset();
        },
        update(dt, input) {
            const stepDt = Math.min(0.05, Math.max(0, dt));
            handleEvents(input.events);
            syncPaddle();
            step(stepDt);
        },
        render() {
            draw();
        },
    }),
    runOpts
);
