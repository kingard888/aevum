# Changelog

All notable changes to AevumDB are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.2] - 2026-03-09

### Added
- Test script suite for FFI module testing
- `scripts/test.sh` - Main test orchestrator for cargo test execution
- `scripts/test/test.sh` - Core test implementation for `src/aevum/ffi`
- FFI unit test documentation in development guide

### Updated
- Enhanced SCRIPTS.md documentation with test script details
- Updated DEVELOPMENT.md testing section with FFI test procedures
- Added test script checklist to PR template
- Improved CONTRIBUTING.md with testing guidelines

## [1.0.1] - 2026-03-07

### Fixed
- Build script parallel jobs validation (fix `-j N` error)
- Fixed `.gitignore` accidentally ignoring build scripts
- Improved CI reliability by focusing on Linux builds
- Fixed Dependabot configuration errors
- Updated Security Audit workflow to use CodeQL v3

## [1.0.0] - 2026-03-07

### Added
- Initial release
- Core database engine with document storage
- Interactive shell with MongoDB-like interface
- BSON serialization support
- WiredTiger storage backend
- Index support (single field)
- Basic query operators ($gt, $lt, $eq, $ne)
- CRUD operations (insert, find, update, delete)
- Connection pooling
- Authentication framework
- Transaction support
- Data export/import functionality

### Documentation
- Complete user guide
- API reference
- Architecture overview
- Installation instructions
- Development setup guide
- Deployment guide
- Troubleshooting guide

## Version History

- `1.0.2` - Test script suite and documentation updates (March 9, 2026)
- `1.0.1` - Build system and CI configuration fixes (March 7, 2026)
- `1.0.0` - Initial public release (March 7, 2026)

## Upgrading

### From 0.x to 1.0

- No breaking changes expected in the 0.x to 1.0 transition
- Follow [MIGRATION_GUIDE.md](docs/MIGRATION_GUIDE.md) if available

## Reporting Issues

Found a bug? Please report it at: https://github.com/aevumdb/aevum/issues

## Contributing

Want to contribute? See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## Release Schedule

- **Major releases**: Significant features and breaking changes
- **Minor releases**: New features (backward compatible)
- **Patch releases**: Bug fixes and security updates

---

For detailed information about a specific version, see the GitHub [Releases](https://github.com/aevumdb/aevum/releases) page.
