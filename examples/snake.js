/**
 * 贪吃蛇 — `canvas` + `game`（desktop profile）。
 *
 * 运行：`qianjs run examples/snake.js`
 * 无界面/自动化：`QIANJS_NULL_UI=1 qianjs run examples/snake.js 64`
 */
import * as canvas from 'canvas';
import * as game from 'game';
import { argv as argvFn } from 'process';

const CELL = 20;
const COLS = 24;
const ROWS = 16;
const W = COLS * CELL;
const H = ROWS * CELL;
const STEP_MS = 130;

const argv = argvFn();
const maxFrames = argv.length > 1 ? parseInt(argv[1], 10) : -1;
const cap = Number.isFinite(maxFrames) && maxFrames > 0 ? maxFrames : -1;

const cvs = canvas.createCanvas(W, H, { title: 'Snake — QianJS' });
const ctx = cvs.getContext('2d');

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
    ctx.setFillStyle('rgb(18, 20, 26)');
    ctx.clearRect(0, 0, W, H);

    ctx.setFillStyle('rgb(38, 46, 56)');
    for (let gx = 0; gx <= COLS; gx++) {
        ctx.fillRect(gx * CELL, 0, 1, H);
    }
    for (let gy = 0; gy <= ROWS; gy++) {
        ctx.fillRect(0, gy * CELL, W, 1);
    }

    ctx.setFillStyle('rgb(242, 64, 51)');
    ctx.fillRect(food.x * CELL + 2, food.y * CELL + 2, CELL - 4, CELL - 4);

    ctx.setFillStyle('rgb(64, 209, 115)');
    for (let k = 0; k < snake.length; k++) {
        const s = snake[k];
        const inset = k === 0 ? 1 : 2;
        ctx.fillRect(s.x * CELL + inset, s.y * CELL + inset, CELL - inset * 2, CELL - inset * 2);
    }

    ctx.setFillStyle('rgb(89, 115, 140)');
    const barW = Math.min(W - 8, 8 + score * 12);
    ctx.fillRect(4, 4, barW, 6);

    if (dead) {
        ctx.setFillStyle('rgba(217, 38, 31, 0.45)');
        ctx.fillRect(0, 0, W, H);
        ctx.setFillStyle('rgb(255, 255, 255)');
        ctx.fillRect(W * 0.2, H * 0.45, W * 0.6, 8);
        ctx.setFillStyle('rgba(230, 230, 242, 0.9)');
        ctx.fillRect(W * 0.25, H * 0.55, W * 0.5, 4);
    }
}

const runOpts = { maxFrames: cap > 0 ? cap : -1, fps: 60 };

game.run(
    cvs,
    {
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
    },
    runOpts
);
