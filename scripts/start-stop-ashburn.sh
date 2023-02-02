#!/bin/bash
instances=(
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacjtwa5axie5ghgjzcgsvoz2uubnyvdv64bscbf2qbt6pq"   #phnx_16core_000
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacvdiprpgiixnjfxuvsqkqearkylag5htnnnhzpt7mcnkq"   #phnx_16core_001
  # "ocid1.instance.oc1.phx.anyhqljs7deiexaczdv4qoqc3hqs4z35lz5jfxbq7qil4vap3n27ltz2oykq"   #phnx_16core_002
  # "ocid1.instance.oc1.phx.anyhqljs7deiexaccgkn3axgeewqcivgcgvv47zfojlsp4uunjxkn3dcgzxa"   #phnx_16core_003
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacjgvwm2pansrpaacn7e6mod5oaq7gbqc5lewdqhe4nwvq"   #phnx_16core_004
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacomun3p7pwuux4edawxk77stvnuu7xow24xmfizzjg3rq"   #phnx_16core_005
  # "ocid1.instance.oc1.phx.anyhqljs7deiexaczzenvon7rk4dcjxmjgmmakthg2dvhg3idalifubvs6mq"   #phnx_16core_006
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacvlhadut2hyv5hazjtcuyc5c7sqrz3keagzivraeeyooa"   #phnx_16core_007
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacrfqkq4ket32d7q5algm55l5ecyw4qbdcgo67deoeczkq"   #phnx_16core_008
  # "ocid1.instance.oc1.phx.anyhqljs7deiexaca6e724qqxmw6iscavf5fbshi3xgz5bsznqstjmw3ioea"   #phnx_16core_009
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacwmkv52czobkegq7hefassgh4aaabz7shiqgkmxkbuzjq"   #phnx_16core_010
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacz3okcqofdegmon5zhi6z5jtnj47w3jpkwng7e4o423gq"   #phnx_16core_011
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacf5lteji4mlqeufklu5g4wm5zu6gbwoka23bfmetucw3a"   #phnx_16core_012
  # "ocid1.instance.oc1.phx.anyhqljs7deiexachgxfl77gj4odmgsqv4h5tcaf72ax7sw4r2bbwtwbel5q"   #phnx_16core_013
  # "ocid1.instance.oc1.phx.anyhqljs7deiexach56avjwhh2pfvy7tmddh5wgwdlttiknt72mhhhp7277a"   #phnx_16core_014
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacotrm4wcuvh3ofpn5fxx5qosrmoii4pibzxp3aa56cs7a"   #phnx_16core_015
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacrhd6mvafavi7rdfsp7nt46pd3hnqw3pigu7qdu7gwdeq"   #phnx_16core_016
  # "ocid1.instance.oc1.phx.anyhqljs7deiexaccwgsctqsv7rznhy4p7o3qqngxiuocl6jwoydeb3zvdsq"   #phnx_16core_017
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacm7bkux7ytp53envct2ihg6m2gwnmtdg2zhwuklgzupdq"   #phnx_16core_018
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacym5oryjcgcrozim4j3hdbok5ty4rbbh76znjnrdfoezq"   #phnx_16core_019
  # "ocid1.instance.oc1.phx.anyhqljs7deiexac7m3ihf2kaee2z7l34xnypv4lxzfgazdv7sav7qc6inta"   #phnx_16core_020
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacdsxyenmss3v6samxrbbfqnv5kdiqlaaiqldndqlty32q"   #phnx_16core_021
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacizz32z6xcjil6psb2pkxpayumz6p2alukold5cwus2wq"   #phnx_16core_022
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacha4k3jft7cvqp2ebsxcgipn6f2i7gxtfaweq7eyypcfq"   #phnx_16core_023
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacmewgakr3xommsq7bteqhcqfrildbwci3jpjk5r7hp64q"   #phnx_16core_024
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacstyf6ulkmewmy7pxg7sbnchpsckfqfog7npivdkvmgda"   #phnx_16core_025
  # "ocid1.instance.oc1.phx.anyhqljs7deiexac76j7bdv22s4iyhaggzqmvfwcmpegedszkttvqzkbojka"   #phnx_16core_026
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacxx7b3ap3f5zghsntte26jceqpzdkrrquysjwcjw2ifpa"   #phnx_16core_027
  # "ocid1.instance.oc1.phx.anyhqljs7deiexactqnp5oyh4ee6yftskabmgawttzkl5tiv7cch33732erq"   #phnx_16core_028
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacoaxxcqf2oia4yspz73uwuyxskqqbegr5k6dprhqwz4yq"   #phnx_16core_029
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacqyzd5luk3vulzg7lzyjobbihi4eder7qpceey6bx44yq"   #phnx_16core_030
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacnqxsnblcn6cly3sasmiw55xbewpuk5pudot4q5amneda"   #phnx_16core_031
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacq6ljieyvzhtyqpzsexj7cx4bttlkkjd6yivb3reytgdq"   #phnx_16core_032
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacljsgkxidw3db7gjbnwivdlnuufuzzvqigspk56jk2omq"   #phnx_16core_033
  # "ocid1.instance.oc1.phx.anyhqljs7deiexaceilh2n77jsmehqls2hx6cqponjvk42ipbmufjw42pguq"   #phnx_16core_034
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacesyjjaassxnkiuozbbtdzyuzawuqcuv5ehfjgahqyboq"   #phnx_16core_035
  # "ocid1.instance.oc1.phx.anyhqljs7deiexaczxkjbd6zmgab4ivnuyxenlhuablnwv3sfgzgh2mvcczq"   #phnx_16core_036
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacemacuaepoi5g6rltxrwu37bbqmxykqopjnnftby5om5a"   #phnx_16core_037
  # "ocid1.instance.oc1.phx.anyhqljs7deiexackjqpnqox7uzsgq7iomwnct2zdqkid56baiqmiz7s3rkq"   #phnx_16core_038
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacsvx2m2n4lvursgwnqyxbnv243kfww5lfdgh7h5pqzoua"   #phnx_16core_039
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacpdcxppq6caob5bq3amiskmv2dqvndi3uovrtkepicana"   #phnx_16core_040
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacngbkn4nnbtzv5527d5nb4w3gn77hgh5wpfkmlal2ou3a"   #phnx_16core_041
  # "ocid1.instance.oc1.phx.anyhqljs7deiexactqguxorz56pbth56rk3ue5m4xh4qseff24kwv66arkza"   #phnx_16core_042
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacetaez7sbmxugvyvedgfmpbappxz4wlcww4mctkv6n2hq"   #phnx_16core_043
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacm5jldwcobazk2jhwdbdnaxjkkagqyygiy6ixakwhdztq"   #phnx_16core_044
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacvoavzii3eepeg4dkh4iumkjopdf6cxzt424k3yhl6lhq"   #phnx_16core_045
  # "ocid1.instance.oc1.phx.anyhqljs7deiexac7uvoxy5rshewmgbv4xdmgjwcqqfoiblgzsgfnvhsbvtq"   #phnx_16core_046
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacygmug3sw36phmvl3mdbc2or3nkfbo3euvcmit426jbcq"   #phnx_16core_047
  # "ocid1.instance.oc1.phx.anyhqljs7deiexac7vdnvfddi7tidmdncwnhudvnjqflijqsf3tgocqtsjra"   #phnx_16core_048
  # "ocid1.instance.oc1.phx.anyhqljs7deiexachmv43kkomqnpnwg6ax76gjewaz6k7gad6pa77uka4giq"   #phnx_16core_049
  # "ocid1.instance.oc1.phx.anyhqljs7deiexach6eodaqhboilqw5fhxrerv6diah2ryggxtlr2iwx4eza"   #phnx_16core_050
  # "ocid1.instance.oc1.phx.anyhqljs7deiexac5cceqcpgztx7cmjwwahjq3s6524votpycl646ksyjaiq"   #phnx_16core_051
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacks4m2ngxsrs2scbd6kh4gpl7bzywuu5pliirnslmzsla"   #phnx_16core_052
  # "ocid1.instance.oc1.phx.anyhqljs7deiexact5ku7aguoav6kxrogwnr2zm4cp3ycvb3novgwqhkhqea"   #phnx_16core_053
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacyfk7bdpafgjsk7n5xnxcjth3dk4egm3ldcmy5amhxsia"   #phnx_16core_054
  # "ocid1.instance.oc1.phx.anyhqljs7deiexac5amiqsjdbx3ond4apd7j2p2c3wpzbo6gahongpbkklvq"   #phnx_16core_055
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacrjjpjbrwaxlw2edmn5g4jk7enphmauohs5g3huomdzxq"   #phnx_16core_056
  # "ocid1.instance.oc1.phx.anyhqljs7deiexac6smjlutyn7jz2woxsos3og6gmkmxjn63mwuodsf5kifa"   #phnx_16core_057
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacutfo6o4givt3wofjgmqba3wqqahdjlqguruabiidwy5a"   #phnx_16core_058
  # "ocid1.instance.oc1.phx.anyhqljs7deiexac7ps5u7jrdeuhqmcrtm5rea3ismmfekznn24f55dgwj6q"   #phnx_16core_059
  # "ocid1.instance.oc1.phx.anyhqljs7deiexac57cszkpzul5ipnlv3qqfzrvc6v47z7gpgsqcyabfecva"   #phnx_16core_060
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacluw2dv5dzohsawkxw5oiuhyawv6hr2qr4jf3lt3kgdba"   #phnx_16core_061
  # "ocid1.instance.oc1.phx.anyhqljs7deiexaceadcbpc355z2c7o4au4vt5xqkpguuvlrqgyb23c365qq"   #phnx_16core_062
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacced4zhpj6cz7bs3gjokzczd64kiqjoebiutbne4du6ba"   #phnx_16core_063
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacp2qpq3jnvs3n6yg5suvdxsvxpyzt6ts6maekv2pibfta"   #phnx_16core_064
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacgv3azfwxdt7xonnq2jygomdt5us554z7i5aly757bjiq"   #phnx_16core_065
  # "ocid1.instance.oc1.phx.anyhqljs7deiexactqhpmq5rzkdocxsgty275j4xco44al7rmdgqxnaq2ulq"   #phnx_16core_066
  # "ocid1.instance.oc1.phx.anyhqljs7deiexac5qoho6as7ztl6fpdlg3x7tckmyp7u2mu6khyymiipdtq"   #phnx_16core_067
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacgeqvwwbymol2cbmxf4ahkpw7ne3o4fdxgzwj47bsdbva"   #phnx_16core_068
  # "ocid1.instance.oc1.phx.anyhqljs7deiexachzugkqs4j4acyxk5x7qtlnzyel27gqdb4yiyqnbczwuq"   #phnx_16core_069
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacjsfkv4chdna3kudwrfnb4qm4lbwwlaeiwvo2rx552faa"   #phnx_16core_070
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacyvsc3bu5fqwcbwievqv66a4jwr5ynpzxlyfwjye6uleq"   #phnx_16core_071
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacsuan5earjw3bnpbwnbwlrcyeyvxbb4ph7accomdhmivq"   #phnx_16core_072
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacuilq4nvkd34ilsxqwrmg2nypwzh6dsmdgw6kdps5sxta"   #phnx_16core_073
  # "ocid1.instance.oc1.phx.anyhqljs7deiexaclzuizev27564julilji5daiudsf6x4jssexkacjbs2sa"   #phnx_16core_074
  # "ocid1.instance.oc1.phx.anyhqljs7deiexaciynevxl7c4fndoul2du5l6penryhomaeaukgqw7fd5qq"   #phnx_16core_075
  # "ocid1.instance.oc1.phx.anyhqljs7deiexac7mr75h6txwqaxosssywc26f5muccil56hcx2mv4y2yma"   #phnx_16core_076
  # "ocid1.instance.oc1.phx.anyhqljs7deiexaci73etvtdlopcqwxcw2uijxz3bzilcjpkfiof3axuoula"   #phnx_16core_077
  # "ocid1.instance.oc1.phx.anyhqljs7deiexactkl6w23lon5aitvq26ttdijrzr76djomdlxqxwagktqq"   #phnx_16core_078
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacvwyn53bzvvrkbdnzwe7beat4eqv6jzm4qk2wfhqssikq"   #phnx_16core_079
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacna7lg7ywza465rq42dc7bsb6zmverw4nekyz3sgs3siq"   #phnx_16core_080
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacixgaobg4kgee76dkmczumgwnneo2niz3ltdxy7kdnjcq"   #phnx_16core_081
  # "ocid1.instance.oc1.phx.anyhqljs7deiexac3v3c7gnxwa3zv4kgg7whufdbep23td5jfcythkzvofyq"   #phnx_16core_082
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacpcffc626d2targu57ernvpw7uc3ay4xk7umxcvx6haea"   #phnx_16core_083
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacubtypnjaedktaujglcx4diupcyq573jnbywlenlxf6ga"   #phnx_16core_084
  # "ocid1.instance.oc1.phx.anyhqljs7deiexac5cjey6atg4pqj7ozolpl5x6gpeqjfi2vklzd73lysprq"   #phnx_16core_085
  # "ocid1.instance.oc1.phx.anyhqljs7deiexac3z6ymoyjwqxowxrghz27nkxiqonth4cglrt7hhw2tpha"   #phnx_16core_086
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacwmfib5gvqovgsjs5u3hqacgttoxztobgtwizkaedgaqq"   #phnx_16core_087
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacgvmqt5qqd73vkmtqw7pd5j7qbe7xjuckgaid7ltvicra"   #phnx_16core_088
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacy3vle7i2n7ijh2eomsju6k7xdy3tfvkboza7pi6lghya"   #phnx_16core_089
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacnqvnp6i4xspkbvvrs2gomocqe5pcmfc6ws5zyldslxwq"   #phnx_16core_090
  # "ocid1.instance.oc1.phx.anyhqljs7deiexaca7qydn3i5d4or43fkv62d27rkfg2qxvjyv22fzm5hv5q"   #phnx_16core_091
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacbahwt7fnrux6zgqu37ah75caxye4qd2ioyvrkdcyuglq"   #phnx_16core_092
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacz2ms4or3vpub4ysupx3uehpbcuj3at4ascvr6eearltq"   #phnx_16core_093
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacnm6fnsgcne3yok75f2rw5sqjvvc6exer5t3npxv34jsq"   #phnx_16core_094
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacnqktoolzlnzi5kvago5vgpnjogsb7xrjan4v4acisrxa"   #phnx_16core_095
  # "ocid1.instance.oc1.phx.anyhqljs7deiexaccwnywwlzbbzrpdofbdps5fzorztki3sr23w7jxwva2za"   #phnx_16core_096
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacy6flauvi4zismmvvtbhemrvf7575azeq2wyx2xov7u5a"   #phnx_16core_097
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacj7cnag3gb7oydr4372z3rfgye2ygmmqq5xw5gdqolapq"   #phnx_16core_098
  # "ocid1.instance.oc1.phx.anyhqljs7deiexackbwcvjf5c7yrmgg6ecubteswiu7ik2n6pzrzf3uzz5ma"   #phnx_16core_099
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacgk7ngq775b2q5knslylinr2yfihkdgbbzufsmnudls4a"   #phnx_16core_100
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacfomiakm2ffsh4jfna2hltawku7t3xokatp2xpktmj2pq"   #phnx_16core_101
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacptpkoq4lnkskmjhr6vd42e53z3aeooq6dqvsyrhh7zsq"   #phnx_16core_102
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacoym2phckpmpom3hkvwenzv5hv66sehrallujl256bjcq"   #phnx_16core_103
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacq366gavoexqq7wgx7rulu5ht4bgaw73dzchkjmr6klfq"   #phnx_16core_104
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacbp3hnltijv7yjvjjuidrtlwghuw6copbkqg4axwgfyiq"   #phnx_16core_105
  # "ocid1.instance.oc1.phx.anyhqljs7deiexaczgwzqjqls63aygbcx46m5qcyaxi5cyvgqwj4fbvbbema"   #phnx_16core_106
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacm444yhzneliqe5gceejymnsrabk25fbn433c763bwqra"   #phnx_16core_107
  # "ocid1.instance.oc1.phx.anyhqljs7deiexaco7e6hhv6veu7st5wdhgbw4s7zgtpivkldl5kxofdofrq"   #phnx_16core_108
  # "ocid1.instance.oc1.phx.anyhqljs7deiexac3yuj5xpixxnh2akvu4vgey5xuoaye43b6lc523mamjzq"   #phnx_16core_109
  # "ocid1.instance.oc1.phx.anyhqljs7deiexac5hsy3prxzxbp7vsrfrrrrr34y77h5f4ii37xc4nk2rjq"   #phnx_16core_110
  # "ocid1.instance.oc1.phx.anyhqljs7deiexac6ctmnav7j4lvbvvoerfqounq2di22rggzxqesmvkyvwa"   #phnx_16core_111
  # "ocid1.instance.oc1.phx.anyhqljs7deiexaccvlksvqotf44dqrt5hw7zbkbqzqslflkw6r3soykqovq"   #phnx_16core_112
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacqld252j3yroatpasysjuvf6ry2dqvoun4a75ziqp2agq"   #phnx_16core_113
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacpp4vt3xqjimavw62jbrsr6owaijqsdmza2kxlfbovmqa"   #phnx_16core_114
  # "ocid1.instance.oc1.phx.anyhqljs7deiexaczgcn2rxomvh7o5bi2zjpis4ak4l5mn3i54gbw2zfhiaq"   #phnx_16core_115
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacfuaaxbw7xwdz47nrz3d24d5yzxhepnjivcmhqmfjzzba"   #phnx_16core_116
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacg3ddmkdibfzhqicjqw5hlninaxse222zd6xctiwsifja"   #phnx_16core_117
  # "ocid1.instance.oc1.phx.anyhqljs7deiexac5bpckpaz6kntyqd6ljsvtxun575a5yyv2ocekkmrtpha"   #phnx_16core_118
  # "ocid1.instance.oc1.phx.anyhqljs7deiexacl6o3cgauc2ew65w63mhbgpur7vcr4aqz4xlftrouobya"   #phnx_16core_119
  "ocid1.instance.oc1.phx.anyhqljs7deiexacodm7x4qucnolhsjt47jdsnt35rdpcfyitnlj42d5aiaa"   #phnx_16core_120
  "ocid1.instance.oc1.phx.anyhqljs7deiexacdcji7kg7zpxqqhodjuhlzy7w4x432jp5yh6c5furhoqq"   #phnx_16core_121
  "ocid1.instance.oc1.phx.anyhqljs7deiexacledscs3ac2ceofan5gpxmyfqjb63vzmiknmmorjd6nta"   #phnx_16core_122
  "ocid1.instance.oc1.phx.anyhqljs7deiexachghsawcvfknkfxyynkxirkkmjrqbv27gt34wp3ru3xia"   #phnx_16core_123
  "ocid1.instance.oc1.phx.anyhqljs7deiexacmffrjewlqmrwwrlgrbmhx7omwmdf34t7v7rurjxe2r7a"   #phnx_16core_124
  "ocid1.instance.oc1.phx.anyhqljs7deiexacunmc6p6uyr3yzvqmiitqi4a7l4ugqklvsxgfzmipybaq"   #phnx_16core_125
  "ocid1.instance.oc1.phx.anyhqljs7deiexac3y6siv5dpwjqrn7o75jlw3zrrqyplpwdtfyolmytkl6q"   #phnx_16core_126
  "ocid1.instance.oc1.phx.anyhqljs7deiexaczxjogrzcz72ajj66zzc6bocngifu4kac4imfk7nylbmq"   #phnx_16core_127
  "ocid1.instance.oc1.phx.anyhqljs7deiexacgm5kriwu2jtgiccruxairiil45j5rqxlx3cbp3rghuka"   #phnx_16core_128
  "ocid1.instance.oc1.phx.anyhqljs7deiexac3gfkpg2jxcwae3cubtfxn4ywkyckeywdzkp2h276obma"   #phnx_16core_129
  "ocid1.instance.oc1.phx.anyhqljs7deiexaciwdclpuosrih24kshkyux47di5xkp2rzrkptn3rmpgga"   #phnx_16core_130
  "ocid1.instance.oc1.phx.anyhqljs7deiexaco2lzoeeh47lvtdueddlq7a6qo4xxiorq3vmurkjofrdq"   #phnx_16core_131
  "ocid1.instance.oc1.phx.anyhqljs7deiexacli5nw2p46yi3y7vybne3agjklrnx5scte5y7mt6n467a"   #phnx_16core_132
  "ocid1.instance.oc1.phx.anyhqljs7deiexac7knkx6c7nuehqc4tqt6cpg4oqevq2ycfh3qw4srpoquq"   #phnx_16core_133
  "ocid1.instance.oc1.phx.anyhqljs7deiexac2b7xth3pmfwb5vl7x2icbdfqnjh6nfqm23udkjulx7ma"   #phnx_16core_134
  "ocid1.instance.oc1.phx.anyhqljs7deiexac5phw62awdj6x6l4hyu2l2iai5eirlzrarwtjmyvuwbya"   #phnx_16core_135
  "ocid1.instance.oc1.phx.anyhqljs7deiexaci577fknncn6kadkjieda6daek6cesr5luxtwvcmcruqq"   #phnx_16core_136
  "ocid1.instance.oc1.phx.anyhqljs7deiexacgyfjh6uazylaoez6ibbp7en6grpzk4wbqe7vtkvb5ncq"   #phnx_16core_137
  "ocid1.instance.oc1.phx.anyhqljs7deiexac53tpfg6zcdpvw5pd5iogznuk6ses5wlfbome36gzf2gq"   #phnx_16core_138
  "ocid1.instance.oc1.phx.anyhqljs7deiexac565aruft4nwhifsimzijulols26obfuvnfzwspzl2ijq"   #phnx_16core_139
  "ocid1.instance.oc1.phx.anyhqljs7deiexacn5elqidpwzyljg3zfkfi7bcsr7klexdcjzi4uqwpeyha"   #phnx_16core_140
  "ocid1.instance.oc1.phx.anyhqljs7deiexac44cxz72mfur37scrk74q5n2jj7pt75hi2jyimjvjeknq"   #phnx_16core_141
  "ocid1.instance.oc1.phx.anyhqljs7deiexacgx24xrtlmx4yc7fd2yrf2to5iftgnbavg2nwz7gkveqa"   #phnx_16core_142
  "ocid1.instance.oc1.phx.anyhqljs7deiexacnk7fg4ypvn2jyzf75uua4pqzaics5agxzmhbssx7vtwq"   #phnx_16core_143
  "ocid1.instance.oc1.phx.anyhqljs7deiexaccbcmuh5ycj5ripvcw3d2nhszib36scxnzwstdpyy55cq"   #phnx_16core_144
  "ocid1.instance.oc1.phx.anyhqljs7deiexacla2ypxcrhzv26yokodc2stckaqztcqg66esxvmtmpydq"   #phnx_16core_145
  "ocid1.instance.oc1.phx.anyhqljs7deiexacvdjhpgjhlp5qvfaxkrznqhcz3tapvfmb4qi3nzf7fbca"   #phnx_16core_146
  "ocid1.instance.oc1.phx.anyhqljs7deiexac7t2kegt5iv6hwzajyffortfca3fk234y6iwcoppxm26a"   #phnx_16core_147
  "ocid1.instance.oc1.phx.anyhqljs7deiexact4yhxj22tseehxzatnhbucfqnnhf65ozgutfbycrpaxq"   #phnx_16core_148
  "ocid1.instance.oc1.phx.anyhqljs7deiexacw3jdprv2cjd5bsmz57jn7vavag7tw3bzqg5vmlcgguqa"   #phnx_16core_149
  "ocid1.instance.oc1.phx.anyhqljs7deiexacsnedabwlqc553s65svmbhrqqdfonkgamchcneug2333a"   #phnx_16core_150
  "ocid1.instance.oc1.phx.anyhqljs7deiexacixyxq2tuakraijj4kr2hyn4ghf7ulb5viobmycszymbq"   #phnx_16core_151
  "ocid1.instance.oc1.phx.anyhqljs7deiexac2u2q33ttwfpxgv6nn4mq7xacahisgmwtk6oxuuxdecfa"   #phnx_16core_152
  "ocid1.instance.oc1.phx.anyhqljs7deiexacnpfltqobng5v4nwdmboqg34l6kzsdnsk7zzz2kfkaapa"   #phnx_16core_153
  "ocid1.instance.oc1.phx.anyhqljs7deiexacjvbmaqbortfa4klmqondohx477hx4ai6ep4juswnsf4a"   #phnx_16core_154
  "ocid1.instance.oc1.phx.anyhqljs7deiexac5bb6crxlevshytouqo22zjq2kzqaylfy3grpvupkjfrq"   #phnx_16core_155
  "ocid1.instance.oc1.phx.anyhqljs7deiexace4eqekvkadtadhyzp5vscnvvjhlhfue62n5eelf5vb6q"   #phnx_16core_156
  "ocid1.instance.oc1.phx.anyhqljs7deiexacq46djhs3kyscrezzmlwp7tpujnse2pl7d6xygbfaqndq"   #phnx_16core_157
  "ocid1.instance.oc1.phx.anyhqljs7deiexac5yralqcnl4pt2iqphhhdiphc7exfqx37cbyxdc5w3xsq"   #phnx_16core_158
  "ocid1.instance.oc1.phx.anyhqljs7deiexacurhgwzrl45t7ot7shj5qlwiupezv6jwcwpgcq2qlijya"   #phnx_16core_159
)

action=START
# action=STOP
# action=RESET
# bash scripts/start-stop-ashburn.sh

# for i in "${instances[@]}"; do
#    /home/ubuntu/bin/oci compute instance terminate --instance-id $i
# done
# wait
for i in "${instances[@]}"; do
    /home/ubuntu/bin/oci compute instance action --action $action --instance-id $i | jq -r '.data."display-name"' &
done
wait
