# XSigma Project Governance

This document describes the governance model for the XSigma project, including decision-making processes, roles and responsibilities, and contribution guidelines.

## Table of Contents

- [Project Overview](#project-overview)
- [Roles and Responsibilities](#roles-and-responsibilities)
- [Decision-Making Process](#decision-making-process)
- [Contribution Process](#contribution-process)
- [Code Review Process](#code-review-process)
- [Release Process](#release-process)
- [Conflict Resolution](#conflict-resolution)
- [Changes to Governance](#changes-to-governance)

## Project Overview

XSigma is an open-source quantitative analysis library designed for high-performance CPU and GPU computing. The project is **owned and maintained by XSigmaAnalyitix** and welcomes contributions from the community.

### Project Authority

**XSigmaAnalyitix retains full ownership and control** of the XSigma project, including:

- **Final decision authority** on all technical, architectural, and strategic matters
- **Exclusive control** over the project roadmap and feature priorities
- **Sole discretion** over contribution acceptance and code merging
- **Ultimate authority** over licensing, governance, and project direction
- **Right to modify or reject** any contribution for any reason

While community input is valued and considered, **XSigmaAnalyitix maintainers have final authority** over all project decisions.

### Project Goals

- Provide a high-performance, production-ready quantitative analysis library
- Maintain cross-platform compatibility (Windows, Linux, macOS)
- Ensure code quality through comprehensive testing and analysis
- Foster an open, welcoming, and inclusive community
- Adhere to industry best practices for security and software engineering
- **Sustain development** through dual licensing business model

### License

XSigma is dual-licensed under:
- **GPL-3.0-or-later** for open-source use
- **Commercial license** for proprietary use (offered exclusively by XSigmaAnalyitix)

**Contributors grant XSigmaAnalyitix the right** to offer commercial licenses for the combined work without additional permission or compensation.

See [LICENSE](LICENSE) and [CONTRIBUTING.md](CONTRIBUTING.md) for details.

## Roles and Responsibilities

### Maintainers

**Maintainers** are individuals with commit access to the repository who are responsible for:

- Reviewing and merging pull requests
- Triaging and responding to issues
- Making architectural and design decisions
- Ensuring code quality and adherence to standards
- Managing releases and versioning
- Enforcing the Code of Conduct
- **Exercising final authority** over all project decisions

#### Current Maintainers

**XSigmaAnalyitix organization members** are the current maintainers and have **final decision-making authority** on all project matters.

**Lead Maintainer**: XSigmaAnalyitix (organizational authority)

#### Maintainer Authority

Maintainers have **sole discretion** over:

- **Accepting or rejecting** any contribution
- **Architectural decisions** and design direction
- **Feature prioritization** and roadmap planning
- **Breaking changes** and API modifications
- **Release timing** and versioning
- **Licensing decisions** and commercial terms
- **Governance changes** and policy updates

**Community input is valued** but does not override maintainer decisions.

#### Becoming a Maintainer

Maintainer status is granted **at the sole discretion of XSigmaAnalyitix**. Criteria include:

- Demonstrate sustained, high-quality contributions over time (typically 12+ months)
- Show deep understanding of the codebase and project goals
- Exhibit good judgment in code reviews and technical discussions
- Align with XSigmaAnalyitix's vision and business objectives
- Be nominated by an existing maintainer and approved by XSigmaAnalyitix leadership

**Note**: Maintainer status is a privilege, not a right, and can be revoked at any time by XSigmaAnalyitix.

### Contributors

**Contributors** are individuals who contribute to the project through:

- Code contributions (features, bug fixes, improvements)
- Documentation improvements
- Bug reports and feature requests
- Code reviews and feedback
- Community support and engagement

**Important**: By contributing, contributors:
- Grant XSigmaAnalyitix irrevocable rights to use their contributions under GPL-3.0-or-later and commercial licenses
- Accept that XSigmaAnalyitix has final authority over contribution acceptance
- Waive future claims to control how their contributions are used

All contributors must adhere to the [Code of Conduct](CODE_OF_CONDUCT.md) and [Contributing Guidelines](CONTRIBUTING.md).

### Users

**Users** are individuals or organizations who use XSigma in their projects. Users are encouraged to:

- Report bugs and issues
- Request features and enhancements
- Provide feedback on usability and documentation
- Share their use cases and success stories

## Decision-Making Process

### Authority Structure

**XSigmaAnalyitix maintainers have final authority** over all project decisions. While community input is valued and considered, **maintainers retain sole discretion** over:

- Contribution acceptance or rejection
- Feature prioritization and roadmap
- Architectural and design decisions
- Release timing and versioning
- Licensing and commercial terms
- Governance and policy changes

### Consensus-Based Decision Making (Among Maintainers)

XSigma maintainers use a **consensus-based decision-making model** for internal decisions:

1. **Proposal**: Anyone can propose changes via GitHub Issues or Pull Requests
2. **Community Discussion**: Community members discuss the proposal, providing feedback and suggestions
3. **Maintainer Review**: Maintainers review the proposal and community input
4. **Maintainer Consensus**: Maintainers work toward consensus among themselves
5. **Final Decision**: Maintainers make the final decision, which may accept, reject, or request revisions

**Important**: Community input is valued but **does not override maintainer authority**. Maintainers may accept or reject proposals regardless of community consensus.

### Types of Decisions

#### Minor Decisions

**Examples**: Bug fixes, documentation updates, minor refactoring

**Process**:
- Submit a pull request
- At least one maintainer review and approval required
- **Maintainer has sole discretion** to accept or reject
- Merge after approval and passing CI/CD checks

#### Major Decisions

**Examples**: New features, API changes, architectural changes, dependency additions

**Process**:
- Open a GitHub Issue for discussion before implementation
- Gather community feedback (minimum 7 days for significant changes)
- Maintainers review and discuss
- Consensus among maintainers sought (but not required)
- **Maintainers have final authority** regardless of community input
- Implementation via pull request with standard review process

#### Critical Decisions

**Examples**: License changes, governance changes, major breaking changes

**Process**:
- Open a GitHub Issue with detailed proposal
- Extended discussion period (minimum 14 days)
- Unanimous consensus among XSigmaAnalyitix maintainers required
- Community input strongly considered but not binding
- **XSigmaAnalyitix retains ultimate authority** over critical decisions
- Final decision documented in issue and announced to community

### Voting (When Maintainer Consensus Cannot Be Reached)

If consensus among maintainers cannot be reached after good-faith discussion:

1. **Maintainer Vote**: XSigmaAnalyitix maintainers vote on the proposal
2. **Majority Rule**: Simple majority (>50%) of maintainers required for approval
3. **Tie-Breaking**: In case of a tie, XSigmaAnalyitix leadership makes final decision
4. **Documentation**: Decision and rationale documented in the issue

**Note**: Community members do not vote on project decisions. Only XSigmaAnalyitix maintainers have voting rights.

## Contribution Process

### Contribution Acceptance Policy

**Important**: All contributions are subject to review and approval by XSigmaAnalyitix maintainers. By contributing, you:

- Grant XSigmaAnalyitix irrevocable rights to use your contribution under GPL-3.0-or-later and commercial licenses
- Accept that XSigmaAnalyitix has sole discretion over contribution acceptance
- Waive future claims to control how your contribution is used
- Acknowledge that submission does not guarantee acceptance

See [CONTRIBUTING.md](CONTRIBUTING.md) for complete details on contribution rights and licensing.

### Contribution Workflow

All contributions must follow the process outlined in [CONTRIBUTING.md](CONTRIBUTING.md):

1. **Fork and Clone**: Fork the repository and clone to your local machine
2. **Create Branch**: Create a feature branch for your changes
3. **Develop**: Make changes following coding standards
4. **Test**: Write tests and ensure ≥98% coverage
5. **Lint**: Run linters and formatters
6. **Commit**: Commit with clear, descriptive messages
7. **Push**: Push to your fork
8. **Pull Request**: Create a pull request with detailed description
9. **Review**: Address feedback from maintainers
10. **Approval**: Maintainer approves (or rejects) at their sole discretion
11. **Merge**: Maintainer merges after approval

### Contribution Standards

All contributions must meet the following standards:

- **Code Quality**: Adhere to [coding standards](.augment/rules/coding.md)
- **Testing**: Minimum 98% code coverage with comprehensive tests
- **Documentation**: Update relevant documentation
- **Security**: Follow [security policy](SECURITY.md)
- **Licensing**: Grant irrevocable license to XSigmaAnalyitix (GPL-3.0-or-later OR Commercial)
- **Alignment**: Align with project goals, architecture, and strategic direction

**Maintainers may reject contributions** that don't meet these standards or don't align with project goals, regardless of technical quality.

## Code Review Process

### Review Requirements

- **All changes** require code review before merging
- **Minimum one maintainer approval** required
- **All CI/CD checks** must pass
- **No unresolved review comments** at time of merge

### Review Criteria

Reviewers evaluate:

1. **Correctness**: Does the code work as intended?
2. **Quality**: Does it follow coding standards?
3. **Testing**: Are tests comprehensive and passing?
4. **Documentation**: Is documentation updated?
5. **Security**: Are there security implications?
6. **Performance**: Are there performance implications?
7. **Maintainability**: Is the code readable and maintainable?

### Review Timeline

- **Minor changes**: 1-3 business days
- **Major changes**: 3-7 business days
- **Critical changes**: 7-14 business days

Maintainers will make best efforts to review within these timelines, but complex changes may require additional time.

## Release Process

### Release Cadence

- **Major releases** (X.0.0): As needed for significant features or breaking changes
- **Minor releases** (X.Y.0): Monthly or as needed for new features
- **Patch releases** (X.Y.Z): As needed for bug fixes

### Release Process

1. **Version Update**: Update version in `CMakeLists.txt`
2. **Changelog**: Update `CHANGELOG.md` with release notes
3. **Testing**: Ensure all tests pass on all platforms
4. **Tag**: Create and push git tag (e.g., `v1.2.3`)
5. **GitHub Release**: Create GitHub Release with changelog excerpt
6. **Announcement**: Announce release to community

See [CONTRIBUTING.md - Release Process](CONTRIBUTING.md#release-process) for detailed steps.

### Release Approval

- **Patch releases**: Any maintainer can approve
- **Minor releases**: Consensus among maintainers
- **Major releases**: Unanimous consensus among maintainers

## Conflict Resolution

### Code of Conduct Violations

Violations of the [Code of Conduct](CODE_OF_CONDUCT.md) are handled according to the enforcement guidelines in that document:

1. **Report**: Report to [conduct@xsigma.co.uk](mailto:conduct@xsigma.co.uk)
2. **Investigation**: Maintainers investigate promptly and fairly
3. **Action**: Appropriate action taken based on severity (warning, temporary ban, permanent ban)
4. **Appeal**: Decisions may be appealed to maintainers

### Technical Disagreements

For technical disagreements that cannot be resolved through discussion:

1. **Good Faith Discussion**: Engage in respectful, good-faith discussion
2. **Seek Consensus**: Work toward consensus considering all perspectives
3. **Maintainer Decision**: If consensus cannot be reached, maintainers make final decision
4. **Document Rationale**: Decision and rationale documented for transparency

### Maintainer Conflicts

For conflicts between maintainers:

1. **Direct Communication**: Attempt to resolve directly and respectfully
2. **Mediation**: If needed, involve neutral third-party maintainer
3. **Community Input**: Consider community perspective
4. **Voting**: If necessary, use voting process (majority rule)

## Changes to Governance

### Proposing Changes

Anyone can propose changes to this governance document:

1. **Open Issue**: Create GitHub Issue with proposed changes and rationale
2. **Discussion**: Community discussion (minimum 14 days)
3. **Revision**: Revise proposal based on feedback
4. **Approval**: Unanimous consensus among maintainers required
5. **Implementation**: Update document and announce to community

### Amendment Process

- **Minor clarifications**: Single maintainer approval
- **Substantive changes**: Unanimous maintainer consensus
- **Major restructuring**: Extended discussion (30 days) + unanimous consensus

## Contact

- **General Inquiries**: [licensing@xsigma.co.uk](mailto:licensing@xsigma.co.uk)
- **Code of Conduct**: [conduct@xsigma.co.uk](mailto:conduct@xsigma.co.uk)
- **Security Issues**: [GitHub Security Advisories](https://github.com/KhwarizmiAnalytix/XSigma/security/advisories)
- **GitHub Issues**: [https://github.com/KhwarizmiAnalytix/XSigma/issues](https://github.com/KhwarizmiAnalytix/XSigma/issues)

## Acknowledgments

This governance model is inspired by best practices from successful open-source projects and the [OpenSSF Best Practices](https://www.bestpractices.dev/) guidelines.

---

**Document Version**: 1.0
**Last Updated**: 2025-11-02
**Maintained By**: XSigma Maintainers
