# Security Policy

## Reporting a Vulnerability

If you discover a security vulnerability in AevumDB, please report it responsibly. Do **not** open a public issue or discussion about the vulnerability.

### How to Report

1. **Email**: Send a detailed report to security.aevumdb@gmail.com
2. **Include**:
   - Description of the vulnerability
   - Steps to reproduce
   - Potential impact
   - Suggested fix (if any)

3. **Response Time**: We aim to acknowledge vulnerability reports within 24 hours

### What to Expect

- We will investigate the reported vulnerability
- We will assess the severity and impact
- We will develop and test a fix
- We will prepare a security advisory
- We will request a responsible disclosure timeline
- We will credit you in the security advisory (unless you prefer anonymity)

## Security Guidelines for Users

### Best Practices

1. **Keep AevumDB Updated**: Always use the latest stable version
2. **Secure Deployment**: 
   - Run AevumDB in isolated environments
   - Use firewalls to restrict network access
   - Authenticate all client connections
   - Use encryption for data in transit (if available)
3. **Database Security**:
   - Implement proper access controls
   - Use strong authentication credentials
   - Monitor logs for suspicious activity
   - Regular backups of critical data

### Known Security Considerations

- AevumDB is an embedded database; network security depends on application-level implementation
- Always validate and sanitize external input
- Review security settings before production deployment

## Security Updates

Security vulnerability patches are released as soon as they are thoroughly tested. Critical patches are released outside of regular release schedules.

### Version Support

- Latest release: Security patches applied
- Previous stable release: Critical security patches only
- Older releases: No guaranteed security support

## Disclosure Timeline

We follow responsible disclosure practices:

1. Reporter notifies us of vulnerability
2. We acknowledge receipt within 24 hours
3. We assign a CVE (if applicable) within 5 days
4. We prepare a fix and test thoroughly
5. We release the patched version
6. We publish a security advisory

We request a 90-day responsible disclosure window before public details are released.

## Contact

For security-related questions, contact: security.aevumdb@gmail.com

For other inquiries, please use our standard issue tracker: https://github.com/aevumdb/aevum/issues
