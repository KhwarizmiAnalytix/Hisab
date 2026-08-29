# Security Policy

- [**Reporting a Vulnerability**](#reporting-a-vulnerability)
- [**Using XSigma Securely**](#using-xsigma-securely)
  - [Data Source Security](#data-source-security)
  - [Untrusted Data Inputs](#untrusted-data-inputs)
  - [Analysis Scripts and Notebooks](#analysis-scripts-and-notebooks)
  - [Data Privacy and Compliance](#data-privacy-and-compliance)
  - [API and Integration Security](#api-and-integration-security)
  - [Dependency Management](#dependency-management)
- [**CI/CD Security Principles**](#cicd-security-principles)

## Reporting Security Issues

If you believe you have found a security vulnerability in XSigma, we encourage you to report it immediately. We take all security reports seriously and will investigate them promptly.

### How to Report

**Please report security issues using one of the following methods:**

1. **GitHub Security Advisories (Preferred):** https://github.com/KhwarizmiAnalytix/XSigma/security/advisories/new
2. **Email:** security@xsigma.co.uk (for private disclosure)

### What to Include

When reporting a vulnerability, please include:

- **Description:** Clear description of the vulnerability
- **Impact:** Potential impact and severity assessment
- **Reproduction steps:** Detailed steps to reproduce the issue
- **Affected versions:** Which versions of XSigma are affected
- **Proof of concept:** Code, configuration, or commands demonstrating the vulnerability (if applicable)
- **Suggested fix:** If you have ideas for remediation (optional)

### Response Timeline

We are committed to addressing security vulnerabilities promptly:

- **Initial response:** Within **48 hours** of receiving your report
- **Severity assessment:** Within **5 business days** we will assess the severity and confirm whether it is a valid security issue
- **Critical vulnerabilities (CVSS ≥ 9.0):** Patched and released within **14 days** of confirmation
- **High severity vulnerabilities (CVSS 7.0-8.9):** Patched and released within **30 days** of confirmation
- **Medium severity vulnerabilities (CVSS 4.0-6.9):** Patched and released within **60 days** of confirmation
- **Low severity vulnerabilities (CVSS < 4.0):** Addressed in the next regular release cycle

### Disclosure Policy

All reports submitted through the security advisories mechanism will **either be made public or dismissed by the team within 90 days of submission**. If an advisory has been closed on the grounds that it is not a security issue, please feel free to create a [new issue](https://github.com/KhwarizmiAnalytix/XSigma/issues/new) as it may still be a valid concern for the project.

We follow **coordinated disclosure** principles:

- We will work with you to understand and validate the vulnerability
- We will keep you informed of our progress toward a fix
- We will credit you in the security advisory (unless you prefer to remain anonymous)
- We request that you do not publicly disclose the vulnerability until we have released a fix
- If we cannot fix the vulnerability within 90 days, we will publicly disclose it with appropriate warnings

### Supported Versions

We provide security updates for the following versions:

| Version | Supported          |
| ------- | ------------------ |
| 1.x.x   | :white_check_mark: |
| < 1.0   | :x:                |

We recommend always using the latest stable release to ensure you have the most recent security patches.

## Using XSigma Securely

XSigma is a data analytics platform that processes and analyzes data from various sources. Security should be a primary concern when handling data, especially sensitive or confidential information.

### Data Source Security

**Validate all data sources before processing.** When connecting to databases, APIs, or file systems:

- **Use secure connection protocols:** Always use encrypted connections (HTTPS, SSL/TLS) when connecting to remote data sources
- **Implement proper authentication:** Use strong authentication mechanisms (API keys, OAuth, certificates) rather than hardcoded credentials
- **Verify data source integrity:** Validate that data sources are from trusted providers and check for data integrity using checksums or signatures where available
- **Principle of least privilege:** Grant XSigma only the minimum permissions necessary to access required data

**Never commit credentials to version control.** Use environment variables, secret management systems (e.g., HashiCorp Vault, AWS Secrets Manager), or encrypted configuration files.

### Untrusted Data Inputs

**Treat all external data as potentially malicious.** When processing data from untrusted or unknown sources:

- **Input validation:** Implement strict validation rules for data types, formats, ranges, and schemas
- **Sanitization:** Clean and normalize data before processing to remove potentially harmful content
- **File type verification:** When accepting file uploads, verify file types by content (magic bytes) rather than extensions alone
- **Size limits:** Enforce reasonable limits on data size to prevent denial-of-service attacks
- **SQL injection prevention:** Use parameterized queries or ORMs; never concatenate user input into SQL statements
- **Code injection prevention:** Avoid using `eval()`, `exec()`, or similar dynamic code execution on untrusted input

**Sandboxing:** Consider running analysis on untrusted data in isolated environments (containers, virtual machines) to limit potential damage.

### Analysis Scripts and Notebooks

**XSigma analysis scripts and Jupyter notebooks are executable code** — treat them with the same security considerations as any software:

- **Code review:** Review all analysis scripts before execution, especially from external contributors
- **Dependency auditing:** Regularly audit and update dependencies for known vulnerabilities
- **Notebook security:** Be cautious with notebooks from untrusted sources; they can contain malicious code in cell outputs or hidden cells
- **Pickle files warning:** Avoid using Python's `pickle` module with untrusted data, as it can execute arbitrary code during deserialization. Prefer safer formats like JSON, CSV, or Parquet

**Prefer read-only operations** when exploring untrusted datasets to prevent unintended modifications or data exfiltration.

### Data Privacy and Compliance

**Handle sensitive data with special care.** If working with personally identifiable information (PII), financial data, or other regulated data:

- **Data minimization:** Only collect and process data that is necessary for your analysis
- **Anonymization and pseudonymization:** Remove or mask identifying information where possible
- **Encryption:** Encrypt sensitive data at rest and in transit
- **Access controls:** Implement role-based access control (RBAC) to limit who can access sensitive data
- **Audit logging:** Maintain logs of data access and modifications for compliance and security monitoring
- **Compliance:** Ensure your use of XSigma complies with relevant regulations (GDPR, HIPAA, CCPA, etc.)
- **Data retention:** Implement policies to securely delete data when it's no longer needed

**Be cautious about data leakage:**
- Avoid printing or logging sensitive information
- Be careful when sharing analysis results, visualizations, or notebooks that might contain sensitive data
- Review outputs before publishing to ensure no confidential information is exposed

### API and Integration Security

When integrating XSigma with external systems:

- **API authentication:** Use secure authentication methods (OAuth 2.0, JWT tokens) for API access
- **Rate limiting:** Implement rate limiting to prevent abuse and denial-of-service attacks
- **Input validation:** Validate all inputs to APIs and integrations
- **CORS policies:** Configure appropriate Cross-Origin Resource Sharing (CORS) policies
- **Webhook security:** Verify webhook signatures to ensure requests come from legitimate sources
- **API keys rotation:** Regularly rotate API keys and credentials

### Dependency Management

**Keep dependencies up to date:**

- Regularly update Python packages and other dependencies
- Use tools like `pip-audit`, `safety`, or `snyk` to scan for known vulnerabilities
- Pin dependency versions in production to ensure reproducibility while maintaining a process for updates
- Review dependency changes and their security implications before upgrading

**Use virtual environments:** Isolate project dependencies to prevent conflicts and limit the scope of potential compromises.

## CI/CD Security Principles

_Audience:_ Contributors, maintainers, and reviewers, especially those modifying workflows or build systems.

XSigma's CI/CD security is designed to balance open collaboration with security and integrity:

### General Principles

- **Code review requirement:** All changes must be reviewed by at least one maintainer before merging
- **Protected branches:** Main and release branches are protected and require status checks to pass
- **Signed commits:** Encourage or require GPG-signed commits for traceability and authenticity
- **Workflow approval:** Workflows from first-time contributors require approval before running

### Build and Test Security

- **Isolated test environments:** Run tests in isolated environments to prevent interference
- **Secret management:** Never hardcode secrets in code or configuration files
  - Use GitHub Secrets for CI/CD credentials
  - Secrets should only be accessible in protected branches and approved workflows
- **Dependency caching:** Use dependency caching carefully; verify cache integrity periodically
- **Artifact security:** Treat build artifacts with caution; do not execute untrusted artifacts in sensitive environments

### Release Pipeline Security

For official releases:

- **Ephemeral runners:** Build releases on ephemeral (single-use) runners to prevent contamination
- **Code signing:** Sign release artifacts (Python packages, containers) for verification
- **Multi-factor authentication:** Require MFA for accounts with release permissions
- **Release verification:** Verify builds are reproducible and match the source code
- **Deployment environments:** Use GitHub deployment environments with protection rules for publishing to package registries (PyPI, Docker Hub)
- **Manual approval gates:** Critical release steps require manual approval from project maintainers

### Monitoring and Response

- **Security scanning:** Automated security scans run on all pull requests (SAST, dependency scanning)
- **Audit logs:** Maintain logs of significant actions (releases, permission changes)
- **Incident response:** Have a documented process for responding to security incidents
- **Vulnerability disclosure:** Follow the 90-day disclosure timeline for reported vulnerabilities

### Contributor Guidelines

Contributors should:

- Never commit sensitive data, credentials, or API keys
- Report any security concerns immediately through proper channels
- Follow secure coding practices and input validation guidelines
- Keep dependencies minimal and justified
- Document security-relevant changes clearly

---

**Note:** Security is an ongoing process, not a one-time task. Regularly review and update security practices as the project evolves and new threats emerge.