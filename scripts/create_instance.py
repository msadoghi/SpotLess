from configparser import Error
from logging import error
import oci
import sys
import time

# based on https://docs.oracle.com/en-us/iaas/tools/python-sdk-examples/2.45.1/core/launch_instance.py.html

ashburn = {
    "name": "us-ashburn-1",
    "availability_domain": "gDlt:US-ASHBURN-AD-3",
    "fault_domain": "FAULT-DOMAIN-3",
    "subnet_id": "ocid1.subnet.oc1.iad.aaaaaaaa6wr2cgbaztsxdr55t47elhbbeunjbynweigkt6ijbzdxzci3risq",
    "image_id": "ocid1.image.oc1.iad.aaaaaaaasixelyn4rg3kvnghpfz7jtits7rzsv6phmns3ch3ckju4ih6wjea",
}

region = ashburn
COMPARTMENT_ID = "ocid1.tenancy.oc1..aaaaaaaak5urycwdhjmdgdkfgu3q7wzamv63w6qa6pf4o5ry2dulte6aos4q"
SHAPE = "VM.Standard.E4.Flex"
OCPUS = 8
MEMORY = 16

dakai_image_id = "ocid1.image.oc1.iad.aaaaaaaamzy6zd3ttakwalewlqc4caezg2hgukn5ol2ou5x2k637c47z7e5q"
region['image_id'] = dakai_image_id

# Using the default profile from a different file
config = oci.config.from_file(file_location='~/.oci/config')

# Initialize service client
core_client = oci.core.ComputeClient(config)
core_client.base_client.set_region(region['name'])



VNIC = oci.core.models.CreateVnicDetails(
    assign_public_ip=True,
    assign_private_dns_record=True,
    subnet_id=region['subnet_id'])
# vlan_id="ocid1.test.oc1..<unique_ID>EXAMPLE-vlanId-Value");


def create_machine(name):
    launch_instance_response = core_client.launch_instance(
        launch_instance_details=oci.core.models.LaunchInstanceDetails(
            availability_domain=region['availability_domain'],
            # fault_domain=region['fault_domain'],
            compartment_id=COMPARTMENT_ID,
            shape="VM.Standard.E4.Flex",
            display_name=name,
            image_id=region['image_id'],
            shape_config=oci.core.models.LaunchInstanceShapeConfigDetails(ocpus=OCPUS, memory_in_gbs=MEMORY),
            create_vnic_details=VNIC))

    # Get the data from response
    print(launch_instance_response.data.display_name)


OCPUS = 8
MEMORY = 16

name = "dakai-"
start = int(sys.argv[1])
end = 160

for i in range(start, end):
    try:
        if i < 10:
            i = '00' + str(i)
        elif i < 100:
            i = '0' + str(i)
        else:
            i = str(i)
        create_machine(name + i)
        # print(name + i)
    except oci.exceptions.ServiceError as err:
        print(err)
        print(i)
        break