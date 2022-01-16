#EasyPot1

A good starting point to make sure your pot won't suffer due to lack of attention :)

Goal of this board is to introduce you to GrowNode platform, using super common components you can buy online.

##Features

- Soil moisture level detection with configurable low/high treshold
- Temperature detection
- Access to online tracking system to track your growing progress

The board is equipped with LED showing the need to water your plant, excess light and temperature out of target

##Bill of Material

- 1 [capacitive moisture level sensor](https://it.aliexpress.com/item/32864255890.html?spm=a2g0o.search0302.0.0.5c99558eRVKgmF&algo_pvid=fe8b7ef4-2e3a-4970-850d-d82c474e6777&aem_p4p_detail=202201051246061694626298027740030537040&algo_exp_id=fe8b7ef4-2e3a-4970-850d-d82c474e6777-0R)
- 1 [temperature sensor](https://it.aliexpress.com/item/1005001636433931.html?spm=a2g0o.productlist.0.0.2be22a659R9vD1&algo_pvid=29a85c9b-c44a-4d82-88d4-d77f482607d7&aem_p4p_detail=202201051248362525021342591050030527186&algo_exp_id=29a85c9b-c44a-4d82-88d4-d77f482607d7-5&pdp_ext_f=%7B%22sku_id%22%3A%2212000016918505158%22%7D&pdp_pi=-1%3B1.68%3B-1%3BEUR+1.29%40salePrice%3BEUR%3Bsearch-mainSearch)
- [LED](https://it.aliexpress.com/item/32626322055.html?spm=a2g0o.productlist.0.0.637b3bcfbiyAa5&algo_pvid=4d6cd08f-f9b7-4ff3-9a05-e983cb1dd17c&aem_p4p_detail=202201051257527368520968769800030588268&algo_exp_id=4d6cd08f-f9b7-4ff3-9a05-e983cb1dd17c-35&pdp_ext_f=%7B%22sku_id%22%3A%2259399079352%22%7D&pdp_pi=-1%3B3.03%3B-1%3BEUR+0.96%40salePrice%3BEUR%3Bsearch-mainSearch)
- [resistors](https://it.aliexpress.com/item/1005002631550177.html?spm=a2g0o.productlist.0.0.20cdbd970OCIhw&algo_pvid=9eff731c-5630-4da2-8214-68107f77e5b0&aem_p4p_detail=202201051259481008733339598140030601318&algo_exp_id=9eff731c-5630-4da2-8214-68107f77e5b0-0&pdp_ext_f=%7B%22sku_id%22%3A%2212000021480015801%22%7D&pdp_pi=-1%3B1.9%3B-1%3BEUR+0.79%40salePrice%3BEUR%3Bsearch-mainSearch)
- [breadboard and wires](https://it.aliexpress.com/item/4000805673115.html?spm=a2g0o.productlist.0.0.13c056a4HYIA1P&algo_pvid=3b89dd30-2e62-4c5e-9f47-a3d1ba448ff9&aem_p4p_detail=20220116111751259270886284480019365032&algo_exp_id=3b89dd30-2e62-4c5e-9f47-a3d1ba448ff9-14&pdp_ext_f=%7B%22sku_id%22%3A%2210000008092850406%22%7D&pdp_pi=-1%3B0.87%3B-1%3BEUR+0.59%40salePrice%3BEUR%3Bsearch-mainSearch)

##Assembling

- Wire your ESP32 to breadboard GND and 3V3
- Wire your DS18B20 to GND, 3V3 and GPIO0 through a 4K7ohm resistor
- Wire your moisture sensor to GND, 3V3 and GPIO12 
- Wire your blue (moisture) LED to GND and GPIO1 (also called TX0) through a 100ohm resistor
- Wire your red (temperature) LED to GND and GPIO2 through a 100ohm resistor

final configuration will look like:

![board](resources/images/board_easypot1_wiring.png)

##Code

The configuration code is contained in `components\grownode\boards\easypot1.c`.

##Controlling the pot

todo