# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## 0.0.4 - 2021-02-15
### Fixed
- Assert on writes to wave pattern RAM.
- Bank switching sometimes failed.

### Added
- lsdpack -r flag to record raw register writes and disable all optimisations.

## 0.0.3 - 2021-02-12
### Added
- Game Boy Sound System (GBS) support.
- Allow multiple input LSDj ROMs. e.g. "./lsdpack.exe 1.gb 2.gb 3.gb ..."
- Timeout with error message when a song has played for one hour.

### Fixed
- Crackling sound on DMG from LSDj v8.8.0+ soft envelopes.
- rgbasm 0.4.0 support.
- Visual Studio 2019 build.
- Crash when failing to load .gb file.
- Skip past empty songs.

## 0.0.2 - 2018-10-24
### Fixed
- Fixed bank overflow on sample playback near bank end. Thanks Def Mechan!

## 0.0.1 - 2018-07-03
### Added
- Initial release.
