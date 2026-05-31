/**
 * 双人五子棋 — `canvas` + `game`。
 *
 * 运行：`qianjs run examples/gomoku.js`
 * 无头：`QIANJS_NULL_UI=1 qianjs run examples/gomoku.js 5000`
 */
import * as canvas from 'canvas';
import * as game from 'game';
import { argv as argvFn } from 'process';

const SIZE = 15;
const CELL = 36;
const MARGIN = 28;
const TOP = 52;
const STONE_R = Math.floor(CELL * 0.38);
const CLICK_R2 = (CELL * 0.42) * (CELL * 0.42);

const W = MARGIN * 2 + (SIZE - 1) * CELL;
const H = TOP + (SIZE - 1) * CELL + MARGIN;

const argv = argvFn();
const maxFrames = argv.length > 1 ? parseInt(argv[1], 10) : -1;
const cap = Number.isFinite(maxFrames) && maxFrames > 0 ? maxFrames : -1;

const cvs = canvas.createCanvas(W, H, { title: '五子棋 — QianJS' });
const ctx = cvs.getContext('2d');

function rgba(r, g, b, a) {
    return `rgba(${Math.round(r * 255)}, ${Math.round(g * 255)}, ${Math.round(b * 255)}, ${a})`;
}

/** @type {number[][]} */
let board;
let turn;
let winner;
/** @type {{ cx: number, cy: number } | null} */
let lastMove;

function reset() {
    board = [];
    for (let y = 0; y < SIZE; y++) {
        const row = [];
        for (let x = 0; x < SIZE; x++) row.push(0);
        board.push(row);
    }
    turn = 1;
    winner = 0;
    lastMove = null;
}

function gridToPixel(cx, cy) {
    return { x: MARGIN + cx * CELL, y: TOP + cy * CELL };
}

function pixelToGrid(mx, my) {
    const cx = Math.round((mx - MARGIN) / CELL);
    const cy = Math.round((my - TOP) / CELL);
    if (cx < 0 || cx >= SIZE || cy < 0 || cy >= SIZE) return null;
    const p = gridToPixel(cx, cy);
    const dx = mx - p.x;
    const dy = my - p.y;
    if (dx * dx + dy * dy > CLICK_R2) return null;
    return { cx, cy };
}

function countDir(r, c, dr, dc, side) {
    let n = 0;
    let rr = r + dr;
    let cc = c + dc;
    while (rr >= 0 && rr < SIZE && cc >= 0 && cc < SIZE && board[rr][cc] === side) {
        n++;
        rr += dr;
        cc += dc;
    }
    return n;
}

function checkWin(r, c, side) {
    const dirs = [
        [1, 0],
        [0, 1],
        [1, 1],
        [1, -1],
    ];
    for (let i = 0; i < dirs.length; i++) {
        const dr = dirs[i][0];
        const dc = dirs[i][1];
        const total = 1 + countDir(r, c, dr, dc, side) + countDir(r, c, -dr, -dc, side);
        if (total >= 5) return true;
    }
    return false;
}

function tryPlace(cx, cy) {
    if (winner !== 0) return;
    if (board[cy][cx] !== 0) return;
    board[cy][cx] = turn;
    lastMove = { cx, cy };
    if (checkWin(cy, cx, turn)) {
        winner = turn;
        return;
    }
    turn = turn === 1 ? 2 : 1;
}

function fillDisk(px, py, radius, r, g, b, a) {
    ctx.setFillStyle(rgba(r, g, b, a));
    const r0 = radius | 0;
    for (let dy = -r0; dy <= r0; dy++) {
        const w = Math.sqrt(Math.max(0, radius * radius - dy * dy)) | 0;
        ctx.fillRect((px - w) | 0, (py + dy) | 0, w * 2 + 1, 1);
    }
}

function handleEvents(events) {
    for (let i = 0; i < events.length; i++) {
        const ev = events[i];
        if (ev.type === 'keydown' && (ev.key === 'r' || ev.key === 'R')) {
            reset();
            continue;
        }
        if (ev.type !== 'mousedown' || ev.button !== 'left') continue;
        const g = pixelToGrid(ev.x, ev.y);
        if (g === null) continue;
        tryPlace(g.cx, g.cy);
    }
}

function draw() {
    ctx.setFillStyle(rgba(0.18, 0.14, 0.1, 1));
    ctx.clearRect(0, 0, W, H);

    ctx.setFillStyle(rgba(0.92, 0.82, 0.62, 1));
    ctx.fillRect(MARGIN - 8, TOP - 8, (SIZE - 1) * CELL + 16, (SIZE - 1) * CELL + 16);

    ctx.setStrokeStyle(rgba(0.12, 0.1, 0.08, 1));
    ctx.setLineWidth(1.5);
    for (let i = 0; i < SIZE; i++) {
        const x0 = MARGIN + i * CELL;
        const y0 = TOP;
        const y1 = TOP + (SIZE - 1) * CELL;
        ctx.beginPath();
        ctx.moveTo(x0, y0);
        ctx.lineTo(x0, y1);
        ctx.stroke();
        const yy = TOP + i * CELL;
        ctx.beginPath();
        ctx.moveTo(MARGIN, yy);
        ctx.lineTo(MARGIN + (SIZE - 1) * CELL, yy);
        ctx.stroke();
    }

    const stars = [
        [3, 3],
        [11, 3],
        [3, 11],
        [11, 11],
        [7, 7],
    ];
    for (let s = 0; s < stars.length; s++) {
        const sx = stars[s][0];
        const sy = stars[s][1];
        const p = gridToPixel(sx, sy);
        ctx.setFillStyle(rgba(0.22, 0.16, 0.12, 1));
        ctx.fillRect(p.x - 2, p.y - 2, 5, 5);
    }

    for (let cy = 0; cy < SIZE; cy++) {
        for (let cx = 0; cx < SIZE; cx++) {
            const v = board[cy][cx];
            if (v === 0) continue;
            const p = gridToPixel(cx, cy);
            if (v === 1) fillDisk(p.x, p.y, STONE_R, 0.08, 0.08, 0.1, 1);
            else fillDisk(p.x, p.y, STONE_R, 0.94, 0.94, 0.96, 1);
        }
    }

    if (lastMove !== null && winner === 0) {
        const p = gridToPixel(lastMove.cx, lastMove.cy);
        ctx.setStrokeStyle(rgba(0.95, 0.75, 0.2, 0.85));
        ctx.setLineWidth(1);
        ctx.strokeRect(p.x - STONE_R - 2, p.y - STONE_R - 2, (STONE_R + 2) * 2, (STONE_R + 2) * 2);
    }

    ctx.setFillStyle(rgba(0.92, 0.89, 0.84, 1));
    ctx.fillRect(0, 0, W, TOP - 4);
    fillDisk(28, 26, 11, 0.06, 0.06, 0.08, 1);
    fillDisk(76, 26, 11, 0.93, 0.93, 0.95, 1);
    ctx.setLineWidth(3);
    if (winner === 0) {
        ctx.setStrokeStyle(rgba(0.95, 0.75, 0.15, 1));
        if (turn === 1) ctx.strokeRect(14, 12, 28, 28);
        else ctx.strokeRect(62, 12, 28, 28);
    } else {
        ctx.setStrokeStyle(rgba(0.92, 0.22, 0.12, 1));
        if (winner === 1) ctx.strokeRect(10, 8, 36, 36);
        else ctx.strokeRect(58, 8, 36, 36);
    }

    if (winner !== 0) {
        ctx.setFillStyle(rgba(0.12, 0.06, 0.05, 0.5));
        ctx.fillRect(MARGIN - 8, TOP - 8, (SIZE - 1) * CELL + 16, (SIZE - 1) * CELL + 16);
    }
}

const runOpts = { maxFrames: cap > 0 ? cap : -1 };

game.run(
    cvs,
    {
        init() {
            reset();
        },
        update(dt, input) {
            handleEvents(input.events);
        },
        render() {
            draw();
        },
    },
    runOpts
);
