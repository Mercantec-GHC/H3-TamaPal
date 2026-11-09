# AGENTS.md – instructions for AI agents (Claude Code, Cursor, Copilot …)

Read this file first. The human you work with is a student on the team; explain things simply.
The team talks Danish – write docs, Issues and PR texts in Danish. Code and identifiers in English.

## Project

TamaPal – H3 middle project at Mercantec (deadline: presentation 11/11-2026).
A game where an **Arduino Oplà** (MKR WiFi 1010 + MKR IoT Carrier) is the controller,
a **.NET backend** runs the game logic, and a **browser frontend** draws the game on a canvas.

Current MVP proposal: Pong against the wall with one Oplà (tilt or buttons = paddle, button = start).
Status, dates and open questions: `docs/PLAN.md`.

| Person (GitHub) | Area | Folder |
|---|---|---|
| Oleh (`deusflow`) | Arduino: input (IMU, button), MQTT, LED/sound feedback | `Arduino/` |
| Oliver (`OliverAgB`) | Backend (ASP.NET Core): game loop, MQTT, SignalR hub, database | `Backend/` |
| Andrii (`AndriiSoft`) | Frontend: canvas, start/game over screens, SignalR client | `Frontend/` |

Stay in your human's folder. Do not change another person's area without asking them.

## Read before you start

1. `docs/MESSAGES.md` – **the contract between the parts**: MVP, project flow, the 4 messages (MQTT + SignalR JSON). Most important file.
2. `docs/PLAN.md` – dates, Definition of Done mapped to our plan, status per part, open questions.
3. `docs/MOEDE-2026-11-02.md` – teachers' workflow and Definition of Done.
4. `docs/TIL-DISKUSSION.md` – earlier questions and Oliver's full game loop vision.
5. `docs/TamaPal-game-design-doc.docx` – original game design (Oleh).

## Before you change anything

Check `docs/PLAN.md` for status and open questions. If your human's part still has open points, help them answer the questions in **Del 2** of `docs/MESSAGES.md` for their part:

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
- **Push directly to `dev`** (group decision 4/11): `git switch dev` → `git pull` → commit → `git push`.
  Always `git pull` before you push. No Pull Request or review is needed for `dev`.
  A short `feature/<name>` branch + PR into `dev` is optional, e.g. if you want feedback first.
- Write `Closes #<issue>` in the commit message when a commit finishes an Issue.
- Never push directly to `main`, never `push --force`, never delete branches without the human asking.
- Small commits with short messages that say what changed.

## Message contract (summary – full version in docs/MESSAGES.md)

- MQTT `StormLamaerne/Tamapal/opla-1/input` (Oplà → backend, ~20/s + on button change):
  `{ "up": false, "down": true, "button": false, "tiltX": 0.0, "tiltY": 0.35, "tilt": 0.35 }` (T1 = up, T0 = down, T2 = button)
- MQTT `StormLamaerne/Tamapal/opla-1/event` (backend → Oplà, on events): `{ "type": "hit" | "miss" | "gameover" }`
- Topics are case-sensitive and must start with the group name `StormLamaerne/` (broker rule).
- SignalR hub `/gamehub`, event `GameState` (backend → browser, ~30/s):
  `{ "status": "playing", "ball": {"x":0.5,"y":0.3}, "paddle": {"y":0.4}, "score": 7, "lives": 2 }`
- SignalR method `StartGame()` (browser → backend)

Coordinates are 0–1. If you need to change the contract, change `docs/MESSAGES.md` and tell the group – never change it silently in code.

## Rules

- **No secrets in git.** Arduino: Wi-Fi/MQTT passwords only in `config.h` / `arduino_secrets.h` (git-ignored;
  commit a `*.example.h` without real values). Backend: database connection strings and passwords via
  `dotnet user-secrets` or environment variables / `.env` – never in `appsettings*.json`.
- Simple solutions over clever ones – this is a learning project and everyone must be able to explain the code.
- Use fake data to work without waiting for the other parts.
