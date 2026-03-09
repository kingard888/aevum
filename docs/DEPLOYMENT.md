# Deployment Guide

Deploy and run AevumDB in production environments.

## Pre-Deployment Checklist

### Hardware Requirements

**Minimum**
- 1 GB RAM
- 1 GB disk space for data
- Single core CPU

**Recommended**
- 4+ GB RAM
- 10+ GB disk space
- Multi-core CPU
- SSD storage

**Production**
- 8+ GB RAM
- 50+ GB disk space (depends on data)
- 4+ cores
- Enterprise SSD / NVMe
- Redundant storage

### Software Requirements

- **OS**: Linux 20.04+ (Ubuntu, Fedora, Debian, etc.)
- **Runtime**: None (static binary)
- **Dependencies**: Built into binary

## Building for Production

### Release Build

Optimized for performance and size:

```bash
mkdir -p build_prod
cd build_prod
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

Binaries: `build_prod/bin/aevumdb` and `build_prod/bin/aevumsh`

### Install to System

System-wide installation:

```bash
cd build_prod
sudo make install DESTDIR=/opt/aevumdb
```

Or to standard location:

```bash
cd build_prod
sudo make install  # Installs to /usr/local/bin/
```

## Running the Daemon

### Basic Start

```bash
# Start daemon (creates data directory automatically)
/opt/aevumdb/bin/aevumdb
```

### Custom Data Directory

Specify custom data location (positional argument):

```bash
# Use custom directory
/opt/aevumdb/bin/aevumdb /var/lib/aevumdb

# Custom directory with custom port
/opt/aevumdb/bin/aevumdb /data/aevumdb 55001
```

### Background Process

Start as background daemon:

```bash
/opt/aevumdb/bin/aevumdb > /var/log/aevumdb/daemon.log 2>&1 &
```

### Using Systemd (Recommended)

Create service file:

**`/etc/systemd/system/aevumdb.service`**
```ini
[Unit]
Description=AevumDB Database Server
Documentation=https://github.com/aevumdb/aevum
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
User=aevumdb
Group=aevumdb
WorkingDirectory=/var/lib/aevumdb
ExecStart=/opt/aevumdb/bin/aevumdb
Restart=on-failure
RestartSec=10

StandardOutput=journal
StandardError=journal

# Security
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=strict
ProtectHome=true
ReadWritePaths=/var/lib/aevumdb

[Install]
WantedBy=multi-user.target
```

Enable and start:

```bash
# Create service user
sudo useradd -r -s /bin/false aevumdb

# Enable service
sudo systemctl enable aevumdb

# Start service
sudo systemctl start aevumdb

# Check status
sudo systemctl status aevumdb

# View logs
sudo journalctl -u aevumdb -f
```

## Configuration

### Command-Line Options

```bash
aevumdb --help
```

**aevumdb** options:
- `DATA_PATH` - Database directory (default: `./aevum_data`)
- `PORT` - Listen port (default: `55001`)

**Examples:**
```bash
# Default (creates ./aevum_data)
/opt/aevumdb/bin/aevumdb

# Custom data directory
/opt/aevumdb/bin/aevumdb /var/lib/aevumdb

# Custom directory and port
/opt/aevumdb/bin/aevumdb /var/lib/aevumdb 55001
```

### Environment Variables

```bash
# Set data directory
export AEVUMDB_DATA_DIR=/var/lib/aevumdb

# Set port
export AEVUMDB_PORT=55001

# Start daemon
/opt/aevumdb/bin/aevumdb
```

### Network Configuration

#### Firewall (ufw on Ubuntu/Debian)

Allow connections on port 55001:

```bash
sudo ufw allow 55001/tcp
sudo ufw enable
sudo ufw status
```

#### Firewall (firewalld on Fedora/RHEL)

```bash
sudo firewall-cmd --permanent --add-port=55001/tcp
sudo firewall-cmd --reload
sudo firewall-cmd --list-ports
```

#### iptables (Manual)

```bash
sudo iptables -A INPUT -p tcp --dport 55001 -j ACCEPT
sudo iptables-save > /etc/iptables/rules.v4
```

#### Listen on Network Interface

To configure the port that aevumdb listens on:

```bash
/opt/aevumdb/bin/aevumdb /var/lib/aevumdb 55001
```

Note: aevumdb listens on 127.0.0.1 by default. To expose to network, use VPN or SSH tunneling.

## Storage Configuration

### Data Directory

AevumDB stores everything in the data directory:

```
data/
├── database.wt          # Main WiredTiger database
├── _auth.wt             # User/authentication data
└── ... (other metadata)
```

### Disk Space Planning

Estimate based on documents:

```
Average document size: ~1 KB
For 1 million documents: ~1 GB
With indexes: ~1.5 GB
With write buffer: ~2 GB total
```

### Performance Tuning

#### SSD Recommended
```bash
# Place data on SSD for better performance
/opt/aevumdb/bin/aevumdb /mnt/ssd/aevumdb
```

#### Multiple Disks
```bash
# Create symlink to another disk
ln -s /mnt/data1/aevumdb /var/lib/aevumdb
```

## Backup and Recovery

### Manual Backup

```bash
# Backup data directory
tar -czf aevumdb-backup-$(date +%Y%m%d).tar.gz /var/lib/aevumdb

# Backup to another location
cp -r /var/lib/aevumdb /backup/aevumdb-$(date +%Y%m%d)
```

### Automated Backup (Cron)

**`/etc/cron.daily/aevumdb-backup`**
```bash
#!/bin/bash
BACKUP_DIR="/backups/aevumdb"
mkdir -p $BACKUP_DIR
tar -czf $BACKUP_DIR/aevumdb-$(date +\%Y\%m\%d-\%H\%M\%S).tar.gz /var/lib/aevumdb
# Keep last 30 days
find $BACKUP_DIR -name "*.tar.gz" -mtime +30 -delete
```

Make executable:
```bash
sudo chmod +x /etc/cron.daily/aevumdb-backup
```

### Restore from Backup

Stop daemon:
```bash
sudo systemctl stop aevumdb
```

Restore:
```bash
sudo rm -rf /var/lib/aevumdb/*
sudo tar -xzf aevumdb-backup-20240101.tar.gz -C /
```

Start daemon:
```bash
sudo systemctl start aevumdb
```

### Write-Ahead Logging (WAL)

WiredTiger includes journal files for durability:

```bash
# Journal files in data directory
ls -l /var/lib/aevumdb/WiredTiger.log
```

Automatic recovery on restart if crash occurs.

## Monitoring

### Health Check

Test connectivity:

```bash
# Quick test
echo "db.health.count({})" | /opt/aevumdb/bin/aevumsh > /dev/null 2>&1 && echo "OK" || echo "FAIL"
```

### Process Monitoring

```bash
# Check if daemon is running
ps aux | grep aevumdb

# Check listening port
netstat -tlnp | grep 55001
# or
ss -tlnp | grep 55001

# Check memory usage
top -p $(pidof aevumdb)
```

### Log Monitoring

With systemd:
```bash
# Real-time logs
sudo journalctl -u aevumdb -f

# Recent logs
sudo journalctl -u aevumdb --since today

# Error logs
sudo journalctl -u aevumdb -p err
```

### Performance Monitoring

```bash
# Monitor daemon resource usage
watch -n 1 'ps aux | grep aevumdb'

# Check I/O
iostat -x 1

# Network connections
netstat -an | grep 55001
```

## Database Maintenance

### User Management

Create new user with API key:

```bash
/opt/aevumdb/bin/aevumsh << EOF
db.create_user("serviceuser", "READ_WRITE")
exit
EOF
```

### Periodic Tasks

**Weekly checks**:
- Verify backup completion
- Check disk space: `df -h /var/lib/aevumdb`
- Review logs for errors
- Count documents in database

**Monthly tasks**:
- Test restore procedure
- Review connection logs
- Update documentation
- Performance review

## Disaster Recovery

### Complete Data Loss

1. **Stop service**
   ```bash
   sudo systemctl stop aevumdb
   ```

2. **Restore from backup**
   ```bash
   sudo rm -rf /var/lib/aevumdb/*
   sudo tar -xzf latest-backup.tar.gz -C /
   sudo chown -R aevumdb:aevumdb /var/lib/aevumdb
   ```

3. **Start service**
   ```bash
   sudo systemctl start aevumdb
   ```

4. **Verify**
   ```bash
   sudo systemctl status aevumdb
   /opt/aevumdb/bin/aevumsh
   > db.health.count({})
   > exit
   ```

### Corrupted Data

If data is corrupted:

1. Stop daemon
2. Delete corrupted collection(s) from backup
3. Restore that part only
4. Restart daemon

## Performance Tuning

### Network Tuning

```bash
# Increase TCP backlog
sudo sysctl -w net.core.somaxconn=4096

# Increase file descriptors
sudo sysctl -w fs.file-max=2097152

# Permanent: add to /etc/sysctl.conf
net.core.somaxconn=4096
fs.file-max=2097152
```

### Disk Tuning

```bash
# Enable write-back caching
echo "writeback" | sudo tee /sys/block/sda/queue/write_cache
```

### Memory Tuning

Monitor memory usage and adjust system accordingly:

```bash
# Check available memory
free -h

# Monitor while running
watch -n 1 free -h
```

## Upgrades

### One-Time Upgrade

1. **Backup current data**
   ```bash
   cp -r /var/lib/aevumdb /backup/aevumdb-pre-upgrade
   ```

2. **Build new version**
   ```bash
   git pull
   ./scripts/build.sh rebuild
   ```

3. **Stop daemon**
   ```bash
   sudo systemctl stop aevumdb
   ```

4. **Replace binary**
   ```bash
   sudo cp build/bin/aevumdb /opt/aevumdb/bin/aevumdb.new
   sudo mv /opt/aevumdb/bin/aevumdb /opt/aevumdb/bin/aevumdb.old
   sudo mv /opt/aevumdb/bin/aevumdb.new /opt/aevumdb/bin/aevumdb
   ```

5. **Start daemon**
   ```bash
   sudo systemctl start aevumdb
   ```

6. **Verify**
   ```bash
   sudo systemctl status aevumdb
   ```

## Troubleshooting

### Daemon won't start

Check for error messages:
```bash
/opt/aevumdb/bin/aevumdb
```

Common issues:
- Port in use: Change port or kill process on 55001
- Permission denied: Check file ownership
- No space left: Clean up disk space

### High memory usage

Monitor process:
```bash
top -p $(pidof aevumdb)
```

Solutions:
- Reduce number of concurrent connections
- Increase swap (temporary)
- Increase RAM (permanent)

### Slow queries

Profile performance:
```bash
/opt/aevumdb/bin/aevumsh << EOF
db.large_collection.count({})
EOF
```

Solutions:
- Add indexes for frequently queried fields
- Archive old data
- Upgrade hardware

### Cannot connect from remote

Check firewall:
```bash
sudo ufw allow 55001/tcp
```

Check binding:
```bash
sudo ss -tlnp | grep 55001
```

Check network from client:
```bash
telnet <server-ip> 55001
```

## Security Hardening

### User Permissions

```bash
# Create dedicated user
sudo useradd -r -s /bin/false aevumdb

# Restrict data directory
sudo chmod 700 /var/lib/aevumdb
sudo chown aevumdb:aevumdb /var/lib/aevumdb
```

### API Keys

Create separate users for different applications:

```bash
# Read-only analytics user
db.create_user("analytics_ro", "READ_ONLY")

# Application write user
db.create_user("app_service", "READ_WRITE")

# Admin user
db.create_user("db_admin", "ADMIN")
```

### Network Security

- Don't expose publicly without VPN/firewall
- Use API keys for authentication
- Monitor authentication logs
- Regular security audits

## See Also

- [Building](BUILDING.md) - Build instructions
- [Getting Started](GETTING_STARTED.md) - Quick start
- [Troubleshooting](TROUBLESHOOTING.md) - Common issues
- [Architecture](ARCHITECTURE.md) - System design
- [Scripts](SCRIPTS.md) - Build automation scripts
- [Third-Party Libraries](THIRD_PARTY.md) - Dependencies
