# Macvlan Global Driver #

### Setup

- The same functionality is preserved as in the instructions in the experimental [Macvlan README](https://github.com/docker/docker/blob/master/experimental/vlan-networks.md). To run the driver locally dockerd daemon label is used for local driver registration like so: `dockerd --label com.docker.network.driver.macvlan=local`

- By running the drivers globally it enables the user to have globally distributed IPAM and DNS support via the libnetwork IPAM/DNS excellence.

- To run the PoC binary download this tar and extract to the `/usr/bin/` directory from here: [dockerd-binaries](https://www.dropbox.com/s/gl4z6vzvw2pp63j/dockerd-macvlan.zip?dl=0) *Note:* (backup your Docker binaries prior if you dont want them overwritten by the extracted binaries).

- PR at: [nerdalert/libnetwork](https://github.com/docker/libnetwork/pull/1196) 

- Since the parent interfaces are global and are easier to work with if commonly named across the docker hosts, see the last section in this doc for reverting back to `eth0` `eth1` naming: [Rename your interfaces to a common name] (https://gist.github.com/nerdalert/847d7dba1699956bc700543f6f754c6b#rename-your-interfaces-to-a-common-name-like-eth0--eth1)

- Start a Docker instance and record the IP address of the host to be used when starting the next Docker instance below:

```
dockerd -D -H tcp://0.0.0.0:2376 \
    --cluster-store consul://0.0.0.0:8500 \
    --cluster-advertise eth1:2376 \
    -H unix:///var/run/docker.sock \
```


- Start a Consul instance with the following (Note: Consul uses port 8500. You can also start it with port-mappings if you replace `--net=host` with `-p "8500:8500"`. The following will keep the container address the same and avoid restart issues):

```
docker run -d \
  --net=host \
  -h "consul" \ 
  progrium/consul \
  -server \
  -bootstrap
```

- Start a second Docker instace and fill in the IP address of the first instance you started above in the option `consul://<IP_of_Swarm_Manager>:8500`:

```
dockerd -D -H tcp://0.0.0.0:2376 \
    --cluster-store consul://<IP_of_Swarm_Manager>:8500 \
    --cluster-advertise eth1:2376 \
    -H unix:///var/run/docker.sock \
    --label com.docker.network.driver.macvlan=global
 ```

- Once the endpoints join the Consul cluster should see messages like so:

```
DEBU[0998] Watch triggered with 2 nodes                  discovery=consul
DEBU[1001] Watch triggered with 2 nodes                  discovery=consul
DEBU[1018] Watch triggered with 2 nodes                  discovery=consul
DEBU[1019] 2016/05/17 02:23:59 [DEBUG] memberlist: Initiating push/pull sync with: <IP_of_Swarm_Manager>:7946
```

# Example Global or Local Macvlan Usages 

**Note:** all of the following examples compatable with both globally or locally scoped driver modes differentiated by the Docker daemon runtime label of `--label com.docker.network.driver.macvlan=global` exemplified in the previous example. 

### Single Parent Network ###

```
docker network create -d macvlan   \
 --subnet=192.168.1.0/24   \
--gateway=192.168.1.254 \
  -o parent=eth1  \
 -o macvlan_mode=bridge mac

docker run --net=mac --ip=192.168.1.10 -it --rm alpine /bin/sh 
```

### Macvlan Dual IPv4/Ipv6 Stack + Multi-Network ###

```
docker network create -d macvlan  \
  --ipv6 \
  --subnet=192.168.222.0/24 --subnet=192.168.224.0/24  \
  --subnet=fd18::/64  \
  --aux-address="exclude1=fd18::2" \
  --aux-address="exclude2=192.168.222.2" \
  --aux-address="exclude3=192.168.224.2" \
  --ip-range=192.168.222.0/25 \
  --ip-range=192.168.224.0/25 \
  -o parent="bond0,  eth0,  eth1" \
  -o macvlan_mode=bridge dualstack

# Start containers on the network
docker run --net=dualstack -itd alpine /bin/sh
docker run --net=dualstack --ip=192.168.222.10 --ip6=fd18::10 -itd alpine /bin/sh
docker run --net=dualstack --ip=192.168.222.20 --ip6=fd18::20 -itd alpine /bin/sh
docker run --net=dualstack --ip=192.168.222.21 --ip6=fd18::21 -it --rm alpine ping6 -c4 fd18::20
docker run --net=dualstack --ip=192.168.222.21 --ip6=fd18::21 -it --rm alpine ping -c4 192.168.222.20
docker run --net=dualstack --ip6=fd18::11 -itd alpine /bin/sh
docker run --net=dualstack --ip6=fd18::10 -it --rm alpine /bin/sh
```

### 802.1Q Trunk Examples ###

Users can specify either a single interface or a list of interfaces (new change to the driver) so that different interfaces can be used on different hosts if interface names are not consistent across hosts. 

- Trunk Example #1 - Single parent example:

```
docker network create -d macvlan  \
  --subnet=192.168.65.0/24 \
  --gateway=192.168.65.254 \
  -o parent=eth0.65  \
  -o macvlan_mode=bridge vlan65

docker run --net=vlan65 --ip=192.168.65.10 -it --rm alpine /bin/sh 
```

- 802.1q Trunks and sub-interfaces can also be in a parent list (`-o parent="bond0.65, eth3.65, eth1.65, eth0.65"`). The order of preference is based on the master interface of the specified sub-interface. In the case of `-o parent="bond0.65, eth3.65, eth1.65, eth0.65"` the first usable interface would be `eth0.65` if `bond0` does not exist, and `eth0` does exist. It is simply a first match ordered list. The sub-interface of `eth0.65` will be matched and created as long as the master portion of the sub-interface exists (`master.slave` where `eth0` represents the `master` and `.65` represents the `.slave` and VLAN ID `65` of the sub-interface). That sub-interface that maps to a tagged VLAN on the physical network is used as the parent interface `-o parent` in the Docker network.

- Trunk Example #2 - list of parents example:

```
docker network create -d macvlan  \
  --subnet=192.168.65.0/24 \
  --gateway=192.168.65.254 \
  -o =bond0.65,eth3.65,eth1.65,eth0.65  \
  -o macvlan_mode=bridge vlan65

docker run --net=vlan65 --ip=192.168.65.10 -it --rm alpine /bin/sh 
```


### More on Parent Interfaces

Create two different named interfaces and create a network that includes the two names.

- Host #1 

```
ip link add foo type dummy
ip link set foo up

docker network create -d macvlan   --subnet=192.168.20.0/24  --gateway=192.168.20.1  -o parent="toothbrush, foo.10, bar.10"   -o macvlan_mode=bridge mcv20
docker run --net=mcv20  -it --rm alpine /bin/sh
```

Host #2

```
ip link add bar type dummy
ip link set bar up

docker run --net=mcv20  -it --rm alpine /bin/sh
```

The dummy will get a subinterface created tagged with vlan `10`

```
$ ip -d link show foo
140: foo: <BROADCAST,NOARP,UP,LOWER_UP> mtu 1500 qdisc noqueue state UNKNOWN mode DEFAULT group default qlen 1000
    link/ether de:86:3d:02:56:d1 brd ff:ff:ff:ff:ff:ff promiscuity 1
    dummy addrgenmode eui64
    
$ ip -d link show foo.10
142: foo.10@foo: <BROADCAST,NOARP,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP mode DEFAULT group default
    link/ether de:86:3d:02:56:d1 brd ff:ff:ff:ff:ff:ff promiscuity 1
    vlan protocol 802.1Q id 10 <REORDER_HDR> addrgenmode eui64
```

On host #2 there will now be a link of `bar.10` as well as the dummy type parent of `bar`

```
$ ip -d link  show bar
519: bar: <BROADCAST,NOARP,UP,LOWER_UP> mtu 1500 qdisc noqueue state UNKNOWN mode DEFAULT group default qlen 1000
    link/ether 42:53:1f:e7:e5:fd brd ff:ff:ff:ff:ff:ff promiscuity 1
    dummy

$ ip -d link  show bar.10
521: bar.10@bar: <BROADCAST,NOARP,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP mode DEFAULT group default
    link/ether 42:53:1f:e7:e5:fd brd ff:ff:ff:ff:ff:ff promiscuity 1
    vlan protocol 802.1Q id 10 <REORDER_HDR>
```

### Parent Interface Configuration File

Since some users may not want to ensure common naming across all of their Docker hosts, there is also an option to specify a simple configuration file that contains the name of the parent interface to use for that specific host. This is applicable for *both* local and global driver modes.

- If the driver instance is being run in a globally scoped fashion, across multiple nodes with Swarm, each file on each Docker host can use different interfaces on each host.

 - **Note:** The **filepath** and **file name** for the `-o parent-file=` can be any location and filename as long as the daemon has read permissions to the specified filesystem. The two requirements are, the interface in the config file exists on the Docker host and the file path and file name must be located in the **same path** and have the **same file name** on each host as specified in the `network create`.

- *Config file Example #1* - The following example creates a network named `net1` and specifies a file with the option `-o parent-file=/<path_to_file>/<filename>` which contains the parent interface name `parent = "eth1"` in the config file `net1.toml`. The first line in the file that contains text will be used. 

```
cat /root/net1.toml
[macvlan]
version = "0.1"
parent = "eth1"
```



```
$ docker network create -d macvlan \
    --subnet=192.168.1.0/24 \
    --gateway=192.168.1.1 \
    -o parent-file=/root/net1.toml mcv0

$ docker run --net=mcv0 -itd alpine /bin/sh
```
On a second host in the cluster, another host may want to 

- *Config file Example #2* - Using a file to specify the parent interface has the flexibility for creating 802.1q sub-interfaces as using`-o parent=`. You just need to ensure the master interface exists. In the example of `VLAN 10` the interface in the file would be `eth0.10`. In that example, `eth0` has to exist on the host and the driver will create the slave `eth0.10`.


```
$ echo eth0.30 > /root/vlan30.conf 
$ docker network create -d macvlan \
    --subnet=172.16.86.0/24 \
    --gateway=172.16.86.2 \
    -o parent-file=/root/vlan30.conf  vlan30

$ docker run --net=mcv30 -it --rm alpine /bin/sh
```

### Rename your interfaces to a common name like eth0 / eth1

If you want to have a common interface name across the hosts and do not want `eno1234567` you can revert to `eth0`, `eth1` style naming with the following:

- Edit your /etc/default/grub. Change the line from:

```
GRUB_CMDLINE_LINUX=""
```

to

```
GRUB_CMDLINE_LINUX="net.ifnames=0 biosdevname=0"
```

next run:

```
update-grub
```

lastly reboot.


### Tests for parent lists w/various whitespaces and null commas

-  **Test #1** 

```
docker network create -d macvlan   \
    --subnet=192.168.1.0/24 --subnet=192.168.10.0/24 \
    --gateway=192.168.1.1 --gateway=192.168.10.1 \ 
    -o parent=eth0  \
    -o parent="bond0,  eth0,  eth1"  \
    -o macvlan_mode=bridge mac1

docker run --net=mac1 --ip=192.168.1.10 -it --rm alpine /bin/sh 
```

-  **Test #2** (no parentheses and extra comma)

```
docker network create -d macvlan \
    --subnet=192.168.1.0/24 \
    --gateway=192.168.1.254 \
    -o parent=bond0,eth1,eth0, \
    -o macvlan_mode=bridge mac2
    
    docker run --net=mac2 --ip=192.168.1.10 -it --rm alpine /bin/sh 
```

-  **Test #3** (extra whitespaces)

```
docker network create -d macvlan   \
    --subnet=192.168.1.0/24 --subnet=192.168.10.0/24 \
    --gateway=192.168.1.1 --gateway=192.168.10.1 \
    -o parent="bond0,  eth1,  eth0," \
    -o macvlan_mode=bridge mac3

docker run --net=mac3 --ip=192.168.1.10 -it --rm alpine /bin/sh 
```

-  **Test #4** (single parent)

```
docker network create -d macvlan \
    --subnet=192.168.1.0/24 --subnet=192.168.10.0/24 \
    --gateway=192.168.1.1 --gateway=192.168.10.1 \
    -o parent=eth0 \
    -o macvlan_mode=bridge mac4

docker run --net=mac4 --ip=192.168.1.10 -it --rm alpine /bin/sh 
```

-  **Test #5** (mixed invalid interface w/ valid interfaces)

```
docker network create -d macvlan \
    --subnet=192.168.1.0/24 --subnet=192.168.10.0/24 \
    --gateway=192.168.1.1 --gateway=192.168.10.1 \
    -o parent="toothbrush, eth1, eth0" \
    -o macvlan_mode=bridge mac5

docker run --net=mac5 --ip=192.168.1.10 -it --rm alpine /bin/sh 
```

-  **Test #6** (create a dummy interface on one host to check multiple interface names function)

- Host #1

```
docker network create -d macvlan \
    --ipv6 \
    --subnet=fd1a::/64 \
    -o parent=foo,eth1,pancakes \
    -o macvlan_mode=bridge mac6
```

- Host #2

```
ip link add foo type dummy
ip link set foo up
docker run --net=mac6 --ip6=fd1a::10 -it --rm alpine /bin/sh 
```

-  **Test #7** 

- Create a dummy interface `foo` on Host #2 to check multiple interface names functionality.
- Pass a subinterface to the dummy interface on Host #2 `foo.10` and Host #1 `eth1.20` on the host that does not have a host named `foo`.

- Host #1

```
docker network create -d macvlan \
    --ipv6 \
    --subnet=fd1a::/64 \
    -o parent=foo.10,eth1.20,pancakes \
    -o macvlan_mode=bridge mac7
```

- Host #2

```
ip link add foo type dummy
ip link set foo up
docker run --net=mac7 --ip6=fd1a::10 -it --rm alpine /bin/sh 
```

- Destroy the container and cleanup the dummy type link:

```
ip link del foo
```

-  **Test #8** (null parent - this should fail if in Global scope, it will succeed if scoped locally by the driver creating a dummy interface)

```
docker network create -d macvlan \
    --subnet=192.168.100.0/24 --subnet=192.168.110.0/24 \
    --gateway=192.168.100.1 --gateway=192.168.110.1 \
    -o macvlan_mode=bridge mac8

Error msg: Error response from daemon: no parent interface was passed, use for example '-o parent="eth0,eth1,eth3"
```
