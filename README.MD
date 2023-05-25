# ASICAM v2
### Synchronize date over `ssh`
- Shell function:
```sh
function ssh-set-date { ssh "$@" sudo date -s @$(date -u +"%s") }
```
- Shell command:
```sh
ssh-set-date <remote server>
```

### Synchronize directory over `ssh`
```sh
rsync -rav /path/to/local remote_host:/path/to/remote
```
