#!/bin/bash
instances=(
    #16c
    "ocid1.instance.oc1.us-sanjose-1.anzwuljr7deiexace4cfyf4sefayxd4hylcq57emeovmrf4ip35h5r6vhozq"
    "ocid1.instance.oc1.us-sanjose-1.anzwuljr7deiexacdqvygwdk5zot25u3kdnhidb3zkhek5bzqx2yu5s6in7q"
    "ocid1.instance.oc1.us-sanjose-1.anzwuljr7deiexac3uxamqobj7pgd3ugvq7upyle56ctum25s5fxbdoy7dea"
    "ocid1.instance.oc1.us-sanjose-1.anzwuljr7deiexackeqw7oatw6vvz4fsv7jnb44czhd36pqplmwpyhnpuqqa"
    "ocid1.instance.oc1.us-sanjose-1.anzwuljr7deiexacldaco6pcgtbsdm5m6a7443zvrwcrmzx66v5sicusqjjq"
    "ocid1.instance.oc1.us-sanjose-1.anzwuljr7deiexacnzipmuu24yzghr4jtpgtmy6h73m2udhdfgfawi4ez5jq"
    "ocid1.instance.oc1.us-sanjose-1.anzwuljr7deiexacx256mtw3yt3hludlxwfo2pmgai6i7xhd2eu4kx44eidq"
    "ocid1.instance.oc1.us-sanjose-1.anzwuljr7deiexac2t3qqwedzt3crrdeiraqf2i2jc5pvg4rnkb25qijunfa"
    "ocid1.instance.oc1.us-sanjose-1.anzwuljr7deiexacdirviqhztp6bleofdqitvzpzm23r6okbzzgsycqxnmya"
    "ocid1.instance.oc1.us-sanjose-1.anzwuljr7deiexacueujobwk5tdpis5l6ysuindi34l7xuc7xqkymluagbiq"
    "ocid1.instance.oc1.us-sanjose-1.anzwuljr7deiexacf45audghg42n5zj7inhb4re75l4mgpmattyh6zgwrjba"
    "ocid1.instance.oc1.us-sanjose-1.anzwuljr7deiexac45vqacsrqfq65e5nquu6vrzuwnu4ifwmqpkaldq2qrla"
    "ocid1.instance.oc1.us-sanjose-1.anzwuljr7deiexac375mbh7qbsvksquo4vurw7ppi726m4etc7hnseh2m5oq"
    "ocid1.instance.oc1.us-sanjose-1.anzwuljr7deiexac2ug6k3vtdjejwa7bfsot427u7e4x3u5agxpfedxykq4a"
    "ocid1.instance.oc1.us-sanjose-1.anzwuljr7deiexac5vhpbxfq46nhdbu347vbt6kay6wawwetmtbyihktbcpq"
    "ocid1.instance.oc1.us-sanjose-1.anzwuljr7deiexaczkp32t2vmr7lzotboosvr3s5eexkrhbha4yzbybpan3a"
    #4c
    "ocid1.instance.oc1.us-sanjose-1.anzwuljr7deiexac2ckqervo3w7ucxrrxe7v3ahud2s3y75xaccsaf4outfa"
    "ocid1.instance.oc1.us-sanjose-1.anzwuljr7deiexachmamyhuu3iw4fekhwn4hxgyiews4ej2qiaj4uluvzbua"
    "ocid1.instance.oc1.us-sanjose-1.anzwuljr7deiexacdyz7bnkuvkqvk5sudgfmagvackk7iyywtj4hqdewndsq"
    "ocid1.instance.oc1.us-sanjose-1.anzwuljr7deiexacbgqx6qw2qlo6fv5ezu6qjibwkizj4txxixcfx5kl3c6a"
    "ocid1.instance.oc1.us-sanjose-1.anzwuljr7deiexacereahp6bitp6trcwqaqqm2xahscunfk4nvasje5ceepa"
    "ocid1.instance.oc1.us-sanjose-1.anzwuljr7deiexacnkige7pzaa52mg2luov6y7mc3thx5fmguvykwfimloea"
    "ocid1.instance.oc1.us-sanjose-1.anzwuljr7deiexac7ovauev44me5jilx24am32kn7lbfp2aqm4s6bavwo23a"
    "ocid1.instance.oc1.us-sanjose-1.anzwuljr7deiexacrre4lah2tu3bdhvd27e4vg3zuolpvqbix2dbby6cjhdq"
    "ocid1.instance.oc1.us-sanjose-1.anzwuljr7deiexacekzoqp4cvhwfvrirugvkobf7bb2jou6vkdjpaybscf4a"
    "ocid1.instance.oc1.us-sanjose-1.anzwuljr7deiexac4brz2et5uokfytfr5dcokg34kxwelvu4olq4orphr73a"
    "ocid1.instance.oc1.us-sanjose-1.anzwuljr7deiexacmtrv2mr434luqgcphknf36j7ozlcteoicay2z6wrw6mq"
    "ocid1.instance.oc1.us-sanjose-1.anzwuljr7deiexaczrxolfcgnnhmng4xhn4jkkjbzy35oga327bzmg6rudja"
    "ocid1.instance.oc1.us-sanjose-1.anzwuljr7deiexace427lceskq5o3fofu2qahnxazcpoo3vrqmk3njg2lm4a"
    "ocid1.instance.oc1.us-sanjose-1.anzwuljr7deiexacin7a7zni2wabro5ezycu4v6drdcdemvsgzbvreyn722a"
    "ocid1.instance.oc1.us-sanjose-1.anzwuljr7deiexacde53kcdrpyqqg4vfl2kn272fi3hoq55sx3yw74groy3a"
    "ocid1.instance.oc1.us-sanjose-1.anzwuljr7deiexackzkkpqfbz5wwywl7zvfd4gwh3kwls3wifcptnlaaoekq"
)

# action=START
action=STOP
# action=RESET

for i in "${instances[@]}";
do
    /home/ubuntu/bin/oci compute instance action --action $action --instance-id $i | jq -r '.data."display-name"' &
    # /home/ubuntu/bin/oci compute instance action --action $action --instance-id $i &
    # ~/bin/oci compute instance action --action $action --instance-id $i &
done
wait