# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-11-16

### Added
- Initial public release of Tattle, a modern push-to-talk speech-to-text dictation application for Linux (Wayland).
- Hybrid STT engine support: local offline inference with Whisper models powered by `transcribe.cpp` and cloud providers (Groq, Google Gemini, and Meta Musa).
- Optional background dictation daemon (`tattled`) with D-Bus integration (`io.github.tattle.SpeechService`) and system tray StatusNotifierItem integration.
- Hardware key injection service (`keyinjectord`) with `uinput` capability support.
- Pre-built distribution packages for Debian/Ubuntu (`.deb`), Fedora (`.rpm`), and Arch Linux (`.pkg.tar.zst`).

[1.0.0]: https://github.com/Vidhan31/tattle/releases/tag/v1.0.0
