# Follow-up: pin the devkitPro build image

When `pkg.devkitpro.org` is stable again, replace the floating
`devkitpro/devkita64:latest` image in `.github/workflows/build.yml` with a
reviewed, pinned digest. Verify the digest is compatible with libnx and the
SDL2, SDL2_ttf and SDL2_image Switch packages used by this project. Keep the
existing build commands and artifact names unchanged, run both workflow jobs,
and record the chosen image digest and verification date in the README.
