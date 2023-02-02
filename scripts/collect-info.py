ocid_list = list()
ip_list = list()

def deal_with(line):
    if line.find('href="https://cloud.oracle.com/compute/instances/') != -1:
        start = line.find('href="https://cloud.oracle.com/compute/instances/') + len('href="https://cloud.oracle.com/compute/instances/')
        ocid = line[start: start + 83]
        ocid_list.append(ocid)
    if line.find('<dkk>') != -1:
        start = line.find('<dkk>') + len('<dkk>')
        ip = line[start: line.find('</td>,')]
        ip_list.append(ip)

fo = open("./asburn-instance-info2")
while True:
    line = fo.readline()
    if not line:
        break
    deal_with(line)



fo = open("./asburn-instance-info1")

while True:
    line = fo.readline()
    if not line:
        break
    deal_with(line)

ocid_list.reverse()
ip_list.reverse()

print(len(ip_list))

fw = open("./start-stop-asburn.sh", "w+")
fw.write("#!/bin/bash\ninstances=(\n")
for ocid in ocid_list:
    ocid_str =  '   "' + ocid + '"\n'
    fw.write(ocid_str)

ocid_str = ")\n\n# action=START\naction=STOP\n# action=RESET\n\nfor i in \"${instances[@]}\"; do\n"  \
    + "    /home/ubuntu/bin/oci compute instance action --action $action --instance-id $i | jq -r '.data.\"display-name\"' &\n" \
    + "done\n"  \
    + "wait\n"

fw.write(ocid_str)

fw = open("./asburn-ip", "w+")
for ip in ip_list:
    ip_str = '"' + ip + '",\n'
    fw.write(ip_str)

