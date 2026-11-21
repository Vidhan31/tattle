# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.1.0] - 2026-11-22

### Added
- Prompt to switch to offline dictation when network connectivity drops during cloud transcription.
- New Tattle logo and desktop application icons across all supported display sizes.

### Changed
- Redesigned the dictation view, settings screens, and navigation.
- Simplified settings labels and terminology.
- Limited the offline switch prompt to show only when you have at least one local speech model downloaded.

### Fixed
- Prevented speech engine changes from interrupting an active recording until dictation finishes.
- Preserved your selected speech engine when dismissing an offline prompt.

## [1.0.0] - 2026-11-16

### Added
- Initial public release of Tattle, a modern push-to-talk speech-to-text dictation application for Linux (Wayland).
- Hybrid STT engine support: local offline inference with Whisper models powered by `transcribe.cpp` and cloud providers (Groq, Google Gemini, and Meta Musa).
- Optional background dictation daemon (`tattled`) with D-Bus integration (`io.github.tattle.SpeechService`) and system tray StatusNotifierItem integration.
- Hardware key injection service (`keyinjectord`) with `uinput` capability support.
- Pre-built distribution packages for Debian/Ubuntu (`.deb`), Fedora (`.rpm`), and Arch Linux (`.pkg.tar.zst`).

[1.1.0]: https://github.com/Vidhan31/tattle/compare/v1.0.0...v1.1.0
[1.0.0]: https://github.com/Vidhan31/tattle/releases/tag/v1.0.0
