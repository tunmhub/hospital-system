# Conversation Summary Skill

## Description
Summarize the current conversation and save it to a file. Use this skill when the user says "/summary".

## Instructions

When the user invokes this skill:

1. Review the entire conversation history in this session.
2. Create a concise summary covering:
   - What was discussed
   - What decisions were made
   - What actions were taken
   - What files were created or modified
   - What the next steps are
3. Save the summary to `summaries/YYYY-MM-DD_HH-MM.md` (create the `summaries/` directory if needed).
4. Confirm to the user where the summary was saved.
