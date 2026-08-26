# First physical Switch validation

No comparative saves are required.

1. Ensure GTA: San Andreas DE is fully closed.
2. Launch `gtasa-unexplored.nro` once in title-override/full-memory Homebrew Menu mode.
3. Select the Switch profile that owns the GTA save if prompted.
4. Confirm that at least one slot appears and the last-mission key/progress counters look plausible.
5. Press `ZR`. Copy only these small text files back for analysis if anything is wrong:
   - `/switch/gtasa-unexplored/diagnostics.txt`
   - `/switch/gtasa-unexplored/log.txt`
6. Check 2–3 obvious missing markers against the in-game world/map.
7. Close the app, launch GTA, then launch the app through the preferred homebrew path and confirm
   that it clearly switches to the SD backup if Horizon keeps the live save archive busy.
8. With backup mode active, select a marker and press `Y`; confirm that it disappears only locally.
9. Close GTA and reload a fresh live save; temporary marks must be cleared automatically.

The application never asks the user to create before/after save pairs and never commits data back to GTA.
