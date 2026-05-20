/**
 * 贪吃蛇 — 使用 `ui` 模块（需 `-DQIANJS_MODULE_UI=ON` 且已 `git submodule update --init third_party/sdl2`）。
 *
 * 运行：`qianjs run examples/snake.js`
 * 无界面/自动化：`SDL_VIDEODRIVER=dummy qianjs run examples/snake.js 3000`（数字为最大帧数，可选）
 *
 * 移动节奏按**墙钟时间**固定（`STEP_MS`），与渲染帧率无关。
 *
 * 操作：**方向键**或 **WASD**；**R** 游戏结束后重开；**Esc** 或关窗退出。
 */
import * as ui from 'ui';
import { argv as argvFn } from 'process';

const CELL = 20;
const COLS = 24;
const ROWS = 16;
const W = COLS * CELL;
const H = ROWS * CELL;
/** 每移动一格的间隔（毫秒），恒定；与 FPS 解耦 */
const STEP_MS = 130;

const argv = argvFn();
const maxFrames = argv.length > 1 ? parseInt(argv[1], 10) : -1;
const cap = Number.isFinite(maxFrames) && maxFrames > 0 ? maxFrames : -1;

/** @type {{ x: number, y: number }[]} */
let snake;
/** @type {{ x: number, y: number }} */
let food;
/** @type {{ x: number, y: number }} */
let dir;
/** @type {{ x: number, y: number } | null} */
let pendingDir;
let score;
let dead;
/** 累计物理时间（毫秒），用于固定步长 */
let moveAcc = 0;
function opposite(a, b) {
    return a.x + b.x === 0 && a.y + b.y === 0;
}

function reset() {
    snake = [
        { x: 8, y: 8 },
        { x: 7, y: 8 },
        { x: 6, y: 8 },
    ];
    dir = { x: 1, y: 0 };
    pendingDir = null;
    score = 0;
    dead = false;
    moveAcc = 0;
    placeFood();
}

function placeFood() {
    for (let tries = 0; tries < 800; tries++) {
        const fx = (Math.random() * COLS) | 0;
        const fy = (Math.random() * ROWS) | 0;
        let onSnake = false;
        for (let i = 0; i < snake.length; i++) {
            if (snake[i].x === fx && snake[i].y === fy) {
                onSnake = true;
                break;
            }
        }
        if (!onSnake) {
            food = { x: fx, y: fy };
            return;
        }
    }
}

function eventToDir(ev) {
    const name = ev.key || '';
    const lower = name.length === 1 ? name.toLowerCase() : name;
    if (name === 'Up' || lower === 'w') return { x: 0, y: -1 };
    if (name === 'Down' || lower === 's') return { x: 0, y: 1 };
    if (name === 'Left' || lower === 'a') return { x: -1, y: 0 };
    if (name === 'Right' || lower === 'd') return { x: 1, y: 0 };
    return null;
}

function handleEvents(events) {
    for (let i = 0; i < events.length; i++) {
        const ev = events[i];
        if (ev.type !== 'keydown') continue;
        const name = ev.key || '';
        if (dead) {
            if (name === 'r' || name === 'R') reset();
            continue;
        }
        const d = eventToDir(ev);
        if (!d) continue;
        if (opposite(d, dir)) continue;
        pendingDir = d;
    }
}

function step() {
    let next = dir;
    if (pendingDir !== null && !opposite(pendingDir, dir)) next = pendingDir;
    pendingDir = null;

    const head = snake[0];
    const nx = head.x + next.x;
    const ny = head.y + next.y;
    dir = next;

    if (nx < 0 || nx >= COLS || ny < 0 || ny >= ROWS) {
        dead = true;
        return;
    }

    const ate = nx === food.x && ny === food.y;
    for (let i = 0; i < snake.length; i++) {
        if (snake[i].x !== nx || snake[i].y !== ny) continue;
        if (i === snake.length - 1 && !ate) continue;
        dead = true;
        return;
    }
    const newHead = { x: nx, y: ny };
    const nextSnake = [newHead];
    for (let j = 0; j < snake.length; j++) nextSnake.push(snake[j]);
    if (!ate) nextSnake.pop();
    else {
        score += 1;
        placeFood();
    }
    snake = nextSnake;
}

function draw() {
    ui.clear(0.07, 0.08, 0.1, 1);
    ui.setSourceRGBA(0.15, 0.18, 0.22, 1);
    for (let gx = 0; gx <= COLS; gx++) {
        ui.fillRect(gx * CELL, 0, 1, H);
    }
    for (let gy = 0; gy <= ROWS; gy++) {
        ui.fillRect(0, gy * CELL, W, 1);
    }

    ui.setSourceRGBA(0.95, 0.25, 0.2, 1);
    ui.fillRect(food.x * CELL + 2, food.y * CELL + 2, CELL - 4, CELL - 4);

    ui.setSourceRGBA(0.25, 0.82, 0.45, 1);
    for (let k = 0; k < snake.length; k++) {
        const s = snake[k];
        const inset = k === 0 ? 1 : 2;
        ui.fillRect(s.x * CELL + inset, s.y * CELL + inset, CELL - inset * 2, CELL - inset * 2);
    }

    ui.setSourceRGBA(0.35, 0.45, 0.55, 1);
    const barW = Math.min(W - 8, 8 + score * 12);
    ui.fillRect(4, 4, barW, 6);

    if (dead) {
        ui.setSourceRGBA(0.85, 0.15, 0.12, 0.45);
        ui.fillRect(0, 0, W, H);
        ui.setSourceRGBA(1, 1, 1, 1);
        ui.fillRect(W * 0.2, H * 0.45, W * 0.6, 8);
        ui.setSourceRGBA(0.9, 0.9, 0.95, 0.9);
        ui.fillRect(W * 0.25, H * 0.55, W * 0.5, 4);
    }
}

const runOpts = { width: W, height: H, title: 'Snake — QianJS' };
if (cap > 0) {
    runOpts.maxFrames = cap;
}

ui.runApp(
    ui.createApp({
        init() {
            reset();
        },
        update(dt, input) {
            const ms = Math.max(0, dt * 1000);
            handleEvents(input.events);
            if (!dead && ms > 0) {
                moveAcc += ms;
                while (moveAcc >= STEP_MS) {
                    moveAcc -= STEP_MS;
                    step();
                    if (dead) break;
                }
            }
        },
        render() {
            draw();
        },
    }),
    runOpts
);
