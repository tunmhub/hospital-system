# Development Log Skill

## Description
Write development logs to `devlog.md` in the project root. Use this skill when the user says "/log" or asks to write a development log entry.

## Instructions

When the user invokes this skill:

1. Read the current `devlog.md` file in the project root (create it if it doesn't exist).
2. Ask the user what they want to log, or if they provide content directly, use that.
3. Append a new log entry to the top of the file (below the title) with the following format:

```markdown
## YYYY-MM-DD HH:MM - <title>

<description of what was done>

- Key change 1
- Key change 2

---
```

4. After writing, confirm to the user what was logged.

## Rules
- Always use the current date and time.
- Each entry should be concise but informative.
- If the user says "/log <message>", use the message as the entry content directly without asking again.
- If the user just says "/log" with no message, ask what they want to log.
