# Deploying AevumDB

AevumDB can be deployed as a system-wide service, similar to `mongodb-community`, ensuring it starts automatically and remains manageable through standard system tools.

## System-Wide Installation

The recommended way to deploy AevumDB on a Linux system is using the provided installation script.

### Installation

```bash
# Clone and build (if not done)
git clone https://github.com/aevumdb/aevum.git
cd aevum

# Run the installation script with sudo
sudo ./scripts/install.sh
```

### What the Installation Does
1.  **Centralized Files**: Installs binaries to `/opt/aevumdb/bin`.
2.  **Global Commands**: Creates symbolic links in `/usr/local/bin/` so you can run `aevumdb` and `aevumsh` from anywhere.
3.  **Data & Logs**: Sets up `/opt/aevumdb/data` and `/opt/aevumdb/log`.
4.  **Configuration**: Creates a default configuration at `/etc/aevum/aevumdb.conf`.
5.  **Systemd Service**: Registers AevumDB as a system service (`aevumdb.service`).

## Managing the Service

Once installed, you can manage the AevumDB server using `systemctl`.

### Start the Service
```bash
sudo systemctl start aevumdb
```

### Enable Auto-Start on Boot
```bash
sudo systemctl enable aevumdb
```

### Check Service Status
```bash
systemctl status aevumdb
```

### Stop the Service
```bash
sudo systemctl stop aevumdb
```

### Restart the Service
```bash
sudo systemctl restart aevumdb
```

## Configuration

AevumDB uses a YAML-based configuration file located at `/etc/aevum/aevumdb.conf`.

### Default Configuration
```yaml
storage:
  dbPath: /opt/aevumdb/data
systemLog:
  destination: file
  path: /opt/aevumdb/log/aevumdb.log
net:
  port: 55001
  bindIp: 127.0.0.1
```

After modifying the configuration, you must restart the service:
```bash
sudo systemctl restart aevumdb
```

## Monitoring Logs

You can monitor the AevumDB logs in real-time:

```bash
# Using tail
tail -f /opt/aevumdb/log/aevumdb.log

# Using journalctl (if using systemd)
sudo journalctl -u aevumdb -f
```

## Security Recommendations

1.  **Dedicated User**: While the current installer uses root/777 for simplicity, in production, it is recommended to run AevumDB under a dedicated `aevum` user.
2.  **Firewall**: Ensure port `55001` is only accessible from trusted IP addresses if you change `bindIp` to something other than `127.0.0.1`.
3.  **Authentication**: Ensure authentication is enabled in your database configuration.

## Scaling and High Availability

*Information about replication and sharding will be added in future releases.*
