# AGENTS.md – instructions for AI agents (Claude Code, Cursor, Copilot …)

Read this file first. The human you work with is a student on the team; explain things simply.
The team talks Danish – write docs, Issues and PR texts in Danish. Code and identifiers in English.

## Project

TamaPal – H3 middle project at Mercantec (deadline: presentation 11/11-2026).
A game where an **Arduino Oplà** (MKR WiFi 1010 + MKR IoT Carrier) is the controller,
a **.NET backend** runs the game logic, and a **browser frontend** draws the game on a canvas.

Current MVP proposal: Pong against the wall with one Oplà (tilt = paddle, button = start).

| Person (GitHub) | Area | Folder |
|---|---|---|
| Oleh (`deusflow`) | Arduino: input (IMU, button), MQTT, LED/sound feedback | `Arduino/` |
| Oliver (`OliverAgB`) | Backend (ASP.NET Core): game loop, MQTT, SignalR hub, database | `Backend/` |
| Andrii (`AndriiSoft`) | Frontend: canvas, start/game over screens, SignalR client | `Frontend/` |

Stay in your human's folder. Do not change another person's area without asking them.

## Read before you start

1. `docs/MESSAGES.md` – **the contract between the parts**: MVP, project flow, the 4 messages (MQTT + SignalR JSON). Most important file.
2. `docs/MOEDE-2026-11-02.md` – teachers' workflow and Definition of Done.
3. `docs/TIL-DISKUSSION.md` – earlier questions and Oliver's full game loop vision.
4. `docs/TamaPal-game-design-doc.docx` – original game design (Oleh).

## Your first task: give input on the plan

Before writing code, help your human answer the questions in **Del 2** of `docs/MESSAGES.md` for their part:

- Is the message format enough for your part? What is missing or unclear?
- What do you need from the other two (names, endpoints, topics, timing)?
- What are the risks in your part, and what is the smallest working version?

Then help your human write 3–5 Issues for their part (see workflow below).
Do not fill in **Del 3 – Aftale** alone: decisions are made together by the group.

## Workflow (GitHub)

- **Board:** https://github.com/orgs/Mercantec-GHC/projects/143 – columns Backlog → To do → In progress → Review → Done.
- **Issues** are created in the `H3-TamaPal` repo (not in other groups' repos), with title, description,
  **acceptkriterier** (checklist), one assignee, and added to the TamaPal project.
- **Branches:** `main` is protected (releases only, PR + 1 approval). `dev` is the default, shared branch.
  Work in a short branch from `dev`: `git switch dev` → `git pull` → `git switch -c feature/<name>`.
- **Pull Request** into `dev`, write `Closes #<issue>` and check every acceptance criterion before asking for review.
- Never push directly to `main`, never `push --force`, never delete branches without the human asking.
- Small commits with short messages that say what changed.

## Message contract (summary – full version in docs/MESSAGES.md)

- MQTT `tamapal/opla-1/input` (Oplà → backend, ~20/s): `{ "tilt": 0.35, "button": false }`
- MQTT `tamapal/opla-1/event` (backend → Oplà, on events): `{ "type": "hit" | "miss" | "gameover" }`
- SignalR hub `/gamehub`, event `GameState` (backend → browser, ~30/s):
  `{ "status": "playing", "ball": {"x":0.5,"y":0.3}, "paddle": {"y":0.4}, "score": 7, "lives": 2 }`
- SignalR method `StartGame()` (browser → backend)

Coordinates are 0–1. If you need to change the contract, change `docs/MESSAGES.md` in a PR and tell the group – never change it silently in code.

## Rules

- **No secrets in git.** Arduino: Wi-Fi/MQTT passwords only in `config.h` / `arduino_secrets.h` (git-ignored;
  commit a `*.example.h` without real values). Backend: database connection strings and passwords via
  `dotnet user-secrets` or environment variables / `.env` – never in `appsettings*.json`.
- Simple solutions over clever ones – this is a learning project and everyone must be able to explain the code.
- Use fake data to work without waiting for the other parts.
