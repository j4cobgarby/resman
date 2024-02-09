Things that would be nice to do:

 - An option to put yourself in a queue to reserve the server, in the case that it's locked and several people want to use it next.
 - Integrate it into Linux:
   - ~~A warning message at user login, that lets them know if someone is already running an experiment.~~
   - An optional bash prompt modification that adds a little (!) or something if an experiment is running.
   - Use of `wall` or similar (maybe there's something a bit more modern) that tells everyone on the server when someone begins an experiment (though careful, this might get annoying, maybe there's a better way)

 - Look into cgroups to impose actual resource locking constraints. Chatgpt says we can do something like this:
```bash
# Create a named cgroup
cgcreate -g cpuset:resman_cgroup

# Set CPU affinity for that group (this particular example would use cpus 0 to 10
cgset -r cpuset.cpus=0-10 resman_cgroup

# Run a process within this cgroup
cgexec -g cpuset:resman_cgroup <some command>
```
